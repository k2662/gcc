/* Loop unrolling and peeling.
   Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
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
#include "rtl.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "cfgloop.h"
#include "cfglayout.h"
#include "params.h"
#include "output.h"
#include "expr.h"

/* This pass performs loop unrolling and peeling.  We only perform these
   optimizations on innermost loops (with single exception) because
   the impact on performance is greatest here, and we want to avoid
   unnecessary code size growth.  The gain is caused by greater sequentiality
   of code, better code to optimize for further passes and in some cases
   by fewer testings of exit conditions.  The main problem is code growth,
   that impacts performance negatively due to effect of caches.

   What we do:

   -- complete peeling of once-rolling loops; this is the above mentioned
      exception, as this causes loop to be cancelled completely and
      does not cause code growth
   -- complete peeling of loops that roll (small) constant times.
   -- simple peeling of first iterations of loops that do not roll much
      (according to profile feedback)
   -- unrolling of loops that roll constant times; this is almost always
      win, as we get rid of exit condition tests.
   -- unrolling of loops that roll number of times that we can compute
      in runtime; we also get rid of exit condition tests here, but there
      is the extra expense for calculating the number of iterations
   -- simple unrolling of remaining loops; this is performed only if we
      are asked to, as the gain is questionable in this case and often
      it may even slow down the code
   For more detailed descriptions of each of those, see comments at
   appropriate function below.

   There is a lot of parameters (defined and described in params.def) that
   control how much we unroll/peel.

   ??? A great problem is that we don't have a good way how to determine
   how many times we should unroll the loop; the experiments I have made
   showed that this choice may affect performance in order of several %.
   */

static void decide_unrolling_and_peeling (struct loops *, int);
static void peel_loops_completely (struct loops *, int);
static void decide_peel_simple (struct loop *, int);
static void decide_peel_once_rolling (struct loop *, int);
static void decide_peel_completely (struct loop *, int);
static void decide_unroll_stupid (struct loop *, int);
static void decide_unroll_constant_iterations (struct loop *, int);
static void decide_unroll_runtime_iterations (struct loop *, int);
static void peel_loop_simple (struct loops *, struct loop *);
static void peel_loop_completely (struct loops *, struct loop *);
static void unroll_loop_stupid (struct loops *, struct loop *);
static void unroll_loop_constant_iterations (struct loops *, struct loop *);
static void unroll_loop_runtime_iterations (struct loops *, struct loop *);

/* Unroll and/or peel (depending on FLAGS) LOOPS.  */
void
unroll_and_peel_loops (struct loops *loops, int flags)
{
  struct loop *loop, *next;
  bool check;

  /* First perform complete loop peeling (it is almost surely a win,
     and affects parameters for further decision a lot).  */
  peel_loops_completely (loops, flags);

  /* Now decide rest of unrolling and peeling.  */
  decide_unrolling_and_peeling (loops, flags);

  loop = loops->tree_root;
  while (loop->inner)
    loop = loop->inner;

  /* Scan the loops, inner ones first.  */
  while (loop != loops->tree_root)
    {
      if (loop->next)
	{
	  next = loop->next;
	  while (next->inner)
	    next = next->inner;
	}
      else
	next = loop->outer;

      check = true;
      /* And perform the appropriate transformations.  */
      switch (loop->lpt_decision.decision)
	{
	case LPT_PEEL_COMPLETELY:
	  /* Already done.  */
	  abort ();
	case LPT_PEEL_SIMPLE:
	  peel_loop_simple (loops, loop);
	  break;
	case LPT_UNROLL_CONSTANT:
	  unroll_loop_constant_iterations (loops, loop);
	  break;
	case LPT_UNROLL_RUNTIME:
	  unroll_loop_runtime_iterations (loops, loop);
	  break;
	case LPT_UNROLL_STUPID:
	  unroll_loop_stupid (loops, loop);
	  break;
	case LPT_NONE:
	  check = false;
	  break;
	default:
	  abort ();
	}
      if (check)
	{
#ifdef ENABLE_CHECKING
	  verify_dominators (CDI_DOMINATORS);
	  verify_loop_structure (loops);
#endif
	}
      loop = next;
    }

  iv_analysis_done ();
}

/* Check whether exit of the LOOP is at the end of loop body.  */

static bool
loop_exit_at_end_p (struct loop *loop)
{
  struct niter_desc *desc = get_simple_loop_desc (loop);
  rtx insn;

  if (desc->in_edge->dest != loop->latch)
    return false;

  /* Check that the latch is empty.  */
  FOR_BB_INSNS (loop->latch, insn)
    {
      if (INSN_P (insn))
	return false;
    }

  return true;
}

/* Check whether to peel LOOPS (depending on FLAGS) completely and do so.  */
static void
peel_loops_completely (struct loops *loops, int flags)
{
  struct loop *loop, *next;

  loop = loops->tree_root;
  while (loop->inner)
    loop = loop->inner;

  while (loop != loops->tree_root)
    {
      if (loop->next)
	{
	  next = loop->next;
	  while (next->inner)
	    next = next->inner;
	}
      else
	next = loop->outer;

      loop->lpt_decision.decision = LPT_NONE;

      if (dump_file)
	fprintf (dump_file,
		 "\n;; *** Considering loop %d for complete peeling ***\n",
		 loop->num);

      loop->ninsns = num_loop_insns (loop);

      decide_peel_once_rolling (loop, flags);
      if (loop->lpt_decision.decision == LPT_NONE)
	decide_peel_completely (loop, flags);

      if (loop->lpt_decision.decision == LPT_PEEL_COMPLETELY)
	{
	  peel_loop_completely (loops, loop);
#ifdef ENABLE_CHECKING
	  verify_dominators (CDI_DOMINATORS);
	  verify_loop_structure (loops);
#endif
	}
      loop = next;
    }
}

/* Decide whether unroll or peel LOOPS (depending on FLAGS) and how much.  */
static void
decide_unrolling_and_peeling (struct loops *loops, int flags)
{
  struct loop *loop = loops->tree_root, *next;

  while (loop->inner)
    loop = loop->inner;

  /* Scan the loops, inner ones first.  */
  while (loop != loops->tree_root)
    {
      if (loop->next)
	{
	  next = loop->next;
	  while (next->inner)
	    next = next->inner;
	}
      else
	next = loop->outer;

      loop->lpt_decision.decision = LPT_NONE;

      if (dump_file)
	fprintf (dump_file, "\n;; *** Considering loop %d ***\n", loop->num);

      /* Do not peel cold areas.  */
      if (!maybe_hot_bb_p (loop->header))
	{
	  if (dump_file)
	    fprintf (dump_file, ";; Not considering loop, cold area\n");
	  loop = next;
	  continue;
	}

      /* Can the loop be manipulated?  */
      if (!can_duplicate_loop_p (loop))
	{
	  if (dump_file)
	    fprintf (dump_file,
		     ";; Not considering loop, cannot duplicate\n");
	  loop = next;
	  continue;
	}

      /* Skip non-innermost loops.  */
      if (loop->inner)
	{
	  if (dump_file)
	    fprintf (dump_file, ";; Not considering loop, is not innermost\n");
	  loop = next;
	  continue;
	}

      loop->ninsns = num_loop_insns (loop);
      loop->av_ninsns = average_num_loop_insns (loop);

      /* Try transformations one by one in decreasing order of
	 priority.  */

      decide_unroll_constant_iterations (loop, flags);
      if (loop->lpt_decision.decision == LPT_NONE)
	decide_unroll_runtime_iterations (loop, flags);
      if (loop->lpt_decision.decision == LPT_NONE)
	decide_unroll_stupid (loop, flags);
      if (loop->lpt_decision.decision == LPT_NONE)
	decide_peel_simple (loop, flags);

      loop = next;
    }
}

/* Decide whether the LOOP is once rolling and suitable for complete
   peeling.  */
static void
decide_peel_once_rolling (struct loop *loop, int flags ATTRIBUTE_UNUSED)
{
  struct niter_desc *desc;

  if (dump_file)
    fprintf (dump_file, "\n;; Considering peeling once rolling loop\n");

  /* Is the loop small enough?  */
  if ((unsigned) PARAM_VALUE (PARAM_MAX_ONCE_PEELED_INSNS) < loop->ninsns)
    {
      if (dump_file)
	fprintf (dump_file, ";; Not considering loop, is too big\n");
      return;
    }

  /* Check for simple loops.  */
  desc = get_simple_loop_desc (loop);

  /* Check number of iterations.  */
  if (!desc->simple_p
      || desc->assumptions
      || !desc->const_iter
      || desc->niter != 0)
    {
      if (dump_file)
	fprintf (dump_file,
		 ";; Unable to prove that the loop rolls exactly once\n");
      return;
    }

  /* Success.  */
  if (dump_file)
    fprintf (dump_file, ";; Decided to peel exactly once rolling loop\n");
  loop->lpt_decision.decision = LPT_PEEL_COMPLETELY;
}

/* Decide whether the LOOP is suitable for complete peeling.  */
static void
decide_peel_completely (struct loop *loop, int flags ATTRIBUTE_UNUSED)
{
  unsigned npeel;
  struct niter_desc *desc;

  if (dump_file)
    fprintf (dump_file, "\n;; Considering peeling completely\n");

  /* Skip non-innermost loops.  */
  if (loop->inner)
    {
      if (dump_file)
	fprintf (dump_file, ";; Not considering loop, is not innermost\n");
      return;
    }

  /* Do not peel cold areas.  */
  if (!maybe_hot_bb_p (loop->header))
    {
      if (dump_file)
	fprintf (dump_file, ";; Not considering loop, cold area\n");
      return;
    }

  /* Can the loop be manipulated?  */
  if (!can_duplicate_loop_p (loop))
    {
      if (dump_file)
	fprintf (dump_file,
		 ";; Not considering loop, cannot duplicate\n");
      return;
    }

  /* npeel = number of iterations to peel.  */
  npeel = PARAM_VALUE (PARAM_MAX_COMPLETELY_PEELED_INSNS) / loop->ninsns;
  if (npeel > (unsigned) PARAM_VALUE (PARAM_MAX_COMPLETELY_PEEL_TIMES))
    npeel = PARAM_VALUE (PARAM_MAX_COMPLETELY_PEEL_TIMES);

  /* Is the loop small enough?  */
  if (!npeel)
    {
      if (dump_file)
	fprintf (dump_file, ";; Not considering loop, is too big\n");
      return;
    }

  /* Check for simple loops.  */
  desc = get_simple_loop_desc (loop);

  /* Check number of iterations.  */
  if (!desc->simple_p
      || desc->assumptions
      || !desc->const_iter)
    {
      if (dump_file)
	fprintf (dump_file,
		 ";; Unable to prove that the loop iterates constant times\n");
      return;
    }

  if (desc->niter > npeel - 1)
    {
      if (dump_file)
	{
	  fprintf (dump_file,
		   ";; Not peeling loop completely, rolls too much (");
	  fprintf (dump_file, HOST_WIDEST_INT_PRINT_DEC, desc->niter);
	  fprintf (dump_file, " iterations > %d [maximum peelings])\n", npeel);
	}
      return;
    }

  /* Success.  */
  if (dump_file)
    fprintf (dump_file, ";; Decided to peel loop completely\n");
  loop->lpt_decision.decision = LPT_PEEL_COMPLETELY;
}

/* Peel all iterations of LOOP, remove exit edges and cancel the loop
   completely.  The transformation done:

   for (i = 0; i < 4; i++)
     body;

   ==>

   i = 0;
   body; i++;
   body; i++;
   body; i++;
   body; i++;
   */
static void
peel_loop_completely (struct loops *loops, struct loop *loop)
{
  sbitmap wont_exit;
  unsigned HOST_WIDE_INT npeel;
  unsigned n_remove_edges, i;
  edge *remove_edges, ei;
  struct niter_desc *desc = get_simple_loop_desc (loop);

  npeel = desc->niter;

  if (npeel)
    {
      wont_exit = sbitmap_alloc (npeel + 1);
      sbitmap_ones (wont_exit);
      RESET_BIT (wont_exit, 0);
      if (desc->noloop_assumptions)
	RESET_BIT (wont_exit, 1);

      remove_edges = xcalloc (npeel, sizeof (edge));
      n_remove_edges = 0;

      if (!duplicate_loop_to_header_edge (loop, loop_preheader_edge (loop),
		loops, npeel,
		wont_exit, desc->out_edge, remove_edges, &n_remove_edges,
		DLTHE_FLAG_UPDATE_FREQ))
	abort ();

      free (wont_exit);

      /* Remove the exit edges.  */
      for (i = 0; i < n_remove_edges; i++)
	remove_path (loops, remove_edges[i]);
      free (remove_edges);
    }

  ei = desc->in_edge;
  free_simple_loop_desc (loop);

  /* Now remove the unreachable part of the last iteration and cancel
     the loop.  */
  remove_path (loops, ei);

  if (dump_file)
    fprintf (dump_file, ";; Peeled loop completely, %d times\n", (int) npeel);
}

/* Decide whether to unroll LOOP iterating constant number of times
   and how much.  */

static void
decide_unroll_constant_iterations (struct loop *loop, int flags)
{
  unsigned nunroll, nunroll_by_av, best_copies, best_unroll = 0, n_copies, i;
  struct niter_desc *desc;

  if (!(flags & UAP_UNROLL))
    {
      /* We were not asked to, just return back silently.  */
      return;
    }

  if (dump_file)
    fprintf (dump_file,
	     "\n;; Considering unrolling loop with constant "
	     "number of iterations\n");

  /* nunroll = total number of copies of the original loop body in
     unrolled loop (i.e. if it is 2, we have to duplicate loop body once.  */
  nunroll = PARAM_VALUE (PARAM_MAX_UNROLLED_INSNS) / loop->ninsns;
  nunroll_by_av
    = PARAM_VALUE (PARAM_MAX_AVERAGE_UNROLLED_INSNS) / loop->av_ninsns;
  if (nunroll > nunroll_by_av)
    nunroll = nunroll_by_av;
  if (nunroll > (unsigned) PARAM_VALUE (PARAM_MAX_UNROLL_TIMES))
    nunroll = PARAM_VALUE (PARAM_MAX_UNROLL_TIMES);

  /* Skip big loops.  */
  if (nunroll <= 1)
    {
      if (dump_file)
	fprintf (dump_file, ";; Not considering loop, is too big\n");
      return;
    }

  /* Check for simple loops.  */
  desc = get_simple_loop_desc (loop);

  /* Check number of iterations.  */
  if (!desc->simple_p || !desc->const_iter || desc->assumptions)
    {
      if (dump_file)
	fprintf (dump_file,
		 ";; Unable to prove that the loop iterates constant times\n");
      return;
    }

  /* Check whether the loop rolls enough to consider.  */
  if (desc->niter < 2 * nunroll)
    {
      if (dump_file)
	fprintf (dump_file, ";; Not unrolling loop, doesn't roll\n");
      return;
    }

  /* Success; now compute number of iterations to unroll.  We alter
     nunroll so that as few as possible copies of loop body are
     necessary, while still not decreasing the number of unrollings
     too much (at most by 1).  */
  best_copies = 2 * nunroll + 10;

  i = 2 * nunroll + 2;
  if (i - 1 >= desc->niter)
    i = desc->niter - 2;

  for (; i >= nunroll - 1; i--)
    {
      unsigned exit_mod = desc->niter % (i + 1);

      if (!loop_exit_at_end_p (loop))
	n_copies = exit_mod + i + 1;
      else if (exit_mod != (unsigned) i
	       || desc->noloop_assumptions != NULL_RTX)
	n_copies = exit_mod + i + 2;
      else
	n_copies = i + 1;

      if (n_copies < best_copies)
	{
	  best_copies = n_copies;
	  best_unroll = i;
	}
    }

  if (dump_file)
    fprintf (dump_file, ";; max_unroll %d (%d copies, initial %d).\n",
	     best_unroll + 1, best_copies, nunroll);

  loop->lpt_decision.decision = LPT_UNROLL_CONSTANT;
  loop->lpt_decision.times = best_unroll;
  
  if (dump_file)
    fprintf (dump_file,
	     ";; Decided to unroll the constant times rolling loop, %d times.\n",
	     loop->lpt_decision.times);
}

/* Unroll LOOP with constant number of iterations LOOP->LPT_DECISION.TIMES + 1
   times.  The transformation does this:

   for (i = 0; i < 102; i++)
     body;

   ==>

   i = 0;
   body; i++;
   body; i++;
   while (i < 102)
     {
       body; i++;
       body; i++;
       body; i++;
       body; i++;
     }
  */
static void
unroll_loop_constant_iterations (struct loops *loops, struct loop *loop)
{
  unsigned HOST_WIDE_INT niter;
  unsigned exit_mod;
  sbitmap wont_exit;
  unsigned n_remove_edges, i;
  edge *remove_edges;
  unsigned max_unroll = loop->lpt_decision.times;
  struct niter_desc *desc = get_simple_loop_desc (loop);
  bool exit_at_end = loop_exit_at_end_p (loop);

  niter = desc->niter;

  if (niter <= max_unroll + 1)
    abort ();  /* Should not get here (such loop should be peeled instead).  */

  exit_mod = niter % (max_unroll + 1);

  wont_exit = sbitmap_alloc (max_unroll + 1);
  sbitmap_ones (wont_exit);

  remove_edges = xcalloc (max_unroll + exit_mod + 1, sizeof (edge));
  n_remove_edges = 0;

  if (!exit_at_end)
    {
      /* The exit is not at the end of the loop; leave exit test
	 in the first copy, so that the loops that start with test
	 of exit condition have continuous body after unrolling.  */

      if (dump_file)
	fprintf (dump_file, ";; Condition on beginning of loop.\n");

      /* Peel exit_mod iterations.  */
      RESET_BIT (wont_exit, 0);
      if (desc->noloop_assumptions)
	RESET_BIT (wont_exit, 1);

      if (exit_mod)
	{
	  if (!duplicate_loop_to_header_edge (loop, loop_preheader_edge (loop),
					      loops, exit_mod,
					      wont_exit, desc->out_edge,
					      remove_edges, &n_remove_edges,
					      DLTHE_FLAG_UPDATE_FREQ))
	    abort ();

	  desc->noloop_assumptions = NULL_RTX;
	  desc->niter -= exit_mod;
	  desc->niter_max -= exit_mod;
	}

      SET_BIT (wont_exit, 1);
    }
  else
    {
      /* Leave exit test in last copy, for the same reason as above if
	 the loop tests the condition at the end of loop body.  */

      if (dump_file)
	fprintf (dump_file, ";; Condition on end of loop.\n");

      /* We know that niter >= max_unroll + 2; so we do not need to care of
	 case when we would exit before reaching the loop.  So just peel
	 exit_mod + 1 iterations.  */
      if (exit_mod != max_unroll
	  || desc->noloop_assumptions)
	{
	  RESET_BIT (wont_exit, 0);
	  if (desc->noloop_assumptions)
	    RESET_BIT (wont_exit, 1);

	  if (!duplicate_loop_to_header_edge (loop, loop_preheader_edge (loop),
		loops, exit_mod + 1,
		wont_exit, desc->out_edge, remove_edges, &n_remove_edges,
		DLTHE_FLAG_UPDATE_FREQ))
	    abort ();

	  desc->niter -= exit_mod + 1;
	  desc->niter_max -= exit_mod + 1;
	  desc->noloop_assumptions = NULL_RTX;

	  SET_BIT (wont_exit, 0);
	  SET_BIT (wont_exit, 1);
	}

      RESET_BIT (wont_exit, max_unroll);
    }

  /* Now unroll the loop.  */
  if (!duplicate_loop_to_header_edge (loop, loop_latch_edge (loop),
		loops, max_unroll,
		wont_exit, desc->out_edge, remove_edges, &n_remove_edges,
		DLTHE_FLAG_UPDATE_FREQ))
    abort ();

  free (wont_exit);

  if (exit_at_end)
    {
      basic_block exit_block = desc->in_edge->src->rbi->copy;
      /* Find a new in and out edge; they are in the last copy we have made.  */
      
      if (exit_block->succ->dest == desc->out_edge->dest)
	{
	  desc->out_edge = exit_block->succ;
	  desc->in_edge = exit_block->succ->succ_next;
	}
      else
	{
	  desc->out_edge = exit_block->succ->succ_next;
	  desc->in_edge = exit_block->succ;
	}
    }

  desc->niter /= max_unroll + 1;
  desc->niter_max /= max_unroll + 1;
  desc->niter_expr = GEN_INT (desc->niter);

  /* Remove the edges.  */
  for (i = 0; i < n_remove_edges; i++)
    remove_path (loops, remove_edges[i]);
  free (remove_edges);

  if (dump_file)
    fprintf (dump_file,
	     ";; Unrolled loop %d times, constant # of iterations %i insns\n",
	     max_unroll, num_loop_insns (loop));
}

/* Decide whether to unroll LOOP iterating runtime computable number of times
   and how much.  */
static void
decide_unroll_runtime_iterations (struct loop *loop, int flags)
{
  unsigned nunroll, nunroll_by_av, i;
  struct niter_desc *desc;

  if (!(flags & UAP_UNROLL))
    {
      /* We were not asked to, just return back silently.  */
      return;
    }

  if (dump_file)
    fprintf (dump_file,
	     "\n;; Considering unrolling loop with runtime "
	     "computable number of iterations\n");

  /* nunroll = total number of copies of the original loop body in
     unrolled loop (i.e. if it is 2, we have to duplicate loop body once).  */
  nunroll = PARAM_VALUE (PARAM_MAX_UNROLLED_INSNS) / loop->ninsns;
  nunroll_by_av = PARAM_VALUE (PARAM_MAX_AVERAGE_UNROLLED_INSNS) / loop->av_ninsns;
  if (nunroll > nunroll_by_av)
    nunroll = nunroll_by_av;
  if (nunroll > (unsigned) PARAM_VALUE (PARAM_MAX_UNROLL_TIMES))
    nunroll = PARAM_VALUE (PARAM_MAX_UNROLL_TIMES);

  /* Skip big loops.  */
  if (nunroll <= 1)
    {
      if (dump_file)
	fprintf (dump_file, ";; Not considering loop, is too big\n");
      return;
    }

  /* Check for simple loops.  */
  desc = get_simple_loop_desc (loop);

  /* Check simpleness.  */
  if (!desc->simple_p || desc->assumptions)
    {
      if (dump_file)
	fprintf (dump_file,
		 ";; Unable to prove that the number of iterations "
		 "can be counted in runtime\n");
      return;
    }

  if (desc->const_iter)
    {
      if (dump_file)
	fprintf (dump_file, ";; Loop iterates constant times\n");
      return;
    }

  /* If we have profile feedback, check whether the loop rolls.  */
  if (loop->header->count && expected_loop_iterations (loop) < 2 * nunroll)
    {
      if (dump_file)
	fprintf (dump_file, ";; Not unrolling loop, doesn't roll\n");
      return;
    }

  /* Success; now force nunroll to be power of 2, as we are unable to
     cope with overflows in computation of number of iterations.  */
  for (i = 1; 2 * i <= nunroll; i *= 2)
    continue;

  loop->lpt_decision.decision = LPT_UNROLL_RUNTIME;
  loop->lpt_decision.times = i - 1;
  
  if (dump_file)
    fprintf (dump_file,
	     ";; Decided to unroll the runtime computable "
	     "times rolling loop, %d times.\n",
	     loop->lpt_decision.times);
}

/* Unroll LOOP for that we are able to count number of iterations in runtime
   LOOP->LPT_DECISION.TIMES + 1 times.  The transformation does this (with some
   extra care for case n < 0):

   for (i = 0; i < n; i++)
     body;

   ==>

   i = 0;
   mod = n % 4;

   switch (mod)
     {
       case 3:
         body; i++;
       case 2:
         body; i++;
       case 1:
         body; i++;
       case 0: ;
     }

   while (i < n)
     {
       body; i++;
       body; i++;
       body; i++;
       body; i++;
     }
   */
static void
unroll_loop_runtime_iterations (struct loops *loops, struct loop *loop)
{
  rtx old_niter, niter, init_code, branch_code, tmp;
  unsigned i, j, p;
  basic_block preheader, *body, *dom_bbs, swtch, ezc_swtch;
  unsigned n_dom_bbs;
  sbitmap wont_exit;
  int may_exit_copy;
  unsigned n_peel, n_remove_edges;
  edge *remove_edges, e;
  bool extra_zero_check, last_may_exit;
  unsigned max_unroll = loop->lpt_decision.times;
  struct niter_desc *desc = get_simple_loop_desc (loop);
  bool exit_at_end = loop_exit_at_end_p (loop);

  /* Remember blocks whose dominators will have to be updated.  */
  dom_bbs = xcalloc (n_basic_blocks, sizeof (basic_block));
  n_dom_bbs = 0;

  body = get_loop_body (loop);
  for (i = 0; i < loop->num_nodes; i++)
    {
      unsigned nldom;
      basic_block *ldom;

      nldom = get_dominated_by (CDI_DOMINATORS, body[i], &ldom);
      for (j = 0; j < nldom; j++)
	if (!flow_bb_inside_loop_p (loop, ldom[j]))
	  dom_bbs[n_dom_bbs++] = ldom[j];

      free (ldom);
    }
  free (body);

  if (!exit_at_end)
    {
      /* Leave exit in first copy (for explanation why see comment in
	 unroll_loop_constant_iterations).  */
      may_exit_copy = 0;
      n_peel = max_unroll - 1;
      extra_zero_check = true;
      last_may_exit = false;
    }
  else
    {
      /* Leave exit in last copy (for explanation why see comment in
	 unroll_loop_constant_iterations).  */
      may_exit_copy = max_unroll;
      n_peel = max_unroll;
      extra_zero_check = false;
      last_may_exit = true;
    }

  /* Get expression for number of iterations.  */
  start_sequence ();
  old_niter = niter = gen_reg_rtx (desc->mode);
  tmp = force_operand (copy_rtx (desc->niter_expr), niter);
  if (tmp != niter)
    emit_move_insn (niter, tmp);

  /* Count modulo by ANDing it with max_unroll; we use the fact that
     the number of unrollings is a power of two, and thus this is correct
     even if there is overflow in the computation.  */
  niter = expand_simple_binop (desc->mode, AND,
			       niter,
			       GEN_INT (max_unroll),
			       NULL_RTX, 0, OPTAB_LIB_WIDEN);

  init_code = get_insns ();
  end_sequence ();

  /* Precondition the loop.  */
  loop_split_edge_with (loop_preheader_edge (loop), init_code);

  remove_edges = xcalloc (max_unroll + n_peel + 1, sizeof (edge));
  n_remove_edges = 0;

  wont_exit = sbitmap_alloc (max_unroll + 2);

  /* Peel the first copy of loop body (almost always we must leave exit test
     here; the only exception is when we have extra zero check and the number
     of iterations is reliable.  Also record the place of (possible) extra
     zero check.  */
  sbitmap_zero (wont_exit);
  if (extra_zero_check
      && !desc->noloop_assumptions)
    SET_BIT (wont_exit, 1);
  ezc_swtch = loop_preheader_edge (loop)->src;
  if (!duplicate_loop_to_header_edge (loop, loop_preheader_edge (loop),
		loops, 1,
		wont_exit, desc->out_edge, remove_edges, &n_remove_edges,
		DLTHE_FLAG_UPDATE_FREQ))
    abort ();

  /* Record the place where switch will be built for preconditioning.  */
  swtch = loop_split_edge_with (loop_preheader_edge (loop),
				NULL_RTX);

  for (i = 0; i < n_peel; i++)
    {
      /* Peel the copy.  */
      sbitmap_zero (wont_exit);
      if (i != n_peel - 1 || !last_may_exit)
	SET_BIT (wont_exit, 1);
      if (!duplicate_loop_to_header_edge (loop, loop_preheader_edge (loop),
		loops, 1,
		wont_exit, desc->out_edge, remove_edges, &n_remove_edges,
		DLTHE_FLAG_UPDATE_FREQ))
	abort ();

      /* Create item for switch.  */
      j = n_peel - i - (extra_zero_check ? 0 : 1);
      p = REG_BR_PROB_BASE / (i + 2);

      preheader = loop_split_edge_with (loop_preheader_edge (loop), NULL_RTX);
      branch_code = compare_and_jump_seq (copy_rtx (niter), GEN_INT (j), EQ,
					  block_label (preheader), p, NULL_RTX);

      swtch = loop_split_edge_with (swtch->pred, branch_code);
      set_immediate_dominator (CDI_DOMINATORS, preheader, swtch);
      swtch->succ->probability = REG_BR_PROB_BASE - p;
      e = make_edge (swtch, preheader,
		     swtch->succ->flags & EDGE_IRREDUCIBLE_LOOP);
      e->probability = p;
    }

  if (extra_zero_check)
    {
      /* Add branch for zero iterations.  */
      p = REG_BR_PROB_BASE / (max_unroll + 1);
      swtch = ezc_swtch;
      preheader = loop_split_edge_with (loop_preheader_edge (loop), NULL_RTX);
      branch_code = compare_and_jump_seq (copy_rtx (niter), const0_rtx, EQ,
					  block_label (preheader), p, NULL_RTX);

      swtch = loop_split_edge_with (swtch->succ, branch_code);
      set_immediate_dominator (CDI_DOMINATORS, preheader, swtch);
      swtch->succ->probability = REG_BR_PROB_BASE - p;
      e = make_edge (swtch, preheader,
		     swtch->succ->flags & EDGE_IRREDUCIBLE_LOOP);
      e->probability = p;
    }

  /* Recount dominators for outer blocks.  */
  iterate_fix_dominators (CDI_DOMINATORS, dom_bbs, n_dom_bbs);

  /* And unroll loop.  */

  sbitmap_ones (wont_exit);
  RESET_BIT (wont_exit, may_exit_copy);

  if (!duplicate_loop_to_header_edge (loop, loop_latch_edge (loop),
		loops, max_unroll,
		wont_exit, desc->out_edge, remove_edges, &n_remove_edges,
		DLTHE_FLAG_UPDATE_FREQ))
    abort ();

  free (wont_exit);

  if (exit_at_end)
    {
      basic_block exit_block = desc->in_edge->src->rbi->copy;
      /* Find a new in and out edge; they are in the last copy we have made.  */
      
      if (exit_block->succ->dest == desc->out_edge->dest)
	{
	  desc->out_edge = exit_block->succ;
	  desc->in_edge = exit_block->succ->succ_next;
	}
      else
	{
	  desc->out_edge = exit_block->succ->succ_next;
	  desc->in_edge = exit_block->succ;
	}
    }

  /* Remove the edges.  */
  for (i = 0; i < n_remove_edges; i++)
    remove_path (loops, remove_edges[i]);
  free (remove_edges);

  /* We must be careful when updating the number of iterations due to
     preconditioning and the fact that the value must be valid at entry
     of the loop.  After passing through the above code, we see that
     the correct new number of iterations is this:  */
  if (desc->const_iter)
    abort ();
  desc->niter_expr =
    simplify_gen_binary (UDIV, desc->mode, old_niter, GEN_INT (max_unroll + 1));
  desc->niter_max /= max_unroll + 1;
  if (exit_at_end)
    {
      desc->niter_expr =
	simplify_gen_binary (MINUS, desc->mode, desc->niter_expr, const1_rtx);
      desc->noloop_assumptions = NULL_RTX;
      desc->niter_max--;
    }

  if (dump_file)
    fprintf (dump_file,
	     ";; Unrolled loop %d times, counting # of iterations "
	     "in runtime, %i insns\n",
	     max_unroll, num_loop_insns (loop));
}

/* Decide whether to simply peel LOOP and how much.  */
static void
decide_peel_simple (struct loop *loop, int flags)
{
  unsigned npeel;
  struct niter_desc *desc;

  if (!(flags & UAP_PEEL))
    {
      /* We were not asked to, just return back silently.  */
      return;
    }

  if (dump_file)
    fprintf (dump_file, "\n;; Considering simply peeling loop\n");

  /* npeel = number of iterations to peel.  */
  npeel = PARAM_VALUE (PARAM_MAX_PEELED_INSNS) / loop->ninsns;
  if (npeel > (unsigned) PARAM_VALUE (PARAM_MAX_PEEL_TIMES))
    npeel = PARAM_VALUE (PARAM_MAX_PEEL_TIMES);

  /* Skip big loops.  */
  if (!npeel)
    {
      if (dump_file)
	fprintf (dump_file, ";; Not considering loop, is too big\n");
      return;
    }

  /* Check for simple loops.  */
  desc = get_simple_loop_desc (loop);

  /* Check number of iterations.  */
  if (desc->simple_p && !desc->assumptions && desc->const_iter)
    {
      if (dump_file)
	fprintf (dump_file, ";; Loop iterates constant times\n");
      return;
    }

  /* Do not simply peel loops with branches inside -- it increases number
     of mispredicts.  */
  if (num_loop_branches (loop) > 1)
    {
      if (dump_file)
	fprintf (dump_file, ";; Not peeling, contains branches\n");
      return;
    }

  if (loop->header->count)
    {
      unsigned niter = expected_loop_iterations (loop);
      if (niter + 1 > npeel)
	{
	  if (dump_file)
	    {
	      fprintf (dump_file, ";; Not peeling loop, rolls too much (");
	      fprintf (dump_file, HOST_WIDEST_INT_PRINT_DEC,
		       (HOST_WIDEST_INT) (niter + 1));
	      fprintf (dump_file, " iterations > %d [maximum peelings])\n",
		       npeel);
	    }
	  return;
	}
      npeel = niter + 1;
    }
  else
    {
      /* For now we have no good heuristics to decide whether loop peeling
         will be effective, so disable it.  */
      if (dump_file)
	fprintf (dump_file,
		 ";; Not peeling loop, no evidence it will be profitable\n");
      return;
    }

  /* Success.  */
  loop->lpt_decision.decision = LPT_PEEL_SIMPLE;
  loop->lpt_decision.times = npeel;
      
  if (dump_file)
    fprintf (dump_file, ";; Decided to simply peel the loop, %d times.\n",
	     loop->lpt_decision.times);
}

/* Peel a LOOP LOOP->LPT_DECISION.TIMES times.  The transformation:
   while (cond)
     body;

   ==>

   if (!cond) goto end;
   body;
   if (!cond) goto end;
   body;
   while (cond)
     body;
   end: ;
   */
static void
peel_loop_simple (struct loops *loops, struct loop *loop)
{
  sbitmap wont_exit;
  unsigned npeel = loop->lpt_decision.times;
  struct niter_desc *desc = get_simple_loop_desc (loop);

  wont_exit = sbitmap_alloc (npeel + 1);
  sbitmap_zero (wont_exit);

  if (!duplicate_loop_to_header_edge (loop, loop_preheader_edge (loop),
		loops, npeel, wont_exit, NULL, NULL, NULL,
		DLTHE_FLAG_UPDATE_FREQ))
    abort ();

  free (wont_exit);

  if (desc->simple_p)
    {
      if (desc->const_iter)
	{
	  desc->niter -= npeel;
	  desc->niter_expr = GEN_INT (desc->niter);
	  desc->noloop_assumptions = NULL_RTX;
	}
      else
	{
	  /* We cannot just update niter_expr, as its value might be clobbered
	     inside loop.  We could handle this by counting the number into
	     temporary just like we do in runtime unrolling, but it does not
	     seem worthwhile.  */
	  free_simple_loop_desc (loop);
	}
    }
  if (dump_file)
    fprintf (dump_file, ";; Peeling loop %d times\n", npeel);
}

/* Decide whether to unroll LOOP stupidly and how much.  */
static void
decide_unroll_stupid (struct loop *loop, int flags)
{
  unsigned nunroll, nunroll_by_av, i;
  struct niter_desc *desc;

  if (!(flags & UAP_UNROLL_ALL))
    {
      /* We were not asked to, just return back silently.  */
      return;
    }

  if (dump_file)
    fprintf (dump_file, "\n;; Considering unrolling loop stupidly\n");

  /* nunroll = total number of copies of the original loop body in
     unrolled loop (i.e. if it is 2, we have to duplicate loop body once.  */
  nunroll = PARAM_VALUE (PARAM_MAX_UNROLLED_INSNS) / loop->ninsns;
  nunroll_by_av
    = PARAM_VALUE (PARAM_MAX_AVERAGE_UNROLLED_INSNS) / loop->av_ninsns;
  if (nunroll > nunroll_by_av)
    nunroll = nunroll_by_av;
  if (nunroll > (unsigned) PARAM_VALUE (PARAM_MAX_UNROLL_TIMES))
    nunroll = PARAM_VALUE (PARAM_MAX_UNROLL_TIMES);

  /* Skip big loops.  */
  if (nunroll <= 1)
    {
      if (dump_file)
	fprintf (dump_file, ";; Not considering loop, is too big\n");
      return;
    }

  /* Check for simple loops.  */
  desc = get_simple_loop_desc (loop);

  /* Check simpleness.  */
  if (desc->simple_p && !desc->assumptions)
    {
      if (dump_file)
	fprintf (dump_file, ";; The loop is simple\n");
      return;
    }

  /* Do not unroll loops with branches inside -- it increases number
     of mispredicts.  */
  if (num_loop_branches (loop) > 1)
    {
      if (dump_file)
	fprintf (dump_file, ";; Not unrolling, contains branches\n");
      return;
    }

  /* If we have profile feedback, check whether the loop rolls.  */
  if (loop->header->count
      && expected_loop_iterations (loop) < 2 * nunroll)
    {
      if (dump_file)
	fprintf (dump_file, ";; Not unrolling loop, doesn't roll\n");
      return;
    }

  /* Success.  Now force nunroll to be power of 2, as it seems that this
     improves results (partially because of better alignments, partially
     because of some dark magic).  */
  for (i = 1; 2 * i <= nunroll; i *= 2)
    continue;

  loop->lpt_decision.decision = LPT_UNROLL_STUPID;
  loop->lpt_decision.times = i - 1;

  if (dump_file)
    fprintf (dump_file,
	     ";; Decided to unroll the loop stupidly, %d times.\n",
	     loop->lpt_decision.times);
}

/* Unroll a LOOP LOOP->LPT_DECISION.TIMES times.  The transformation:
   while (cond)
     body;

   ==>

   while (cond)
     {
       body;
       if (!cond) break;
       body;
       if (!cond) break;
       body;
       if (!cond) break;
       body;
     }
   */
static void
unroll_loop_stupid (struct loops *loops, struct loop *loop)
{
  sbitmap wont_exit;
  unsigned nunroll = loop->lpt_decision.times;
  struct niter_desc *desc = get_simple_loop_desc (loop);

  wont_exit = sbitmap_alloc (nunroll + 1);
  sbitmap_zero (wont_exit);

  if (!duplicate_loop_to_header_edge (loop, loop_latch_edge (loop),
		loops, nunroll, wont_exit, NULL, NULL, NULL,
		DLTHE_FLAG_UPDATE_FREQ))
    abort ();

  free (wont_exit);

  if (desc->simple_p)
    {
      /* We indeed may get here provided that there are nontrivial assumptions
	 for a loop to be really simple.  We could update the counts, but the
	 problem is that we are unable to decide which exit will be taken
	 (not really true in case the number of iterations is constant,
	 but noone will do anything with this information, so we do not
	 worry about it).  */
      desc->simple_p = false;
    }

  if (dump_file)
    fprintf (dump_file, ";; Unrolled loop %d times, %i insns\n",
	     nunroll, num_loop_insns (loop));
}