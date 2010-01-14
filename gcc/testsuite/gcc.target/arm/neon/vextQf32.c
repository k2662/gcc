/* APPLE LOCAL file v7 merge */
/* Test the `vextQf32' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vextQf32 (void)
{
  float32x4_t out_float32x4_t;
  float32x4_t arg0_float32x4_t;
  float32x4_t arg1_float32x4_t;

  out_float32x4_t = vextq_f32 (arg0_float32x4_t, arg1_float32x4_t, 0);
}

/* { dg-final { scan-assembler "vext\.32\[ 	\]+\[qQ\]\[0-9\]+, \[qQ\]\[0-9\]+, \[qQ\]\[0-9\]+, #\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */
/* { dg-final { cleanup-saved-temps } } */
