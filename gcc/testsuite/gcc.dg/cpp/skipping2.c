/* Copyright (C) 2001 Free Software Foundation, Inc.  */

/* { dg-do preprocess } */

/* Tests that excess tokens in skipped conditional blocks don't warn.  */

/* Source: Neil Booth, 25 Jul 2001.  */

/* APPLE LOCAL -Wextra-tokens required in Apple's compiler to elicit req'd warnings here */
/* { dg-options "-Wextra-tokens" } */

#if 0
#if foo
#else foo   /* { dg-bogus "extra tokens" "extra tokens in skipped block" } */
#endif foo  /* { dg-bogus "extra tokens" "extra tokens in skipped block" } */
#endif bar  /* { dg-warning "extra tokens" "tokens after #endif" } */
