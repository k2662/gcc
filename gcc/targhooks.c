/* Default target hook functions.
   Copyright (C) 2003, 2004 Free Software Foundation, Inc.

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

/* The migration of target macros to target hooks works as follows:

   1. Create a target hook that uses the existing target macros to
      implement the same functionality.

   2. Convert all the MI files to use the hook instead of the macro.

   3. Repeat for a majority of the remaining target macros.  This will
      take some time.

   4. Tell target maintainers to start migrating.

   5. Eventually convert the backends to override the hook instead of
      defining the macros.  This will take some time too.

   6. TBD when, poison the macros.  Unmigrated targets will break at
      this point.

   Note that we expect steps 1-3 to be done by the people that
   understand what the MI does with each macro, and step 5 to be done
   by the target maintainers for their respective targets.

   Note that steps 1 and 2 don't have to be done together, but no
   target can override the new hook until step 2 is complete for it.

   Once the macros are poisoned, we will revert to the old migration
   rules - migrate the macro, callers, and targets all at once.  This
   comment can thus be removed at that point.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "machmode.h"
#include "rtl.h"
#include "tree.h"
#include "expr.h"
#include "output.h"
#include "toplev.h"
#include "function.h"
#include "target.h"
#include "tm_p.h"
#include "target-def.h"

void
default_external_libcall (rtx fun ATTRIBUTE_UNUSED)
{
#ifdef ASM_OUTPUT_EXTERNAL_LIBCALL
  ASM_OUTPUT_EXTERNAL_LIBCALL(asm_out_file, fun);
#endif
}

enum machine_mode
default_cc_modes_compatible (enum machine_mode m1, enum machine_mode m2)
{
  if (m1 == m2)
    return m1;
  return VOIDmode;
}

bool
default_return_in_memory (tree type,
			  tree fntype ATTRIBUTE_UNUSED)
{
#ifndef RETURN_IN_MEMORY
  return (TYPE_MODE (type) == BLKmode);
#else
  return RETURN_IN_MEMORY (type);
#endif
}

rtx
default_expand_builtin_saveregs (void)
{
  error ("__builtin_saveregs not supported by this target");
  return const0_rtx;
}

void
default_setup_incoming_varargs (CUMULATIVE_ARGS *ca ATTRIBUTE_UNUSED,
				enum machine_mode mode ATTRIBUTE_UNUSED,
				tree type ATTRIBUTE_UNUSED,
				int *pretend_arg_size ATTRIBUTE_UNUSED,
				int second_time ATTRIBUTE_UNUSED)
{
}

/* The default implementation of TARGET_BUILTIN_SETJMP_FRAME_VALUE.  */

rtx
default_builtin_setjmp_frame_value (void)
{
  return virtual_stack_vars_rtx;
}

/* Generic hook that takes a CUMULATIVE_ARGS pointer and returns true.  */

bool
hook_bool_CUMULATIVE_ARGS_false (CUMULATIVE_ARGS *ca ATTRIBUTE_UNUSED)
{
  return false;
}

bool
default_pretend_outgoing_varargs_named(CUMULATIVE_ARGS *ca ATTRIBUTE_UNUSED)
{
  return (targetm.calls.setup_incoming_varargs != default_setup_incoming_varargs);
}

bool
default_skip_vec_args(tree type ATTRIBUTE_UNUSED,
		      int pass ATTRIBUTE_UNUSED,
		      int* last_pass ATTRIBUTE_UNUSED)
{
  return false;
}

/* Generic hook that takes a CUMULATIVE_ARGS pointer and returns true.  */

bool
hook_bool_CUMULATIVE_ARGS_true (CUMULATIVE_ARGS * a ATTRIBUTE_UNUSED)
{
  return true;
}