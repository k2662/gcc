/* Control and data flow functions for trees.
   Copyright 2001, 2002, 2003, 2004 Free Software Foundation, Inc.
   Contributed by Diego Novillo <dnovillo@redhat.com>

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

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
#include "expr.h"
#include "diagnostic.h"
#include "basic-block.h"
#include "flags.h"
#include "tree-flow.h"
#include "tree-dump.h"
#include "timevar.h"
#include "function.h"
#include "langhooks.h"
#include "toplev.h"
#include "flags.h"
#include "cgraph.h"
#include "tree-inline.h"
#include "tree-mudflap.h"
#include "tree-pass.h"
#include "tree-alias-common.h"
#include "ggc.h"
#include "cgraph.h"


/* Global variables used to communicate with passes.  */
int dump_flags;
bitmap vars_to_rename;
bool in_gimple_form;

/* The root of the compilation pass tree, once constructed.  */
static struct tree_opt_pass *all_passes;

/* Pass: gimplify the function if it's not been done.  */

static void
execute_gimple (void)
{
  /* We have this test here rather than as the gate because we always
     want to dump the original gimplified function.  */
  if (!lang_hooks.gimple_before_inlining)
    gimplify_function_tree (current_function_decl);
}

static struct tree_opt_pass pass_gimple = 
{
  "gimple",				/* name */
  NULL,					/* gate */
  execute_gimple,			/* execute */
  NULL,					/* sub */
  NULL,					/* next */
  0,					/* static_pass_number */
  0,					/* tv_id */
  0,					/* properties_required */
  PROP_gimple_any,			/* properties_provided */
  0,					/* properties_destroyed */
  0,					/* todo_flags_start */
  TODO_dump_func			/* todo_flags_finish */
};

/* Pass: replace the outermost BIND_EXPR.  We removed all of them while
   optimizing, but the tree->rtl expander requires it.  */

static void
execute_rebuild_bind (void)
{
  DECL_SAVED_TREE (current_function_decl)
    = build (BIND_EXPR, void_type_node, NULL_TREE,
	     DECL_SAVED_TREE (current_function_decl), NULL_TREE);
}

static struct tree_opt_pass pass_rebuild_bind = 
{
  NULL,					/* name */
  NULL,					/* gate */
  execute_rebuild_bind,			/* execute */
  NULL,					/* sub */
  NULL,					/* next */
  0,					/* static_pass_number */
  0,					/* tv_id */
  0,					/* properties_required */
  0,					/* properties_provided */
  0,					/* properties_destroyed */
  0,					/* todo_flags_start */
  0					/* todo_flags_finish */
};

/* Gate: execute, or not, all of the non-trivial optimizations.  */

static bool
gate_all_optimizations (void)
{
  return (optimize >= 1
	  /* Don't bother doing anything if the program has errors.  */
	  && !(errorcount || sorrycount));
}

static struct tree_opt_pass pass_all_optimizations =
{
  NULL,					/* name */
  gate_all_optimizations,		/* gate */
  NULL,					/* execute */
  NULL,					/* sub */
  NULL,					/* next */
  0,					/* static_pass_number */
  0,					/* tv_id */
  0,					/* properties_required */
  0,					/* properties_provided */
  0,					/* properties_destroyed */
  0,					/* todo_flags_start */
  0					/* todo_flags_finish */
};

/* Pass: do the actions required to finish with tree-ssa optimization
   passes.  */

static void
execute_del_cfg (void)
{
  basic_block bb;
  tree *chain;

  /* ??? This isn't the right place for this.  Worse, it got computed
     more or less at random in various passes.  */
  free_dominance_info (CDI_DOMINATORS);

  /* Emit gotos for implicit jumps.  */
  disband_implicit_edges ();

  /* Remove the ssa structures.  Do it here since this includes statement
     annotations that need to be intact during disband_implicit_edges.  */
  delete_tree_ssa ();

  /* Re-chain the statements from the blocks.  */
  chain = &DECL_SAVED_TREE (current_function_decl);
  *chain = alloc_stmt_list ();
  FOR_EACH_BB (bb)
    {
      append_to_statement_list_force (bb->stmt_list, chain);
    }

  /* And get rid of the cfg.  */
  delete_tree_cfg ();
}

static struct tree_opt_pass pass_del_cfg =
{
  NULL,					/* name */
  NULL,					/* gate */
  execute_del_cfg,			/* execute */
  NULL,					/* sub */
  NULL,					/* next */
  0,					/* static_pass_number */
  0,					/* tv_id */
  PROP_cfg,				/* properties_required */
  0,					/* properties_provided */
  PROP_cfg,				/* properties_destroyed */
  0,					/* todo_flags_start */
  0					/* todo_flags_finish */
};

/* Iterate over the pass tree allocating dump file numbers.  We want
   to do this depth first, and independent of whether the pass is
   enabled or not.  */

static void
register_one_dump_file (struct tree_opt_pass *pass)
{
  char *dot_name, *flag_name;
  char num[10];

  if (!pass->name)
    return;

  /* See below in dup_pass_1.  */
  num[0] = '\0';
  if (pass->static_pass_number)
    sprintf (num, "%d", ((int) pass->static_pass_number < 0
			 ? 1 : pass->static_pass_number));

  dot_name = concat (".", pass->name, num, NULL);
  flag_name = concat ("tree-", pass->name, num, NULL);

  pass->static_pass_number = dump_register (dot_name, flag_name);
}

static void 
register_dump_files (struct tree_opt_pass *pass)
{
  do
    {
      register_one_dump_file (pass);
      if (pass->sub)
	register_dump_files (pass->sub);
      pass = pass->next;
    }
  while (pass);
}

/* Duplicate a pass that's to be run more than once.  */

static struct tree_opt_pass *
dup_pass_1 (struct tree_opt_pass *pass)
{
  struct tree_opt_pass *new;

  new = xmalloc (sizeof (*new));
  memcpy (new, pass, sizeof (*new));

  /* Indicate to register_dump_files that this pass has duplicates,
     and so it should rename the dump file.  The first instance will
     be < 0, and be number of duplicates = -static_pass_number + 1.
     Subsequent instances will be > 0 and just the duplicate number.  */
  if (pass->name)
    {
      int n, p = pass->static_pass_number;
	
      if (p)
	n = -(--p) + 1;
      else
	n = 2, p = -1;

      pass->static_pass_number = p;
      new->static_pass_number = n;
    }

  return new;
}

/* Construct the pass tree.  */

void
init_tree_optimization_passes (void)
{
  struct tree_opt_pass **p;

#define NEXT_PASS(PASS) (*p = &PASS, p = &(*p)->next)
#define DUP_PASS(PASS)  (*dup_pass_1 (&PASS))

  p = &all_passes;
  NEXT_PASS (pass_gimple);
  NEXT_PASS (pass_remove_useless_stmts);
  NEXT_PASS (pass_mudflap_1);
  NEXT_PASS (pass_lower_cf);
  NEXT_PASS (pass_lower_eh);
  NEXT_PASS (pass_all_optimizations);
  NEXT_PASS (pass_mudflap_2);
  NEXT_PASS (pass_rebuild_bind);
  *p = NULL;

  p = &pass_all_optimizations.sub;
  NEXT_PASS (pass_build_cfg);
  NEXT_PASS (pass_tree_profile);
  NEXT_PASS (pass_referenced_vars);
  NEXT_PASS (pass_build_pta);
  NEXT_PASS (pass_build_ssa);
  NEXT_PASS (pass_rename_ssa_copies);
  NEXT_PASS (pass_early_warn_uninitialized);
  NEXT_PASS (pass_dce);
  NEXT_PASS (pass_return);
  NEXT_PASS (pass_dominator);
  NEXT_PASS (pass_redundant_phi);
  NEXT_PASS (DUP_PASS (pass_dce));
  NEXT_PASS (pass_forwprop);
  NEXT_PASS (pass_phiopt);
  NEXT_PASS (pass_may_alias);
  NEXT_PASS (pass_tail_recursion);
  NEXT_PASS (pass_ch);
  NEXT_PASS (pass_del_pta);
  NEXT_PASS (pass_profile);
  NEXT_PASS (pass_lower_complex);
  NEXT_PASS (DUP_PASS (pass_return));
  NEXT_PASS (pass_sra);
  NEXT_PASS (DUP_PASS (pass_rename_ssa_copies));
  NEXT_PASS (DUP_PASS (pass_dominator));
  NEXT_PASS (DUP_PASS (pass_redundant_phi));
  NEXT_PASS (DUP_PASS (pass_dce));
  NEXT_PASS (pass_dse);
  NEXT_PASS (DUP_PASS (pass_forwprop));
  NEXT_PASS (DUP_PASS (pass_phiopt));
  NEXT_PASS (pass_ccp);
  NEXT_PASS (DUP_PASS (pass_redundant_phi));
  NEXT_PASS (pass_fold_builtins);
  NEXT_PASS (pass_split_crit_edges);
  NEXT_PASS (pass_pre);
  NEXT_PASS (pass_scev);
  NEXT_PASS (pass_loop);
  NEXT_PASS (DUP_PASS (pass_dominator));
  NEXT_PASS (DUP_PASS (pass_redundant_phi));
  NEXT_PASS (pass_cd_dce);
  NEXT_PASS (DUP_PASS (pass_dse));
  NEXT_PASS (DUP_PASS (pass_forwprop));
  NEXT_PASS (DUP_PASS (pass_phiopt));
  NEXT_PASS (pass_tail_calls);
  NEXT_PASS (pass_late_warn_uninitialized);
  NEXT_PASS (pass_warn_function_return);
  NEXT_PASS (pass_del_ssa);
  NEXT_PASS (pass_nrv);
  NEXT_PASS (pass_remove_useless_vars);
  NEXT_PASS (pass_del_cfg);
  *p = NULL;

  p = &pass_scev.sub;
  NEXT_PASS (pass_scev_init);
  NEXT_PASS (pass_scev_anal);
  NEXT_PASS (pass_scev_depend);
  NEXT_PASS (pass_scev_elim_checks);
  NEXT_PASS (pass_scev_iv_canon);
  NEXT_PASS (pass_scev_linear_transform);
  NEXT_PASS (pass_ddg);
  NEXT_PASS (pass_scev_vectorize);
  NEXT_PASS (pass_delete_ddg);
  NEXT_PASS (pass_scev_done);
  *p = NULL;

#undef NEXT_PASS
#undef DUP_PASS

  /* Register the passes with the tree dump code.  */
  register_dump_files (all_passes);
}

static void execute_pass_list (struct tree_opt_pass *);

static unsigned int current_properties;
static unsigned int last_verified;

static void
execute_todo (unsigned int flags)
{
  if (flags & TODO_rename_vars)
    {
      rewrite_into_ssa (false);
      bitmap_clear (vars_to_rename);
    }

  if ((flags & TODO_dump_func) && dump_file)
    dump_function_to_file (current_function_decl,
			   dump_file, dump_flags);

  if (flags & TODO_ggc_collect)
    ggc_collect ();

#ifdef ENABLE_CHECKING
  if (flags & TODO_verify_ssa)
    verify_ssa ();
  if (flags & TODO_verify_flow)
    verify_flow_info ();
  if (flags & TODO_verify_stmts)
    verify_stmts ();
#endif
}

static bool
execute_one_pass (struct tree_opt_pass *pass)
{
  unsigned int todo; 

  /* See if we're supposed to run this pass.  */
  if (pass->gate && !pass->gate ())
    return false;

  /* Verify that all required properties are present.  */
  if (pass->properties_required & ~current_properties)
    abort ();

  /* Run pre-pass verification.  */
  todo = pass->todo_flags_start & ~last_verified;
  if (todo)
    execute_todo (todo);

  /* If a dump file name is present, open it if enabled.  */
  if (pass->static_pass_number)
    {
      dump_file = dump_begin (pass->static_pass_number, &dump_flags);
      if (dump_file)
	{
	  const char *dname, *aname;
	  dname = lang_hooks.decl_printable_name (current_function_decl, 2);
	  aname = (IDENTIFIER_POINTER
		   (DECL_ASSEMBLER_NAME (current_function_decl)));
	  fprintf (dump_file, "\n;; Function %s (%s)\n\n", dname, aname);
	}
    }

  /* If a timevar is present, start it.  */
  if (pass->tv_id)
    timevar_push (pass->tv_id);

  /* Do it!  */
  if (pass->execute)
    pass->execute ();

  /* Run post-pass cleanup and verification.  */
  todo = pass->todo_flags_finish;
  last_verified = todo & TODO_verify_all;
  if (todo)
    execute_todo (todo);

  /* Update properties.  */
  current_properties &= ~pass->properties_destroyed;
  current_properties |= pass->properties_provided;

  /* Close down timevar and dump file.  */
  if (pass->tv_id)
    timevar_pop (pass->tv_id);
  if (dump_file)
    {
      dump_end (pass->static_pass_number, dump_file);
      dump_file = NULL;
    }

  return true;
}

static void
execute_pass_list (struct tree_opt_pass *pass)
{
  do
    {
      if (execute_one_pass (pass) && pass->sub)
	execute_pass_list (pass->sub);
      pass = pass->next;
    }
  while (pass);
}


/* Called to move the SAVE_EXPRs for parameter declarations in a
   nested function into the nested function.  DATA is really the
   nested FUNCTION_DECL.  */

static tree
set_save_expr_context (tree *tp,
		       int *walk_subtrees,
		       void *data)
{
  if (TREE_CODE (*tp) == SAVE_EXPR && !SAVE_EXPR_CONTEXT (*tp))
    SAVE_EXPR_CONTEXT (*tp) = (tree) data;
  /* Do not walk back into the SAVE_EXPR_CONTEXT; that will cause
     circularity.  */
  else if (DECL_P (*tp))
    *walk_subtrees = 0;

  return NULL;
}

/* For functions-as-trees languages, this performs all optimization and
   compilation for FNDECL.  */

void
tree_rest_of_compilation (tree fndecl, bool nested_p)
{
  location_t saved_loc;
  struct cgraph_node *saved_node = NULL, *node;

  timevar_push (TV_EXPAND);

  if (flag_unit_at_a_time && !cgraph_global_info_ready)
    abort ();

  /* Initialize the RTL code for the function.  */
  current_function_decl = fndecl;
  saved_loc = input_location;
  input_location = DECL_SOURCE_LOCATION (fndecl);
  init_function_start (fndecl);

  /* This function is being processed in whole-function mode.  */
  cfun->x_whole_function_mode_p = 1;

  /* Even though we're inside a function body, we still don't want to
     call expand_expr to calculate the size of a variable-sized array.
     We haven't necessarily assigned RTL to all variables yet, so it's
     not safe to try to expand expressions involving them.  */
  immediate_size_expand = 0;
  cfun->x_dont_save_pending_sizes_p = 1;

  node = cgraph_node (fndecl);

  /* We might need the body of this function so that we can expand
     it inline somewhere else.  This means not lowering some constructs
     such as exception handling.  */
  if (cgraph_preserve_function_body_p (fndecl))
    {
      if (!flag_unit_at_a_time)
	{
	  struct cgraph_edge *e;

	  saved_node = cgraph_clone_node (node);
	  for (e = saved_node->callees; e; e = e->next_callee)
	    if (!e->inline_failed)
	      cgraph_clone_inlined_nodes (e, true);
	}
      cfun->saved_tree = save_body (fndecl, &cfun->saved_args);
    }

  if (flag_inline_trees)
    {
      struct cgraph_edge *e;
      for (e = node->callees; e; e = e->next_callee)
	if (!e->inline_failed || warn_inline)
	  break;
      if (e)
	{
	  timevar_push (TV_INTEGRATION);
	  optimize_inline_calls (fndecl);
	  timevar_pop (TV_INTEGRATION);
	}
    }

  if (!vars_to_rename)
    vars_to_rename = BITMAP_XMALLOC ();

  /* Note that the folders should only create gimple expressions.
     This is a hack until the new folder is ready.  */
  in_gimple_form = true;

  /* Perform all tree transforms and optimizations.  */
  execute_pass_list (all_passes);

  /* Note that the folders can create non-gimple expressions again.  */
  in_gimple_form = false;

  /* If the function has a variably modified type, there may be
     SAVE_EXPRs in the parameter types.  Their context must be set to
     refer to this function; they cannot be expanded in the containing
     function.  */
  if (decl_function_context (fndecl) == current_function_decl
      && variably_modified_type_p (TREE_TYPE (fndecl)))
    walk_tree (&TREE_TYPE (fndecl), set_save_expr_context, fndecl,
	       NULL);

  /* Expand the variables recorded during gimple lowering.  This must
     occur before the call to expand_function_start to ensure that
     all used variables are expanded before we expand anything on the
     PENDING_SIZES list.  */
  expand_used_vars ();

  /* Set up parameters and prepare for return, for the function.  */
  expand_function_start (fndecl, 0);

  /* If this function is `main', emit a call to `__main'
     to run global initializers, etc.  */
  if (DECL_NAME (fndecl)
      && MAIN_NAME_P (DECL_NAME (fndecl))
      && DECL_FILE_SCOPE_P (fndecl))
    expand_main_function ();

  /* Generate the RTL for this function.  */
  expand_expr_stmt_value (DECL_SAVED_TREE (fndecl), 0, 0);

  /* We hard-wired immediate_size_expand to zero above.
     expand_function_end will decrement this variable.  So, we set the
     variable to one here, so that after the decrement it will remain
     zero.  */
  immediate_size_expand = 1;

  /* Make sure the locus is set to the end of the function, so that 
     epilogue line numbers and warnings are set properly.  */
  if (cfun->function_end_locus.file)
    input_location = cfun->function_end_locus;

  /* The following insns belong to the top scope.  */
  record_block_change (DECL_INITIAL (current_function_decl));
  
  /* Generate rtl for function exit.  */
  expand_function_end ();

  /* If this is a nested function, protect the local variables in the stack
     above us from being collected while we're compiling this function.  */
  if (nested_p)
    ggc_push_context ();

  /* There's no need to defer outputting this function any more; we
     know we want to output it.  */
  DECL_DEFER_OUTPUT (fndecl) = 0;

  /* Run the optimizers and output the assembler code for this function.  */
  rest_of_compilation (fndecl);

  /* Restore original body if still needed.  */
  if (cfun->saved_tree)
    {
      DECL_SAVED_TREE (fndecl) = cfun->saved_tree;
      DECL_ARGUMENTS (fndecl) = cfun->saved_args;

      /* When not in unit-at-a-time mode, we must preserve out of line copy
	 representing node before inlining.  Restore original outgoing edges
	 using clone we created earlier.  */
      if (!flag_unit_at_a_time)
	{
	  struct cgraph_edge *e;
	  while (node->callees)
	    cgraph_remove_edge (node->callees);
	  node->callees = saved_node->callees;
	  saved_node->callees = NULL;
	  for (e = saved_node->callees; e; e = e->next_callee)
	    e->caller = node;
	  cgraph_remove_node (saved_node);
	}
    }
  else
    DECL_SAVED_TREE (fndecl) = NULL;
  cfun = 0;

  /* If requested, warn about function definitions where the function will
     return a value (usually of some struct or union type) which itself will
     take up a lot of stack space.  */
  if (warn_larger_than && !DECL_EXTERNAL (fndecl) && TREE_TYPE (fndecl))
    {
      tree ret_type = TREE_TYPE (TREE_TYPE (fndecl));

      if (ret_type && TYPE_SIZE_UNIT (ret_type)
	  && TREE_CODE (TYPE_SIZE_UNIT (ret_type)) == INTEGER_CST
	  && 0 < compare_tree_int (TYPE_SIZE_UNIT (ret_type),
				   larger_than_size))
	{
	  unsigned int size_as_int
	    = TREE_INT_CST_LOW (TYPE_SIZE_UNIT (ret_type));

	  if (compare_tree_int (TYPE_SIZE_UNIT (ret_type), size_as_int) == 0)
	    warning ("%Jsize of return value of '%D' is %u bytes",
                     fndecl, fndecl, size_as_int);
	  else
	    warning ("%Jsize of return value of '%D' is larger than %wd bytes",
                     fndecl, fndecl, larger_than_size);
	}
    }

  if (!nested_p && !flag_inline_trees)
    {
      DECL_SAVED_TREE (fndecl) = NULL;
      if (DECL_STRUCT_FUNCTION (fndecl) == 0
	  && !cgraph_node (fndecl)->origin)
	{
	  /* Stop pointing to the local nodes about to be freed.
	     But DECL_INITIAL must remain nonzero so we know this
	     was an actual function definition.
	     For a nested function, this is done in c_pop_function_context.
	     If rest_of_compilation set this to 0, leave it 0.  */
	  if (DECL_INITIAL (fndecl) != 0)
	    DECL_INITIAL (fndecl) = error_mark_node;
	}
    }

  input_location = saved_loc;

  ggc_collect ();

  /* Undo the GC context switch.  */
  if (nested_p)
    ggc_pop_context ();
  timevar_pop (TV_EXPAND);
}