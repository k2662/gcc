/* internal_debug.c

  TREE and RTL debugging macro functions.  What we do
  here is to instantiate each macro as a function *BY
  THE SAME NAME*.  Depends on the macro not being
  expanded when it is surrounded by parens.

  Note that this one includes the C++ stuff; it might make
  sense to separate that from the C-only stuff.  But I no
  longer think it makes sense to separate the RTL from the
  TREE stuff, nor to put those in print-rtl.c, print-tree.c,
  and cp/ptree.c.   */

#include "config.h"
#include "system.h"
#include "tree.h"
#include "rtl.h"
#include "basic-block.h"

/* get Objective-C stuff also */
#include "objc/objc-act.h"

#define fn_1(name,rt,pt)       rt (name) (pt a)           { return name(a); }
#define fn_2(name,rt,p1,p2)    rt (name) (p1 a,p2 b)      { return name(a,b); }
#define fn_3(name,rt,p1,p2,p3) rt (name) (p1 a,p2 b,p3 c) { return name(a,b,c); }


/* MACROS from tree.h (single-parameter ones) */

fn_1( AGGREGATE_TYPE_P, int, tree )
fn_1( BINFO_BASETYPES, tree, tree )
fn_1( BINFO_INHERITANCE_CHAIN, tree, tree )
fn_1( BINFO_OFFSET, tree, tree )
fn_1( BINFO_OFFSET_ZEROP, int, tree )
fn_1( BINFO_SIZE, tree, tree )
fn_1( BINFO_TYPE, tree, tree )
fn_1( BINFO_VIRTUALS, tree, tree )
fn_1( BINFO_VPTR_FIELD, tree, tree )
fn_1( BINFO_VTABLE, tree, tree )
fn_1( BLOCK_ABSTRACT, int, tree )
fn_1( BLOCK_ABSTRACT_ORIGIN, tree, tree )
fn_1( BLOCK_CHAIN, tree, tree )
fn_1( BLOCK_END_NOTE, rtx, tree )
fn_1( BLOCK_HANDLER_BLOCK, int, tree )
fn_1( BLOCK_LIVE_RANGE_END, int, tree )
fn_1( BLOCK_LIVE_RANGE_FLAG, int, tree )
fn_1( BLOCK_LIVE_RANGE_START, int, tree )
fn_1( BLOCK_LIVE_RANGE_VAR_FLAG, int, tree )
fn_1( BLOCK_SUBBLOCKS, tree, tree )
fn_1( BLOCK_SUPERCONTEXT, tree, tree )
fn_1( BLOCK_TYPE_TAGS, tree, tree )
fn_1( BLOCK_VARS, tree, tree )
fn_1( CALL_EXPR_RTL, rtx, tree )
fn_1( CONSTRUCTOR_ELTS, tree, tree )
fn_1( DECL_ABSTRACT, int, tree )
fn_1( DECL_ABSTRACT_ORIGIN, tree, tree )
fn_1( DECL_ALIGN, int, tree )
fn_1( DECL_ARG_TYPE, tree, tree )
fn_1( DECL_ARG_TYPE_AS_WRITTEN, tree, tree )
fn_1( DECL_ARGUMENTS, tree, tree )
fn_1( DECL_ARTIFICIAL, int, tree )
fn_1( DECL_ASSEMBLER_NAME, tree, tree )
fn_1( DECL_BIT_FIELD, int, tree )
fn_1( DECL_BIT_FIELD_TYPE, tree, tree )
fn_1( DECL_BUILT_IN, int, tree )
fn_1( DECL_BUILT_IN_NONANSI, int, tree )
fn_1( DECL_COMMON, int, tree )
#ifdef HAVE_COALESCED_SYMBOLS
fn_1( DECL_COALESCED, int, tree )
#endif
fn_1( DECL_CONTEXT, tree, tree )
fn_1( DECL_DEFER_OUTPUT, int, tree )
fn_1( DECL_DLLIMPORT, int, tree )
fn_1( DECL_ERROR_ISSUED, int, tree )
fn_1( DECL_EXTERNAL, int, tree )
fn_1( DECL_FCONTEXT, tree, tree )
fn_1( DECL_FIELD_BITPOS, tree, tree )
fn_1( DECL_FIELD_CONTEXT, tree, tree )
fn_1( DECL_FIELD_SIZE, int, tree )
fn_1( DECL_FRAME_SIZE, int, tree )
fn_1( DECL_FROM_INLINE, int, tree )
fn_1( DECL_FUNCTION_CODE, enum built_in_function, tree )
fn_1( DECL_IGNORED_P, int, tree )
fn_1( DECL_IN_SYSTEM_HEADER, int, tree )
fn_1( DECL_IN_TEXT_SECTION, int, tree )
fn_1( DECL_INCOMING_RTL, rtx, tree )
fn_1( DECL_INITIAL, tree, tree )
fn_1( DECL_INLINE, int, tree )
fn_1( DECL_LANG_FLAG_0, int, tree )
fn_1( DECL_LANG_FLAG_1, int, tree )
fn_1( DECL_LANG_FLAG_2, int, tree )
fn_1( DECL_LANG_FLAG_3, int, tree )
fn_1( DECL_LANG_FLAG_4, int, tree )
fn_1( DECL_LANG_FLAG_5, int, tree )
fn_1( DECL_LANG_FLAG_6, int, tree )
fn_1( DECL_LANG_FLAG_7, int, tree )
fn_1( DECL_LANG_SPECIFIC, struct lang_decl *, tree )
fn_1( DECL_LIVE_RANGE_RTL, rtx, tree )
fn_1( DECL_MACHINE_ATTRIBUTES, tree, tree )
fn_1( DECL_MODE, int, tree )
fn_1( DECL_NAME, tree, tree )
fn_1( DECL_NO_STATIC_CHAIN, int, tree )
fn_1( DECL_NON_ADDR_CONST_P, int, tree )
fn_1( DECL_NONLOCAL, int, tree )
fn_1( DECL_ONE_ONLY, int, tree )
fn_1( DECL_ORIGINAL_TYPE, tree, tree )
fn_1( DECL_PACKED, int, tree )
fn_1( DECL_PRIVATE_EXTERN, int, tree )
fn_1( DECL_QUALIFIER, tree, tree )
fn_1( DECL_REGISTER, int, tree )
fn_1( DECL_RELATIVE, int, tree )
fn_1( DECL_RESULT, tree, tree )
fn_1( DECL_RTL, rtx, tree )
fn_1( DECL_SAVED_INSNS, rtx, tree )
fn_1( DECL_SECTION_NAME, tree, tree )
fn_1( DECL_SIZE, tree, tree )
fn_1( DECL_SOURCE_FILE, char*, tree )
fn_1( DECL_SOURCE_LINE, int, tree )
fn_1( DECL_STATIC_CONSTRUCTOR, int, tree )
fn_1( DECL_STATIC_DESTRUCTOR, int, tree )
fn_1( DECL_STDCALL, int, tree )
fn_1( DECL_TOO_LATE, int, tree )
fn_1( DECL_TRANSPARENT_UNION, int, tree )
fn_1( DECL_UID, int, tree )
fn_1( DECL_VINDEX, tree, tree )
fn_1( DECL_VIRTUAL_P, int, tree )
fn_1( DECL_WEAK, int, tree )
fn_1( EXPR_WFL_COLNO, int, tree )
fn_1( EXPR_WFL_EMIT_LINE_NOTE, int, tree )
fn_1( EXPR_WFL_FILENAME, char*, tree )
fn_1( EXPR_WFL_FILENAME_NODE, tree, tree )
fn_1( EXPR_WFL_LINECOL, int, tree )
fn_1( EXPR_WFL_LINENO, int, tree )
fn_1( EXPR_WFL_NODE, tree, tree )
fn_1( FLOAT_TYPE_P, int, tree )
fn_1( IDENTIFIER_LENGTH, int, tree )
fn_1( IDENTIFIER_POINTER, char*, tree )
fn_1( INTEGRAL_TYPE_P, int, tree )
fn_1( IS_EXPR_CODE_CLASS, int, int )
fn_1( POINTER_TYPE_P, int, tree )
fn_1( RTL_EXPR_RTL, rtx, tree )
fn_1( RTL_EXPR_SEQUENCE, rtx, tree )
fn_1( SAVE_EXPR_CONTEXT, tree, tree )
fn_1( SAVE_EXPR_NOPLACEHOLDER, int, tree )
fn_1( SAVE_EXPR_RTL, rtx, tree )
fn_1( TREE_ADDRESSABLE, int, tree )
fn_1( TREE_ASM_WRITTEN, int, tree )
fn_1( TREE_CHAIN, tree, tree )
fn_1( TREE_CODE, enum tree_code, tree )
fn_1( TREE_CODE_CLASS, char, int )
fn_1( TREE_COMPLEXITY, int, tree )
fn_1( TREE_CONSTANT, int, tree )
fn_1( TREE_CONSTANT_OVERFLOW, int, tree )
fn_1( TREE_CST_RTL, rtx, tree )
fn_1( TREE_IMAGPART, tree, tree )
fn_1( TREE_INT_CST_HIGH, int, tree )
fn_1( TREE_INT_CST_LOW, int, tree )
fn_1( TREE_LANG_FLAG_0, int, tree )
fn_1( TREE_LANG_FLAG_1, int, tree )
fn_1( TREE_LANG_FLAG_2, int, tree )
fn_1( TREE_LANG_FLAG_3, int, tree )
fn_1( TREE_LANG_FLAG_4, int, tree )
fn_1( TREE_LANG_FLAG_5, int, tree )
fn_1( TREE_LANG_FLAG_6, int, tree )
fn_1( TREE_NO_UNUSED_WARNING, int, tree )
fn_1( TREE_OVERFLOW, int, tree )
fn_1( TREE_PERMANENT, int, tree )
fn_1( TREE_PRIVATE, int, tree )
fn_1( TREE_PROTECTED, int, tree )
fn_1( TREE_PUBLIC, int, tree )
fn_1( TREE_PURPOSE, tree, tree )
fn_1( TREE_RAISES, int, tree )
fn_1( TREE_READONLY, int, tree )
fn_1( TREE_REAL_CST, REAL_VALUE_TYPE, tree )
fn_1( TREE_REALPART, tree, tree )
fn_1( TREE_SIDE_EFFECTS, int, tree )
fn_1( TREE_STATIC, int, tree )
fn_1( TREE_STRING_LENGTH, int, tree )
fn_1( TREE_STRING_POINTER, char*, tree )
fn_1( TREE_SYMBOL_REFERENCED, int, tree )
fn_1( TREE_THIS_VOLATILE, int, tree )
fn_1( TREE_TYPE, tree, tree )
fn_1( TREE_UNSIGNED, int, tree )
fn_1( TREE_USED, int, tree )
fn_1( TREE_VALUE, tree, tree )
fn_1( TREE_VEC_LENGTH, int, tree )
fn_1( TREE_VIA_PRIVATE, int, tree )
fn_1( TREE_VIA_PROTECTED, int, tree )
fn_1( TREE_VIA_PUBLIC, int, tree )
fn_1( TREE_VIA_VIRTUAL, int, tree )
fn_1( TYPE_ALIAS_SET, int, tree )
fn_1( TYPE_ALIAS_SET_KNOWN_P, int, tree )
fn_1( TYPE_ALIGN, int, tree )
fn_1( TYPE_ARG_TYPES, tree, tree )
fn_1( TYPE_ARRAY_MAX_SIZE, tree, tree )
fn_1( TYPE_ATTRIBUTES, tree, tree )
fn_1( TYPE_BINFO, tree, tree )
fn_1( TYPE_BINFO_BASETYPES, tree, tree )
fn_1( TYPE_BINFO_OFFSET, tree, tree )
fn_1( TYPE_BINFO_SIZE, tree, tree )
fn_1( TYPE_BINFO_VIRTUALS, tree, tree )
fn_1( TYPE_BINFO_VTABLE, tree, tree )
fn_1( TYPE_CONTEXT, tree, tree )
fn_1( TYPE_DECL_SUPPRESS_DEBUG, int, tree )
fn_1( TYPE_DOMAIN, tree, tree )
fn_1( TYPE_FIELDS, tree, tree )
fn_1( TYPE_LANG_FLAG_0, int, tree )
fn_1( TYPE_LANG_FLAG_1, int, tree )
fn_1( TYPE_LANG_FLAG_2, int, tree )
fn_1( TYPE_LANG_FLAG_3, int, tree )
fn_1( TYPE_LANG_FLAG_4, int, tree )
fn_1( TYPE_LANG_FLAG_5, int, tree )
fn_1( TYPE_LANG_FLAG_6, int, tree )
fn_1( TYPE_LANG_SPECIFIC, struct lang_type*, tree )
fn_1( TYPE_MAIN_VARIANT, tree, tree )
fn_1( TYPE_MAX_VALUE, tree, tree )
fn_1( TYPE_METHOD_BASETYPE, tree, tree )
fn_1( TYPE_METHODS, tree, tree )
fn_1( TYPE_MIN_VALUE, tree, tree )
fn_1( TYPE_MODE, int, tree )
fn_1( TYPE_NAME, tree, tree )
fn_1( TYPE_NEEDS_CONSTRUCTING, int, tree )
fn_1( TYPE_NEXT_VARIANT, tree, tree )
fn_1( TYPE_NO_FORCE_BLK, int, tree )
fn_1( TYPE_NONCOPIED_PARTS, tree, tree )
fn_1( TYPE_OBSTACK, struct obstack *, tree )
fn_1( TYPE_OFFSET_BASETYPE, tree, tree )
fn_1( TYPE_P, int, tree )
fn_1( TYPE_PACKED, int, tree )
fn_1( TYPE_POINTER_TO, tree, tree )
fn_1( TYPE_PRECISION, int, tree )
fn_1( TYPE_QUALS, int, tree )
fn_1( TYPE_READONLY, int, tree )
fn_1( TYPE_REFERENCE_TO, tree, tree )
fn_1( TYPE_SIZE, tree, tree )
fn_1( TYPE_SIZE_UNIT, tree, tree )
fn_1( TYPE_STDCALL, int, tree )
fn_1( TYPE_STRING_FLAG, int, tree )
fn_1( TYPE_STUB_DECL, tree, tree )
fn_1( TYPE_SYMTAB_ADDRESS, int, tree )
fn_1( TYPE_SYMTAB_POINTER, char*, tree )
fn_1( TYPE_TRANSPARENT_UNION, int, tree )
fn_1( TYPE_UID, int, tree )
fn_1( TYPE_VALUES, tree, tree )
fn_1( TYPE_VFIELD, tree, tree )
fn_1( TYPE_VOLATILE, int, tree )


/* Two-parameter MACROS from tree.h */
fn_2( BINFO_BASETYPE, tree, tree, int )
fn_2( TYPE_BINFO_BASETYPE, tree, tree, int )
fn_2( TREE_OPERAND, tree, tree, int )
fn_2( TREE_VEC_ELT, tree, tree, int )


/* One-parameter MACROS from rtl.h */
fn_1( ADDR_DIFF_VEC_FLAGS, addr_diff_vec_flags, rtx )
fn_1( ADDRESSOF_DECL, tree, rtx )
fn_1( ADDRESSOF_REGNO, int, rtx )
fn_1( CALL_INSN_FUNCTION_USAGE, rtx, rtx )
fn_1( CODE_LABEL_NUMBER, int, rtx )
fn_1( CONST0_RTX, rtx, rtx )
fn_1( CONST1_RTX, rtx, rtx )
fn_1( CONST2_RTX, rtx, rtx )
fn_1( CONST_CALL_P, int, rtx )
fn_1( CONSTANT_P, int, rtx )
fn_1( CONSTANT_POOL_ADDRESS_P, int, rtx )
fn_1( CONTAINING_INSN, rtx, rtx )
fn_1( FIRST_FUNCTION_INSN, rtx, rtx )
fn_1( FIRST_LABELNO, int, rtx )
fn_1( FIRST_PARM_INSN, rtx, rtx )
fn_1( FORCED_LABELS, rtx, rtx )
fn_1( FUNCTION_ARGS_SIZE, int, rtx )
fn_1( FUNCTION_FLAGS, int, rtx )
fn_1( GET_CODE, int, rtx )
fn_1( GET_NOTE_INSN_NAME, char*, int )
fn_1( GET_NUM_ELEM, int, rtvec )
fn_1( GET_REG_NOTE_NAME, char *, rtx )
fn_1( GET_RTX_CLASS, char, rtx )
fn_1( GET_RTX_FORMAT, char*, rtx )
fn_1( GET_RTX_LENGTH, int, rtx )
fn_1( GET_RTX_NAME, char*, rtx )
fn_1( INLINE_REGNO_POINTER_ALIGN, char*, rtx )
fn_1( INLINE_REGNO_POINTER_FLAG, char*, rtx )
fn_1( INLINE_REGNO_REG_RTX, rtvec, rtx )
fn_1( INSN_ANNULLED_BRANCH_P, int, rtx )
fn_1( INSN_CODE, int, rtx )
fn_1( INSN_DELETED_P, int, rtx )
fn_1( INSN_FROM_TARGET_P, int, rtx )
fn_1( INSN_UID, int, rtx )
fn_1( INTVAL, HOST_WIDE_INT, rtx )
fn_1( JUMP_LABEL, rtx, rtx )
fn_1( LABEL_NAME, char*, rtx )
fn_1( LABEL_NEXTREF, rtx, rtx )
fn_1( LABEL_NUSES, int, rtx )
fn_1( LABEL_OUTSIDE_LOOP_P, int, rtx )
fn_1( LABEL_PRESERVE_P, int, rtx )
fn_1( LABEL_REF_NONLOCAL_P, int, rtx )
fn_1( LABEL_REFS, rtx, rtx )
fn_1( LAST_LABELNO, int, rtx )
fn_1( LINK_COST_FREE, int, rtx )
fn_1( LINK_COST_ZERO, int, rtx )
fn_1( LOG_LINKS, rtx, rtx )
fn_1( MAX_PARMREG, int, rtx )
fn_1( MAX_REGNUM, int, rtx )
fn_1( MEM_ALIAS_SET, int, rtx )
fn_1( MEM_IN_STRUCT_P, int, rtx )
fn_1( MEM_VOLATILE_P, int, rtx )
fn_1( NEXT_INSN, rtx, rtx )
fn_1( NOTE_BLOCK_NUMBER, int, rtx )
fn_1( NOTE_LINE_NUMBER, int, rtx )
fn_1( NOTE_LIVE_INFO, rtx, rtx )
fn_1( NOTE_RANGE_INFO, rtx, rtx )
fn_1( NOTE_SOURCE_FILE, char *, rtx )
fn_1( ORIGINAL_ARG_VECTOR, rtvec, rtx )
fn_1( ORIGINAL_DECL_INITIAL, rtx, rtx )
fn_1( OUTGOING_ARGS_SIZE, int, rtx )
fn_1( PARMREG_STACK_LOC, rtvec, rtx )
fn_1( PATTERN, rtx, rtx )
fn_1( POPS_ARGS, int, rtx )
fn_1( PREV_INSN, rtx, rtx )
fn_1( RANGE_INFO_BB_END, int, rtx )
fn_1( RANGE_INFO_BB_START, int, rtx )
fn_1( RANGE_INFO_LIVE_END, struct bitmap_head_def *, rtx )
fn_1( RANGE_INFO_LIVE_START, struct bitmap_head_def *, rtx )
fn_1( RANGE_INFO_LOOP_DEPTH, int, rtx )
fn_1( RANGE_INFO_MARKER_END, int, rtx )
fn_1( RANGE_INFO_MARKER_START, int, rtx )
fn_1( RANGE_INFO_NCALLS, int, rtx )
fn_1( RANGE_INFO_NINSNS, int, rtx )
fn_1( RANGE_INFO_NOTE_END, rtx, rtx )
fn_1( RANGE_INFO_NOTE_START, rtx, rtx )
fn_1( RANGE_INFO_NUM_REGS, int, rtx )
fn_1( RANGE_INFO_REGS, rtvec, rtx )
fn_1( RANGE_INFO_UNIQUE, int, rtx )
fn_1( RANGE_LIVE_BITMAP, struct bitmap_head_def *, rtx )
fn_1( RANGE_LIVE_ORIG_BLOCK, int, rtx )
fn_1( RANGE_VAR_BLOCK, tree, rtx )
fn_1( RANGE_VAR_LIST, rtx, rtx )
fn_1( RANGE_VAR_NUM, int, rtx )
fn_1( REG_FUNCTION_VALUE_P, int, rtx )
fn_1( REG_LOOP_TEST_P, int, rtx )
fn_1( REG_NOTE_KIND, enum reg_note, rtx )
fn_1( REG_NOTES, rtx, rtx )
fn_1( REG_P, int, rtx )
fn_1( REG_USERVAR_P, int, rtx )
fn_1( REGNO, int, rtx )
fn_1( RTX_FRAME_RELATED_P, int, rtx )
fn_1( RTX_INTEGRATED_P, int, rtx )
fn_1( RTX_UNCHANGING_P, int, rtx )
fn_1( SCHED_GROUP_P, int, rtx )
fn_1( SET_DEST, rtx, rtx )
fn_1( SET_SRC, rtx, rtx )
fn_1( STACK_SLOT_LIST, rtx, rtx )
fn_1( SUBREG_PROMOTED_UNSIGNED_P, int, rtx )
fn_1( SUBREG_PROMOTED_VAR_P, int, rtx )
fn_1( SUBREG_REG, rtx, rtx )
fn_1( SUBREG_WORD, int, rtx )
fn_1( SYMBOL_REF_FLAG, int, rtx )
fn_1( SYMBOL_REF_USED, int, rtx )
fn_1( TRAP_CODE, rtx, rtx )
fn_1( TRAP_CONDITION, rtx, rtx )

/* A few basic-block macros...  */
fn_1( BLOCK_FOR_INSN, basic_block, rtx)

/* Two-parameter MACROS from rtl.h */
fn_2( RTVEC_ELT, rtx, rtvec, int )
fn_2( XEXP, rtx, rtx, int )
fn_2( XINT, int, rtx, int )
fn_2( XWINT, HOST_WIDE_INT, rtx, int )
fn_2( XSTR, char*, rtx, int )
fn_2( XVEC, rtvec, rtx, int )
fn_2( XVECLEN, int, rtx, int )
fn_2( XBITMAP, struct bitmap_head_def *, rtx, int )
fn_2( XTREE, tree, rtx, int )
fn_2( FIND_REG_INC_NOTE, rtx, rtx, rtx )
fn_2( RANGE_INFO_REGS_REG, rtx, rtx, int )
fn_2( RANGE_REG_PSEUDO, int, rtx, int )
fn_2( RANGE_REG_COPY, int, rtx, int )
fn_2( RANGE_REG_REFS, int, rtx, int )
fn_2( RANGE_REG_SETS, int, rtx, int )
fn_2( RANGE_REG_DEATHS, int, rtx, int )
fn_2( RANGE_REG_COPY_FLAGS, int, rtx, int )
fn_2( RANGE_REG_LIVE_LENGTH, int, rtx, int )
fn_2( RANGE_REG_N_CALLS, int, rtx, int )
fn_2( RANGE_REG_SYMBOL_NODE, tree, rtx, int )
fn_2( RANGE_REG_BLOCK_NODE, tree, rtx, int )

/* And there's even a three-parameter one... */
fn_3( XVECEXP, rtx, rtx, int, int )
/* And that's more than all the C ones that I've run into. */

/* ...but wait, there's more!  How about retrieving an arbitrary element in
   a chain? And doing it in a brain-dead recursive way? */
tree 
TREE_CHAIN_N(head, n)
  tree head;
  int n;
{
  return n? TREE_CHAIN_N(TREE_CHAIN(head), --n): head;
}

int
TREE_CHAIN_LENGTH(head)
  tree head;
{
  return head? TREE_CHAIN_LENGTH(TREE_CHAIN(head)) + 1: 0;
}

/* Objective-C specific stuff */

#define fn_noden( m ) fn_1(m, tree, tree)
#define fn_nodei( m ) fn_1(m, int, tree)

fn_noden(KEYWORD_KEY_NAME)
fn_noden(KEYWORD_ARG_NAME)
fn_noden(METHOD_SEL_NAME)
fn_noden(METHOD_SEL_ARGS)
fn_noden(METHOD_ADD_ARGS)
fn_noden(METHOD_DEFINITION)
fn_noden(METHOD_ENCODING)
fn_noden(CLASS_NAME)
fn_noden(CLASS_SUPER_NAME)   
fn_noden(CLASS_IVARS)
fn_noden(CLASS_RAW_IVARS)
fn_noden(CLASS_NST_METHODS)
fn_noden(CLASS_CLS_METHODS)
fn_noden(CLASS_STATIC_TEMPLATE)
fn_noden(CLASS_CATEGORY_LIST)
fn_noden(CLASS_PROTOCOL_LIST)
fn_noden(PROTOCOL_NAME)
fn_noden(PROTOCOL_LIST)
fn_noden(PROTOCOL_NST_METHODS)
fn_noden(PROTOCOL_CLS_METHODS)
fn_noden(PROTOCOL_FORWARD_DECL)
fn_nodei(PROTOCOL_DEFINED)
fn_noden(TYPE_PROTOCOL_LIST)
fn_nodei(TREE_STATIC_TEMPLATE)
fn_nodei(IS_ID)
fn_nodei(IS_PROTOCOL_QUALIFIED_ID)
fn_nodei(IS_SUPER)

/* End of internal_debug.c */