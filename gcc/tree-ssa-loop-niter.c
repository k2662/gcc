/* Functions to determine/estimate number of iterations of a loop.
   Copyright (C) 2004 Free Software Foundation, Inc.
   
This file is part of GCC.
   
GCC is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.
   
GCC is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.
   
You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "rtl.h"
#include "tm_p.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "output.h"
#include "diagnostic.h"
#include "tree-flow.h"
#include "tree-dump.h"
#include "cfgloop.h"
#include "tree-pass.h"
#include "ggc.h"
#include "tree-fold-const.h"
#include "tree-chrec.h"
#include "tree-scalar-evolution.h"
#include "params.h"
#include "flags.h"
#include "tree-inline.h"

#define SWAP(X, Y) do { void *tmp = (X); (X) = (Y); (Y) = tmp; } while (0)

/* Just to shorten the ugly names.  */
#define EXEC_BINARY nondestructive_fold_binary_to_constant
#define EXEC_UNARY nondestructive_fold_unary_to_constant

/*

   Analysis of number of iterations of an affine exit test.

*/

/* Checks whether ARG is either NULL_TREE or constant zero.  */

static bool
zero_p (tree arg)
{
  if (!arg)
    return true;

  return integer_zerop (arg);
}

/* Computes inverse of X modulo 2^s, where MASK = 2^s-1.  */

static tree
inverse (tree x, tree mask)
{
  tree type = TREE_TYPE (x);
  tree ctr = EXEC_BINARY (RSHIFT_EXPR, type, mask, integer_one_node);
  tree rslt = convert (type, integer_one_node);

  while (integer_nonzerop (ctr))
    {
      rslt = EXEC_BINARY (MULT_EXPR, type, rslt, x);
      rslt = EXEC_BINARY (BIT_AND_EXPR, type, rslt, mask);
      x = EXEC_BINARY (MULT_EXPR, type, x, x);
      x = EXEC_BINARY (BIT_AND_EXPR, type, x, mask);
      ctr = EXEC_BINARY (RSHIFT_EXPR, type, ctr, integer_one_node);
    }

  return rslt;
}

/* Returns unsigned variant of TYPE.  */

static tree
unsigned_type_for (tree type)
{
  return make_unsigned_type (TYPE_PRECISION (type));
}

/* Returns signed variant of TYPE.  */

static tree
signed_type_for (tree type)
{
  return make_signed_type (TYPE_PRECISION (type));
}

/* Determine the number of iterations according to condition (for staying
   inside loop) BASE0 + STEP0 * i (CODE) BASE1 + STEP1 * i, computed in TYPE.
   Store the results to NITER.  */

void
number_of_iterations_cond (tree type, tree base0, tree step0,
			   enum tree_code code, tree base1, tree step1,
			   struct tree_niter_desc *niter)
{
  tree step, delta, mmin, mmax;
  tree may_xform, bound, s, d, tmp;
  bool was_sharp = false;
  tree assumption;
  tree assumptions = boolean_true_node;
  tree noloop_assumptions = boolean_false_node;
  tree niter_type, signed_niter_type;

  /* The meaning of these assumptions is this:
     if !assumptions
       then the rest of information does not have to be valid
     if noloop_assumptions then the loop does not have to roll
       (but it is only conservative approximation, i.e. it only says that
       if !noloop_assumptions, then the loop does not end before the computed
       number of iterations)  */

  /* Make < comparison from > ones.  */
  if (code == GE_EXPR
      || code == GT_EXPR)
    {
      SWAP (base0, base1);
      SWAP (step0, step1);
      code = swap_tree_comparison (code);
    }

  /* We can take care of the case of two induction variables chasing each other
     if the test is NE. I have never seen a loop using it, but still it is
     cool.  */
  if (!zero_p (step0) && !zero_p (step1))
    {
      if (code != NE_EXPR)
	return;

      step0 = EXEC_BINARY (MINUS_EXPR, type, step0, step1);
      step1 = NULL_TREE;
    }

  /* If the result is a constant,  the loop is weird.  More precise handling
     would be possible, but the situation is not common enough to waste time
     on it.  */
  if (zero_p (step0) && zero_p (step1))
    return;

  /* Ignore loops of while (i-- < 10) type.  */
  if (code != NE_EXPR)
    {
      if (step0 && !tree_expr_nonnegative_p (step0))
	return;

      if (!zero_p (step1) && tree_expr_nonnegative_p (step1))
	return;
    }

  if (TREE_CODE (type) == POINTER_TYPE)
    {
      /* We assume pointer arithmetics never overflows.  */
      mmin = mmax = NULL_TREE;
    }
  else
    {
      mmin = TYPE_MIN_VALUE (type);
      mmax = TYPE_MAX_VALUE (type);
    }

  /* Some more condition normalization.  We must record some assumptions
     due to overflows.  */

  if (code == LT_EXPR)
    {
      /* We want to take care only of <=; this is easy,
	 as in cases the overflow would make the transformation unsafe the loop
	 does not roll.  Seemingly it would make more sense to want to take
	 care of <, as NE is more simmilar to it, but the problem is that here
	 the transformation would be more difficult due to possibly infinite
	 loops.  */
      if (zero_p (step0))
	{
	  if (mmax)
	    assumption = fold (build (EQ_EXPR, boolean_type_node, base0, mmax));
	  else
	    assumption = boolean_false_node;
	  if (integer_nonzerop (assumption))
	    goto zero_iter;
	  base0 = fold (build (PLUS_EXPR, type, base0,
			       convert (type, integer_one_node)));
	}
      else
	{
	  if (mmin)
	    assumption = fold (build (EQ_EXPR, boolean_type_node, base1, mmin));
	  else
	    assumption = boolean_false_node;
	  if (integer_nonzerop (assumption))
	    goto zero_iter;
	  base1 = fold (build (MINUS_EXPR, type, base1,
			       convert (type, integer_one_node)));
	}
      noloop_assumptions = assumption;
      code = LE_EXPR;

      /* It will be useful to be able to tell the difference once more in
	 <= -> != reduction.  */
      was_sharp = true;
    }

  /* Take care of trivially infinite loops.  */
  if (code != NE_EXPR)
    {
      if (zero_p (step0)
	  && mmin
	  && operand_equal_p (base0, mmin, 0))
	return;
      if (zero_p (step1)
	  && mmax
	  && operand_equal_p (base1, mmax, 0))
	return;
    }

  /* If we can we want to take care of NE conditions instead of size
     comparisons, as they are much more friendly (most importantly
     this takes care of special handling of loops with step 1).  We can
     do it if we first check that upper bound is greater or equal to
     lower bound, their difference is constant c modulo step and that
     there is not an overflow.  */
  if (code != NE_EXPR)
    {
      if (zero_p (step0))
	step = EXEC_UNARY (NEGATE_EXPR, type, step1);
      else
	step = step0;
      delta = build (MINUS_EXPR, type, base1, base0);
      delta = fold (build (FLOOR_MOD_EXPR, type, delta, step));
      may_xform = boolean_false_node;

      if (TREE_CODE (delta) == INTEGER_CST)
	{
	  tmp = EXEC_BINARY (MINUS_EXPR, type, step,
			     convert (type, integer_one_node));
	  if (was_sharp
	      && operand_equal_p (delta, tmp, 0))
	    {
	      /* A special case.  We have transformed condition of type
		 for (i = 0; i < 4; i += 4)
		 into
		 for (i = 0; i <= 3; i += 4)
		 obviously if the test for overflow during that transformation
		 passed, we cannot overflow here.  Most importantly any
		 loop with sharp end condition and step 1 falls into this
		 cathegory, so handling this case specially is definitely
		 worth the troubles.  */
	      may_xform = boolean_true_node;
	    }
	  else if (zero_p (step0))
	    {
	      if (!mmin)
		may_xform = boolean_true_node;
	      else
		{
		  bound = EXEC_BINARY (PLUS_EXPR, type, mmin, step);
		  bound = EXEC_BINARY (MINUS_EXPR, type, bound, delta);
		  may_xform = fold (build (LE_EXPR, boolean_type_node,
					   bound, base0));
		}
	    }
	  else
	    {
	      if (!mmax)
		may_xform = boolean_true_node;
	      else
		{
		  bound = EXEC_BINARY (MINUS_EXPR, type, mmax, step);
		  bound = EXEC_BINARY (PLUS_EXPR, type, bound, delta);
		  may_xform = fold (build (LE_EXPR, boolean_type_node,
					   base1, bound));
		}
	    }
	}

      if (!integer_zerop (may_xform))
	{
	  /* We perform the transformation always provided that it is not
	     completely senseless.  This is OK, as we would need this assumption
	     to determine the number of iterations anyway.  */
	  if (!integer_nonzerop (may_xform))
	    assumptions = may_xform;

	  if (zero_p (step0))
	    {
	      base0 = build (PLUS_EXPR, type, base0, delta);
	      base0 = fold (build (MINUS_EXPR, type, base0, step));
	    }
	  else
	    {
	      base1 = build (MINUS_EXPR, type, base1, delta);
	      base1 = fold (build (PLUS_EXPR, type, base1, step));
	    }

	  assumption = fold (build (GT_EXPR, boolean_type_node, base0, base1));
	  noloop_assumptions = fold (build (TRUTH_OR_EXPR, boolean_type_node,
					    noloop_assumptions, assumption));
	  code = NE_EXPR;
	}
    }

  /* Count the number of iterations.  */
  niter_type = unsigned_type_for (type);
  signed_niter_type = signed_type_for (type);

  if (code == NE_EXPR)
    {
      /* Everything we do here is just arithmetics modulo size of mode.  This
	 makes us able to do more involved computations of number of iterations
	 than in other cases.  First transform the condition into shape
	 s * i <> c, with s positive.  */
      base1 = fold (build (MINUS_EXPR, type, base1, base0));
      base0 = NULL_TREE;
      if (!zero_p (step1))
  	step0 = EXEC_UNARY (NEGATE_EXPR, type, step1);
      step1 = NULL_TREE;
      if (!tree_expr_nonnegative_p (convert (signed_niter_type, step0)))
	{
	  step0 = EXEC_UNARY (NEGATE_EXPR, type, step0);
	  base1 = fold (build1 (NEGATE_EXPR, type, base1));
	}

      base1 = convert (niter_type, base1);
      step0 = convert (niter_type, step0);

      /* Let nsd (s, size of mode) = d.  If d does not divide c, the loop
	 is infinite.  Otherwise, the number of iterations is
	 (inverse(s/d) * (c/d)) mod (size of mode/d).  */
      s = step0;
      d = integer_one_node;
      bound = convert (niter_type, build_int_2 (~0, ~0));
      while (1)
	{
	  tmp = EXEC_BINARY (BIT_AND_EXPR, niter_type, s,
			     convert (niter_type, integer_one_node));
	  if (integer_nonzerop (tmp))
	    break;
	  
	  s = EXEC_BINARY (RSHIFT_EXPR, niter_type, s,
			   convert (niter_type, integer_one_node));
	  d = EXEC_BINARY (LSHIFT_EXPR, niter_type, d,
			   convert (niter_type, integer_one_node));
	  bound = EXEC_BINARY (RSHIFT_EXPR, niter_type, bound,
			       convert (niter_type, integer_one_node));
	}

      tmp = fold (build (EXACT_DIV_EXPR, niter_type, base1, d));
      tmp = fold (build (MULT_EXPR, niter_type, tmp, inverse (s, bound)));
      niter->niter = fold (build (BIT_AND_EXPR, niter_type, tmp, bound));
    }
  else
    {
      if (zero_p (step1))
	/* Condition in shape a + s * i <= b
	   We must know that b + s does not overflow and a <= b + s and then we
	   can compute number of iterations as (b + s - a) / s.  (It might
	   seem that we in fact could be more clever about testing the b + s
	   overflow condition using some information about b - a mod s,
	   but it was already taken into account during LE -> NE transform).  */
	{
	  if (mmax)
	    {
	      bound = EXEC_BINARY (MINUS_EXPR, type, mmax, step0);
	      assumption = fold (build (LE_EXPR, boolean_type_node,
					base1, bound));
	      assumptions = fold (build (TRUTH_AND_EXPR, boolean_type_node,
					 assumptions, assumption));
	    }

	  step = step0;
	  tmp = fold (build (PLUS_EXPR, type, base1, step0));
	  assumption = fold (build (GT_EXPR, boolean_type_node, base0, tmp));
	  delta = fold (build (PLUS_EXPR, type, base1, step));
	  delta = fold (build (MINUS_EXPR, type, delta, base0));
	  delta = convert (niter_type, delta);
	}
      else
	{
	  /* Condition in shape a <= b - s * i
	     We must know that a - s does not overflow and a - s <= b and then
	     we can again compute number of iterations as (b - (a - s)) / s.  */
	  if (mmin)
	    {
	      bound = EXEC_BINARY (MINUS_EXPR, type, mmin, step1);
	      assumption = fold (build (LE_EXPR, boolean_type_node,
					bound, base0));
	      assumptions = fold (build (TRUTH_AND_EXPR, boolean_type_node,
					 assumptions, assumption));
	    }
	  step = fold (build1 (NEGATE_EXPR, type, step1));
	  tmp = fold (build (PLUS_EXPR, type, base0, step1));
	  assumption = fold (build (GT_EXPR, boolean_type_node, tmp, base1));
	  delta = fold (build (MINUS_EXPR, type, base0, step));
	  delta = fold (build (MINUS_EXPR, type, base1, delta));
	  delta = convert (niter_type, delta);
	}
      noloop_assumptions = fold (build (TRUTH_OR_EXPR, boolean_type_node,
					noloop_assumptions, assumption));
      delta = fold (build (FLOOR_DIV_EXPR, niter_type, delta,
			   convert (niter_type, step)));
      niter->niter = delta;
    }

  niter->assumptions = assumptions;
  niter->may_be_zero = noloop_assumptions;
  return;

zero_iter:
  niter->assumptions = boolean_true_node;
  niter->may_be_zero = boolean_true_node;
  niter->niter = convert (type, integer_zero_node);
  return;
}

/* Stores description of number of iterations of LOOP derived from EXIT
   in NITER.  */

bool
number_of_iterations_exit (struct loop *loop, edge exit,
			   struct tree_niter_desc *niter)
{
  tree stmt, cond, type;
  tree op0, base0, step0;
  tree op1, base1, step1;
  enum tree_code code;

  if (!dominated_by_p (CDI_DOMINATORS, loop->latch, exit->src))
    return false;

  niter->assumptions = convert (boolean_type_node, integer_zero_node);
  stmt = last_stmt (exit->src);
  if (!stmt || TREE_CODE (stmt) != COND_EXPR)
    return false;

  /* We want the condition for staying inside loop.  */
  cond = COND_EXPR_COND (stmt);
  if (exit->flags & EDGE_TRUE_VALUE)
    cond = invert_truthvalue (cond);

  code = TREE_CODE (cond);
  switch (code)
    {
    case GT_EXPR:
    case GE_EXPR:
    case NE_EXPR:
    case LT_EXPR:
    case LE_EXPR:
      break;

    default:
      return false;
    }
  
  op0 = TREE_OPERAND (cond, 0);
  op1 = TREE_OPERAND (cond, 1);
  type = TREE_TYPE (op0);

  if (TREE_CODE (type) != INTEGER_TYPE
    && TREE_CODE (type) != POINTER_TYPE)
    return false;
     
  if (!simple_iv (loop, stmt, op0, &base0, &step0))
    return false;
  if (!simple_iv (loop, stmt, op1, &base1, &step1))
    return false;

  number_of_iterations_cond (type, base0, step0, code, base1, step1,
			     niter);
  return integer_onep (niter->assumptions);
}

/*

   Analysis of a number of iterations of a loop by a brute-force evaluation.

*/

/* Bound on the number of iterations we try to evaluate.  */

#define MAX_ITERATIONS_TO_TRACK 1000

/* Determines a loop phi node of LOOP such that X is derived from it
   by a chain of operations with constants.  */

static tree
chain_of_csts_start (struct loop *loop, tree x)
{
  tree stmt = SSA_NAME_DEF_STMT (x);
  basic_block bb = bb_for_stmt (stmt);
  use_optype uses;

  if (!bb
      || !flow_bb_inside_loop_p (loop, bb))
    return NULL_TREE;
  
  if (TREE_CODE (stmt) == PHI_NODE)
    {
      if (bb == loop->header)
	return stmt;

      return NULL_TREE;
    }

  if (TREE_CODE (stmt) != MODIFY_EXPR)
    return NULL_TREE;

  get_stmt_operands (stmt);
  if (NUM_VUSES (STMT_VUSE_OPS (stmt)) > 0)
    return NULL_TREE;
  if (NUM_VDEFS (STMT_VDEF_OPS (stmt)) > 0)
    return NULL_TREE;
  if (NUM_DEFS (STMT_DEF_OPS (stmt)) > 1)
    return NULL_TREE;
  uses = STMT_USE_OPS (stmt);
  if (NUM_USES (uses) != 1)
    return NULL_TREE;

  return chain_of_csts_start (loop, USE_OP (uses, 0));
}

/* Determines whether X is derived from a value of a phi node in LOOP
   such that

   * this derivation consists only from operations with constants
   * the initial value of the phi node is constant
   * its value in the next iteration can be derived from the current one
     by a chain of operations with constants.  */

static tree
get_base_for (struct loop *loop, tree x)
{
  tree phi, init, next;

  if (is_gimple_min_invariant (x))
    return x;

  phi = chain_of_csts_start (loop, x);
  if (!phi)
    return NULL_TREE;

  init = phi_element_for_edge (phi, loop_preheader_edge (loop))->def;
  next = phi_element_for_edge (phi, loop_latch_edge (loop))->def;

  if (TREE_CODE (next) != SSA_NAME)
    return NULL_TREE;

  if (!is_gimple_min_invariant (init))
    return NULL_TREE;

  if (chain_of_csts_start (loop, next) != phi)
    return NULL_TREE;

  return phi;
}

/* Evaluates value of X, provided that the value of the variable defined
   in the loop phi node from that X is derived by operations with constants
   is BASE.  */

static tree
get_val_for (tree x, tree base)
{
  tree stmt, *op, nx, val;
  use_optype uses;

  if (!x)
    return base;

  stmt = SSA_NAME_DEF_STMT (x);
  if (TREE_CODE (stmt) == PHI_NODE)
    return base;

  uses = STMT_USE_OPS (stmt);
  op = USE_OP_PTR (uses, 0);

  nx = *op;
  val = get_val_for (nx, base);
  *op = val;
  val = fold (TREE_OPERAND (stmt, 1));
  *op = nx;

  return val;
}

/* Tries to count the number of iterations of LOOP till it exits by EXIT
   by brute force.  */

tree
loop_niter_by_eval (struct loop *loop, edge exit)
{
  tree cond, cnd, acnd;
  tree op[2], val[2], next[2], aval[2], phi[2];
  unsigned i, j;
  enum tree_code cmp;

  cond = last_stmt (exit->src);
  if (!cond || TREE_CODE (cond) != COND_EXPR)
    return chrec_top;

  cnd = COND_EXPR_COND (cond);
  if (exit->flags & EDGE_TRUE_VALUE)
    cnd = invert_truthvalue (cnd);

  cmp = TREE_CODE (cnd);
  switch (cmp)
    {
    case EQ_EXPR:
    case NE_EXPR:
    case GT_EXPR:
    case GE_EXPR:
    case LT_EXPR:
    case LE_EXPR:
      for (j = 0; j < 2; j++)
	op[j] = TREE_OPERAND (cnd, j);
      break;

    default:
      return chrec_top;
    }

  for (j = 0; j < 2; j++)
    {
      phi[j] = get_base_for (loop, op[j]);
      if (!phi[j])
	return chrec_top;
    }

  for (j = 0; j < 2; j++)
    {
      if (TREE_CODE (phi[j]) == PHI_NODE)
	{
	  val[j] = phi_element_for_edge (phi[j],
					 loop_preheader_edge (loop))->def;
	  next[j] = phi_element_for_edge (phi[j],
					  loop_latch_edge (loop))->def;
	}
      else
	{
	  val[j] = phi[j];
	  next[j] = NULL_TREE;
	  op[j] = NULL_TREE;
	}
    }

  for (i = 0; i < MAX_ITERATIONS_TO_TRACK; i++)
    {
      for (j = 0; j < 2; j++)
	aval[j] = get_val_for (op[j], val[j]);

      acnd = fold (build (cmp, boolean_type_node, aval[0], aval[1]));
      if (integer_zerop (acnd))
	{
	  if (dump_file && (dump_flags & TDF_DETAILS))
	    fprintf (dump_file,
		     "Proved that loop %d iterates %d times using brute force.\n",
		     loop->num, i);
	  return build_int_2 (i, 0);
	}

      for (j = 0; j < 2; j++)
	val[j] = get_val_for (next[j], val[j]);
    }

  return chrec_top;
}

/* Finds the exit of the LOOP by that the loop exits after a constant
   number of iterations and stores it to *EXIT.  The iteration count
   is returned.  */

tree
find_loop_niter_by_eval (struct loop *loop, edge *exit)
{
  unsigned n_exits, i;
  edge *exits = get_loop_exit_edges (loop, &n_exits);
  edge ex;
  tree niter = NULL_TREE, aniter;

  *exit = NULL;
  for (i = 0; i < n_exits; i++)
    {
      ex = exits[i];
      if (!just_once_each_iteration_p (loop, ex->src))
	continue;

      aniter = loop_niter_by_eval (loop, ex);
      if (TREE_CODE (aniter) != INTEGER_CST)
	continue;

      if (niter
	  && !integer_nonzerop (fold (build (LT_EXPR, boolean_type_node,
					     aniter, niter))))
	continue;

      niter = aniter;
      *exit = ex;
    }
  free (exits);

  return niter ? niter : chrec_top;
}

/*

   Analysis of upper bounds on number of iterations of a loop.

*/

/* Bound on number of iterations of a loop.  */

struct nb_iter_bound
{
  tree bound;		/* The bound on the number of executions of anything
			   after ...  */
  tree at_stmt;		/* ... this statement during one execution of loop.  */
  struct nb_iter_bound *next;
			/* The next bound in a list.  */
};

/* Records that AT_STMT is executed at most BOUND times in LOOP.  */

static void
record_estimate (struct loop *loop, tree bound, tree at_stmt)
{
  struct nb_iter_bound *elt = xmalloc (sizeof (struct nb_iter_bound));

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "Statements after ");
      print_generic_expr (dump_file, at_stmt, TDF_SLIM);
      fprintf (dump_file, " are executed at most ");
      print_generic_expr (dump_file, bound, TDF_SLIM);
      fprintf (dump_file, " times in loop %d.\n", loop->num);
    }

  elt->bound = bound;
  elt->at_stmt = at_stmt;
  elt->next = loop->bounds;
  loop->bounds = elt;
}

/* Records estimates on numbers of iterations of LOOP.  */

static void
estimate_numbers_of_iterations_loop (struct loop *loop)
{
  edge *exits;
  tree niter, type;
  unsigned i, n_exits;
  struct tree_niter_desc niter_desc;

  /* First, use the scev information about the number of iterations.  */
  niter = number_of_iterations_in_loop (loop);
  if (niter != chrec_top)
    {
      type = TREE_TYPE (niter);
      niter = fold (build (MINUS_EXPR, type, niter,
			   convert (type, integer_one_node)));
      record_estimate (loop, niter, last_stmt (loop_exit_edge (loop, 0)->src));
    }

  /* Now use number_of_iterations_exit.  */
  exits = get_loop_exit_edges (loop, &n_exits);
  for (i = 0; i < n_exits; i++)
    {
      if (!number_of_iterations_exit (loop, exits[i], &niter_desc))
	continue;

      niter = niter_desc.niter;
      type = TREE_TYPE (niter);
      if (!integer_zerop (niter_desc.may_be_zero)
	  && !integer_nonzerop (niter_desc.may_be_zero))
	niter = build (COND_EXPR, type, niter_desc.may_be_zero,
		       convert (type, integer_zero_node),
		       niter);
      record_estimate (loop, niter, last_stmt (exits[i]->src));
    }
  free (exits);
  
  /* TODO Here we could use other possibilities, like bounds of arrays accessed
     in the loop.  */
}

/* Records estimates on numbers of iterations of LOOPS.  */

void
estimate_numbers_of_iterations (struct loops *loops)
{
  unsigned i;
  struct loop *loop;

  for (i = 1; i < loops->num; i++)
    {
      loop = loops->parray[i];
      if (loop)
	estimate_numbers_of_iterations_loop (loop);
    }
}

/* If A > B, returns -1.  If A == B, returns 0.  If A < B, returns 1.
   If neither of these relations can be proved, returns 2.  */

static int
compare_trees (tree a, tree b)
{
  tree typea = TREE_TYPE (a), typeb = TREE_TYPE (b);
  tree type;

  if (TYPE_PRECISION (typea) > TYPE_PRECISION (typeb))
    type = typea;
  else
    type = typeb;

  a = convert (type, a);
  b = convert (type, b);

  if (integer_nonzerop (fold (build (EQ_EXPR, boolean_type_node, a, b))))
    return 0;
  if (integer_nonzerop (fold (build (LT_EXPR, boolean_type_node, a, b))))
    return 1;
  if (integer_nonzerop (fold (build (GT_EXPR, boolean_type_node, a, b))))
    return -1;

  return 2;
}

/* Returns the largest value obtainable by casting something in INNER type to
   OUTER type.  */

static tree
upper_bound_in_type (tree outer, tree inner)
{
  unsigned HOST_WIDE_INT lo, hi;
  unsigned bits = TYPE_PRECISION (inner);

  if (TYPE_UNSIGNED (outer))
    {
      if (bits <= HOST_BITS_PER_WIDE_INT)
	{
	  hi = 0;
	  lo = (~(unsigned HOST_WIDE_INT) 0)
		  >> (HOST_BITS_PER_WIDE_INT - bits);
	}
      else
	{
	  hi = (~(unsigned HOST_WIDE_INT) 0)
		  >> (2 * HOST_BITS_PER_WIDE_INT - bits);
	  lo = ~(unsigned HOST_WIDE_INT) 0;
	}
    }
  else
    {
      if (bits <= HOST_BITS_PER_WIDE_INT)
	{
	  hi = 0;
	  lo = (~(unsigned HOST_WIDE_INT) 0)
		  >> (HOST_BITS_PER_WIDE_INT - bits) >> 1;
	}
      else
	{
	  hi = (~(unsigned HOST_WIDE_INT) 0)
		  >> (2 * HOST_BITS_PER_WIDE_INT - bits) >> 1;
	  lo = ~(unsigned HOST_WIDE_INT) 0;
	}
    }

  return convert (outer,
		  convert (inner,
			   build_int_2 (lo, hi)));
}

/* Returns the smallest value obtainable by casting something in INNER type to
   OUTER type.  */

static tree
lower_bound_in_type (tree outer, tree inner)
{
  unsigned HOST_WIDE_INT lo, hi;
  unsigned bits = TYPE_PRECISION (inner);

  if (TYPE_UNSIGNED (outer))
    lo = hi = 0;
  else if (bits <= HOST_BITS_PER_WIDE_INT)
    {
      hi = ~(unsigned HOST_WIDE_INT) 0;
      lo = (~(unsigned HOST_WIDE_INT) 0) << (bits - 1);
    }
  else
    {
      hi = (~(unsigned HOST_WIDE_INT) 0) << (bits - HOST_BITS_PER_WIDE_INT - 1);
      lo = 0;
    }

  return convert (outer,
		  convert (inner,
			   build_int_2 (lo, hi)));
}

/* Returns true if statement S1 dominates statement S2.  */

static bool
stmt_dominates_stmt_p (tree s1, tree s2)
{
  basic_block bb1 = bb_for_stmt (s1), bb2 = bb_for_stmt (s2);

  if (!bb1
      || s1 == s2)
    return true;

  if (bb1 == bb2)
    {
      block_stmt_iterator bsi;

      for (bsi = bsi_start (bb1); bsi_stmt (bsi) != s2; bsi_next (&bsi))
	if (bsi_stmt (bsi) == s1)
	  return true;

      return false;
    }

  return dominated_by_p (CDI_DOMINATORS, bb2, bb1);
}

/* Checks whether it is correct to count the induction variable BASE + STEP * I
   at AT_STMT in wider TYPE, using the fact that statement OF is executed at
   most BOUND times in the loop.  If it is possible, return the value of step in
   the TYPE, otherwise return NULL_TREE.  */

static tree
can_count_iv_in_wider_type_bound (tree type, tree base, tree step,
				  tree at_stmt,
				  tree bound, tree of)
{
  tree inner_type = TREE_TYPE (base), b, bplusstep, new_step, new_step_abs;
  tree valid_niter, extreme, unsigned_type, delta, bound_type;

  b = convert (type, base);
  bplusstep = convert (type,
		       fold (build (PLUS_EXPR, inner_type, base, step)));
  new_step = fold (build (MINUS_EXPR, type, bplusstep, b));
  if (TREE_CODE (new_step) != INTEGER_CST)
    return NULL_TREE;

  switch (compare_trees (bplusstep, b))
    {
    case -1:
      extreme = upper_bound_in_type (type, inner_type);
      delta = fold (build (MINUS_EXPR, type, extreme, b));
      new_step_abs = new_step;
      break;

    case 1:
      extreme = lower_bound_in_type (type, inner_type);
      new_step_abs = fold (build (NEGATE_EXPR, type, new_step));
      delta = fold (build (MINUS_EXPR, type, b, extreme));
      break;

    case 0:
      return new_step;

    default:
      return NULL_TREE;
    }

  unsigned_type = unsigned_type_for (type);
  delta = convert (unsigned_type, delta);
  new_step_abs = convert (unsigned_type, new_step_abs);
  valid_niter = fold (build (FLOOR_DIV_EXPR, unsigned_type,
			     delta, new_step_abs));

  bound_type = TREE_TYPE (bound);
  if (TYPE_PRECISION (type) > TYPE_PRECISION (bound_type))
    bound = convert (unsigned_type, bound);
  else
    valid_niter = convert (bound_type, valid_niter);
    
  if (at_stmt && stmt_dominates_stmt_p (of, at_stmt))
    {
      /* After the statement OF we know that anything is executed at most
	 BOUND times.  */
      if (integer_nonzerop (fold (build (GE_EXPR, boolean_type_node,
					 valid_niter, bound))))
	return new_step;
    }
  else
    {
      /* Before the statement OF we know that anything is executed at most
	 BOUND + 1 times.  */
      if (integer_nonzerop (fold (build (GT_EXPR, boolean_type_node,
					 valid_niter, bound))))
	return new_step;
    }

  return NULL_TREE;
}

/* Checks whether it is correct to count the induction variable BASE + STEP * I
   at AT_STMT in wider TYPE, using the bounds on numbers of iterations of a
   LOOP.  If it is possible, return the value of step in the TYPE, otherwise
   return NULL_TREE.  */

tree
can_count_iv_in_wider_type (struct loop *loop, tree type, tree base, tree step,
			    tree at_stmt)
{
  struct nb_iter_bound *bound;
  tree new_step;

  for (bound = loop->bounds; bound; bound = bound->next)
    {
      new_step = can_count_iv_in_wider_type_bound (type, base, step,
						   at_stmt,
						   bound->bound,
						   bound->at_stmt);

      if (new_step)
	return new_step;
    }

  return NULL_TREE;
}

/* Frees the information on upper bounds on numbers of iterations of LOOP.  */

static void
free_numbers_of_iterations_estimates_loop (struct loop *loop)
{
  struct nb_iter_bound *bound, *next;
  
  for (bound = loop->bounds; bound; bound = next)
    {
      next = bound->next;
      free (bound);
    }

  loop->bounds = NULL;
}

/* Frees the information on upper bounds on numbers of iterations of LOOPS.  */

void
free_numbers_of_iterations_estimates (struct loops *loops)
{
  unsigned i;
  struct loop *loop;

  for (i = 1; i < loops->num; i++)
    {
      loop = loops->parray[i];
      if (loop)
	free_numbers_of_iterations_estimates_loop (loop);
    }
}