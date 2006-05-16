/* Declarations for objc-act.c.
   Copyright (C) 1990, 2000, 2001, 2002, 2003, 2004, 2005
   Free Software Foundation, Inc.

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

#ifndef GCC_OBJC_ACT_H
#define GCC_OBJC_ACT_H

/* APPLE LOCAL begin mainline */
/* For enum gimplify_status */
#include "tree-gimple.h"

/* APPLE LOCAL end mainline */
/*** Language hooks ***/

bool objc_init (void);
const char *objc_printable_name (tree, int);
/* APPLE LOCAL mainline */
tree objc_get_callee_fndecl (tree);
void objc_finish_file (void);
tree objc_fold_obj_type_ref (tree, tree);
/* APPLE LOCAL mainline */
/* The 'objc_types_compatible_p' routine has been removed.  */
/* APPLE LOCAL mainline */
enum gimplify_status objc_gimplify_expr (tree *, tree *, tree *);
/* APPLE LOCAL radar 4291785 */
void objc_detect_field_duplicates (tree);

/* NB: The remaining public functions are prototyped in c-common.h, for the
   benefit of stub-objc.c and objc-act.c.  */

/* Objective-C structures */

/* APPLE LOCAL radar 4291785 */
#define CLASS_LANG_SLOT_ELTS		7
#define PROTOCOL_LANG_SLOT_ELTS		2
#define OBJC_INFO_SLOT_ELTS		2

/* KEYWORD_DECL */
#define KEYWORD_KEY_NAME(DECL) ((DECL)->decl.name)
#define KEYWORD_ARG_NAME(DECL) ((DECL)->decl.arguments)

/* INSTANCE_METHOD_DECL, CLASS_METHOD_DECL */
#define METHOD_SEL_NAME(DECL) ((DECL)->decl.name)
#define METHOD_SEL_ARGS(DECL) ((DECL)->decl.arguments)
#define METHOD_ADD_ARGS(DECL) ((DECL)->decl.result)
#define METHOD_DEFINITION(DECL) ((DECL)->decl.initial)
#define METHOD_ENCODING(DECL) ((DECL)->decl.context)

/* CLASS_INTERFACE_TYPE, CLASS_IMPLEMENTATION_TYPE,
   CATEGORY_INTERFACE_TYPE, CATEGORY_IMPLEMENTATION_TYPE,
   PROTOCOL_INTERFACE_TYPE */
#define CLASS_NAME(CLASS) ((CLASS)->type.name)
#define CLASS_SUPER_NAME(CLASS) (TYPE_CHECK (CLASS)->type.context)
#define CLASS_IVARS(CLASS) TREE_VEC_ELT (TYPE_LANG_SLOT_1 (CLASS), 0)
#define CLASS_RAW_IVARS(CLASS) TREE_VEC_ELT (TYPE_LANG_SLOT_1 (CLASS), 1)
#define CLASS_NST_METHODS(CLASS) ((CLASS)->type.minval)
#define CLASS_CLS_METHODS(CLASS) ((CLASS)->type.maxval)
/* APPLE LOCAL begin ObjC new abi */
#define CLASS_TYPE(CLASS) ((CLASS)->type.main_variant)
#define PROTOCOL_V2_FORWARD_DECL(CLASS) TREE_VEC_ELT (TYPE_LANG_SLOT_1 (CLASS), 6)
/* APPLE LOCAL end ObjC new abi */
#define CLASS_STATIC_TEMPLATE(CLASS) TREE_VEC_ELT (TYPE_LANG_SLOT_1 (CLASS), 2)
#define CLASS_CATEGORY_LIST(CLASS) TREE_VEC_ELT (TYPE_LANG_SLOT_1 (CLASS), 3)
#define CLASS_PROTOCOL_LIST(CLASS) TREE_VEC_ELT (TYPE_LANG_SLOT_1 (CLASS), 4)
/* APPLE LOCAL radar 4291785 */
#define TOTAL_CLASS_RAW_IVARS(CLASS) TREE_VEC_ELT (TYPE_LANG_SLOT_1 (CLASS), 5)
#define PROTOCOL_NAME(CLASS) ((CLASS)->type.name)
#define PROTOCOL_LIST(CLASS) TREE_VEC_ELT (TYPE_LANG_SLOT_1 (CLASS), 0)
#define PROTOCOL_NST_METHODS(CLASS) ((CLASS)->type.minval)
#define PROTOCOL_CLS_METHODS(CLASS) ((CLASS)->type.maxval)
#define PROTOCOL_FORWARD_DECL(CLASS) TREE_VEC_ELT (TYPE_LANG_SLOT_1 (CLASS), 1)
#define PROTOCOL_DEFINED(CLASS) TREE_USED (CLASS)

/* ObjC-specific information pertaining to RECORD_TYPEs are stored in
   the LANG_SPECIFIC structures, which may itself need allocating first.  */

/* APPLE LOCAL begin mainline */
/* The following three macros must be overridden (in objcp/objcp-decl.h)
   for Objective-C++.  */
#define TYPE_OBJC_INFO(TYPE) TYPE_LANG_SPECIFIC (TYPE)->objc_info
#define SIZEOF_OBJC_TYPE_LANG_SPECIFIC sizeof (struct lang_type)
#define ALLOC_OBJC_TYPE_LANG_SPECIFIC(NODE)				\
  do {									\
    TYPE_LANG_SPECIFIC (NODE) = GGC_CNEW (struct lang_type);		\
  } while (0)

#define TYPE_HAS_OBJC_INFO(TYPE)				\
	(TYPE_LANG_SPECIFIC (TYPE) && TYPE_OBJC_INFO (TYPE))
#define TYPE_OBJC_INTERFACE(TYPE) TREE_VEC_ELT (TYPE_OBJC_INFO (TYPE), 0)
#define TYPE_OBJC_PROTOCOL_LIST(TYPE) TREE_VEC_ELT (TYPE_OBJC_INFO (TYPE), 1)


#define INIT_TYPE_OBJC_INFO(TYPE)				\
	do							\
	  {							\
	    if (!TYPE_LANG_SPECIFIC (TYPE))			\
	      ALLOC_OBJC_TYPE_LANG_SPECIFIC(TYPE);		\
	    if (!TYPE_OBJC_INFO (TYPE))				\
	      TYPE_OBJC_INFO (TYPE)				\
		= make_tree_vec (OBJC_INFO_SLOT_ELTS);		\
	  }							\
	while (0)
#define DUP_TYPE_OBJC_INFO(DST, SRC)				\
	do							\
	  {							\
	    ALLOC_OBJC_TYPE_LANG_SPECIFIC(DST);			\
	    if (TYPE_LANG_SPECIFIC (SRC))			\
	      memcpy (TYPE_LANG_SPECIFIC (DST),			\
		      TYPE_LANG_SPECIFIC (SRC),			\
		      SIZEOF_OBJC_TYPE_LANG_SPECIFIC);		\
	    TYPE_OBJC_INFO (DST)				\
	      = make_tree_vec (OBJC_INFO_SLOT_ELTS);		\
	  }							\
	while (0)
/* APPLE LOCAL end mainline */

/* APPLE LOCAL begin mainline */
/* code removed */
/* APPLE LOCAL end mainline */

#define TYPED_OBJECT(TYPE)					\
	(TREE_CODE (TYPE) == RECORD_TYPE			\
	 && TYPE_HAS_OBJC_INFO (TYPE)				\
	 && TYPE_OBJC_INTERFACE (TYPE))
#define OBJC_TYPE_NAME(TYPE) TYPE_NAME(TYPE)
#define OBJC_SET_TYPE_NAME(TYPE, NAME) (TYPE_NAME (TYPE) = NAME)

/* APPLE LOCAL begin mainline */
#define IDENTIFIER_INTERFACE_VALUE(NODE) \
	(((struct lang_identifier *) (NODE))->interface_value)
/* APPLE LOCAL end mainline */

/* Define the Objective-C or Objective-C++ language-specific tree codes.  */

#define DEFTREECODE(SYM, NAME, TYPE, LENGTH) SYM,
enum objc_tree_code {
#if defined (GCC_CP_TREE_H)
  LAST_BASE_TREE_CODE = LAST_CPLUS_TREE_CODE,
#else 
#if defined (GCC_C_TREE_H)
  LAST_BASE_TREE_CODE = LAST_C_TREE_CODE,
#else
  #error You must include <c-tree.h> or <cp/cp-tree.h> before <objc/objc-act.h>
#endif
#endif
#include "objc-tree.def"
  LAST_OBJC_TREE_CODE
};
#undef DEFTREECODE

/* Hash tables to manage the global pool of method prototypes.  */

typedef struct hashed_entry	*hash;
typedef struct hashed_attribute	*attr;

struct hashed_attribute GTY(())
{
  attr next;
  tree value;
};
struct hashed_entry GTY(())
{
  attr list;
  hash next;
  tree key;
};

extern GTY ((length ("SIZEHASHTABLE"))) hash *nst_method_hash_list;
extern GTY ((length ("SIZEHASHTABLE"))) hash *cls_method_hash_list;

#define SIZEHASHTABLE		257

/* Objective-C/Objective-C++ @implementation list.  */

struct imp_entry GTY(())
{
  struct imp_entry *next;
  tree imp_context;
  tree imp_template;
  tree class_decl;		/* _OBJC_CLASS_<my_name>; */
  tree meta_decl;		/* _OBJC_METACLASS_<my_name>; */
  /* APPLE LOCAL begin ObjC new abi */
  tree class_v2_decl;		/* _OBJC_V2_CLASS_<my_name>; */
  tree meta_v2_decl;		/* _OBJC_V2_METACLASS_<my_name>; */
  /* APPLE LOCAL end ObjC new abi */
  /* APPLE LOCAL mainline */
  BOOL_BITFIELD has_cxx_cdtors : 1;
};

extern GTY(()) struct imp_entry *imp_list;
extern GTY(()) int imp_count;	/* `@implementation' */
extern GTY(()) int cat_count;	/* `@category' */

extern GTY(()) enum tree_code objc_inherit_code;
extern GTY(()) int objc_public_flag;

/* Objective-C/Objective-C++ global tree enumeration.  */

enum objc_tree_index
{
    OCTI_STATIC_NST,
    OCTI_STATIC_NST_DECL,
    OCTI_SELF_ID,
    OCTI_UCMD_ID,
    /* APPLE LOCAL mainline */
    /* 'OCTI_UNUSED_LIST' removed.  */

    OCTI_SELF_DECL,
    OCTI_UMSG_DECL,
    /* APPLE LOCAL mainline */
    OCTI_UMSG_FAST_DECL,
    OCTI_UMSG_SUPER_DECL,
    OCTI_UMSG_STRET_DECL,
    OCTI_UMSG_SUPER_STRET_DECL,
    OCTI_GET_CLASS_DECL,
    OCTI_GET_MCLASS_DECL,
    OCTI_SUPER_TYPE,
    OCTI_SEL_TYPE,
    OCTI_ID_TYPE,
    OCTI_CLS_TYPE,
    OCTI_NST_TYPE,
    OCTI_PROTO_TYPE,

    OCTI_CLS_CHAIN,
    OCTI_ALIAS_CHAIN,
    OCTI_INTF_CHAIN,
    OCTI_PROTO_CHAIN,
    OCTI_IMPL_CHAIN,
    OCTI_CLS_REF_CHAIN,
    OCTI_SEL_REF_CHAIN,
    OCTI_IVAR_CHAIN,
    OCTI_CLS_NAMES_CHAIN,
    OCTI_METH_VAR_NAMES_CHAIN,
    OCTI_METH_VAR_TYPES_CHAIN,

    OCTI_SYMBOLS_DECL,
    OCTI_NST_VAR_DECL,
    OCTI_CLS_VAR_DECL,
    OCTI_NST_METH_DECL,
    OCTI_CLS_METH_DECL,
    OCTI_CLS_DECL,
    OCTI_MCLS_DECL,
    OCTI_SEL_TABLE_DECL,
    OCTI_MODULES_DECL,
    OCTI_GNU_INIT_DECL,

    OCTI_INTF_CTX,
    OCTI_IMPL_CTX,
    OCTI_METH_CTX,
    OCTI_IVAR_CTX,

    OCTI_IMPL_TEMPL,
    OCTI_CLS_TEMPL,
    OCTI_CAT_TEMPL,
    OCTI_UPRIV_REC,
    OCTI_PROTO_TEMPL,
    OCTI_SEL_TEMPL,
    OCTI_UCLS_SUPER_REF,
    OCTI_UUCLS_SUPER_REF,
    OCTI_METH_TEMPL,
    OCTI_IVAR_TEMPL,
    /* APPLE LOCAL begin mainline */
    OCTI_METH_LIST_TEMPL,
    OCTI_METH_PROTO_LIST_TEMPL,
    OCTI_IVAR_LIST_TEMPL,
    /* APPLE LOCAL end mainline */
    OCTI_SYMTAB_TEMPL,
    OCTI_MODULE_TEMPL,
    OCTI_SUPER_TEMPL,
    OCTI_OBJ_REF,
    OCTI_CLS_REF,
    OCTI_METH_PROTO_TEMPL,
    OCTI_FUNCTION1_TEMPL,
    OCTI_FUNCTION2_TEMPL,

    OCTI_OBJ_ID,
    OCTI_CLS_ID,
    OCTI_ID_NAME,
    OCTI_CLASS_NAME,
    OCTI_CNST_STR_ID,
    OCTI_CNST_STR_TYPE,
    OCTI_CNST_STR_GLOB_ID,
    OCTI_STRING_CLASS_DECL,
    /* APPLE LOCAL 4149909 */
    OCTI_INTERNAL_CNST_STR_TYPE,
    OCTI_SUPER_DECL,
    OCTI_UMSG_NONNIL_DECL,
    OCTI_UMSG_NONNIL_STRET_DECL,
    OCTI_STORAGE_CLS,
    OCTI_EXCEPTION_EXTRACT_DECL,
    OCTI_EXCEPTION_TRY_ENTER_DECL,
    OCTI_EXCEPTION_TRY_EXIT_DECL,
    OCTI_EXCEPTION_MATCH_DECL,
    OCTI_EXCEPTION_THROW_DECL,
    OCTI_SYNC_ENTER_DECL,
    OCTI_SYNC_EXIT_DECL,
    OCTI_SETJMP_DECL,
    OCTI_EXCDATA_TEMPL,
    OCTI_STACK_EXCEPTION_DATA_DECL,
    OCTI_LOCAL_EXCEPTION_DECL,
    OCTI_RETHROW_EXCEPTION_DECL,
    OCTI_EVAL_ONCE_DECL,
    OCTI_CATCH_TYPE,
    OCTI_EXECCLASS_DECL,
    /* APPLE LOCAL radar 4280641 */
    OCTI_UMSG_FPRET_DECL,

    /* APPLE LOCAL begin mainline */
    OCTI_ASSIGN_IVAR_DECL,
    OCTI_ASSIGN_IVAR_FAST_DECL,
    OCTI_ASSIGN_GLOBAL_DECL,
    OCTI_ASSIGN_STRONGCAST_DECL,
    /* APPLE LOCAL end mainline */

    /* APPLE LOCAL begin ObjC new abi */
    OCTI_V2_CLS_TEMPL,
    OCTI_V2_CLS_RO_TEMPL,
    OCTI_IMP_TYPE,
    OCTI_V2_CLS_DECL,
    OCTI_V2_MCLS_DECL,
    OCTI_V2_CACHE_DECL,
    OCTI_V2_VTABLE_DECL,
    OCTI_V2_PROTO_TEMPL,
    OCTI_CLASSLIST_REF_CHAIN,
    OCTI_MESSAGE_REF_TEMPL,
    OCTI_SUPER_MESSAGE_REF_TEMPL,
    CTI_MESSAGE_SELECTOR_TYPE,
    CTI_SUPER_MESSAGE_SELECTOR_TYPE,
    OCTI_V2_UMSG_FIXUP_DECL,
    OCTI_V2_UMSG_STRET_FIXUP_DECL,
    OCTI_V2_UMSG_ID_FIXUP_DECL,
    OCTI_V2_UMSG_ID_STRET_FIXUP_DECL,
    OCTI_V2_UMSG_SUPER2_FIXUP_DECL,
    OCTI_V2_UMSG_SUPER2_STRET_FIXUP_DECL,
    OCTI_V2_IMP_TYPE,
    OCTI_V2_SUPER_IMP_TYPE,
    OCTI_V2_MESSAGE_REF_CHAIN,
    OCTI_V2_NST_VAR_DECL,
    OCTI_V2_CLS_VAR_DECL,
    OCTI_V2_IVAR_TEMPL,
    OCTI_V2_IVAR_LIST_TEMPL,
    OCTI_V2_INST_METH_DECL,
    OCTI_V2_CLS_METH_DECL,
    OCTI_V2_IVAR_OFFSET_REF_CHAIN,
    /* APPLE LOCAL end ObjC new abi */

    OCTI_MAX
};

extern GTY(()) tree objc_global_trees[OCTI_MAX];

/* List of classes with list of their static instances.  */
#define objc_static_instances	objc_global_trees[OCTI_STATIC_NST]

/* The declaration of the array administrating the static instances.  */
#define static_instances_decl	objc_global_trees[OCTI_STATIC_NST_DECL]

/* Some commonly used instances of "identifier_node".  */

#define self_id			objc_global_trees[OCTI_SELF_ID]
#define ucmd_id			objc_global_trees[OCTI_UCMD_ID]
/* APPLE LOCAL mainline */
/* 'unused_list' removed.  */

#define self_decl		objc_global_trees[OCTI_SELF_DECL]
#define umsg_decl		objc_global_trees[OCTI_UMSG_DECL]
/* APPLE LOCAL mainline */
#define umsg_fast_decl		objc_global_trees[OCTI_UMSG_FAST_DECL]
#define umsg_super_decl		objc_global_trees[OCTI_UMSG_SUPER_DECL]
#define umsg_stret_decl		objc_global_trees[OCTI_UMSG_STRET_DECL]
#define umsg_super_stret_decl	objc_global_trees[OCTI_UMSG_SUPER_STRET_DECL]
#define objc_get_class_decl	objc_global_trees[OCTI_GET_CLASS_DECL]
#define objc_get_meta_class_decl			\
				objc_global_trees[OCTI_GET_MCLASS_DECL]

#define objc_super_type		objc_global_trees[OCTI_SUPER_TYPE]
/* APPLE LOCAL ObjC new abi */
#define objc_selector_type	objc_global_trees[OCTI_SEL_TYPE]
#define objc_object_type	objc_global_trees[OCTI_ID_TYPE]
#define objc_class_type		objc_global_trees[OCTI_CLS_TYPE]
#define objc_instance_type	objc_global_trees[OCTI_NST_TYPE]
#define objc_protocol_type	objc_global_trees[OCTI_PROTO_TYPE]

/* Type checking macros.  */

#define IS_ID(TYPE)							\
	(TREE_CODE (TYPE) == POINTER_TYPE				\
	 && (TYPE_MAIN_VARIANT (TREE_TYPE (TYPE))			\
	     == TREE_TYPE (objc_object_type)))
#define IS_CLASS(TYPE)							\
	(TREE_CODE (TYPE) == POINTER_TYPE				\
	 && (TYPE_MAIN_VARIANT (TREE_TYPE (TYPE))			\
	     == TREE_TYPE (objc_class_type)))
#define IS_PROTOCOL_QUALIFIED_UNTYPED(TYPE)				\
	((IS_ID (TYPE) || IS_CLASS (TYPE))				\
	 && TYPE_HAS_OBJC_INFO (TREE_TYPE (TYPE))			\
	 && TYPE_OBJC_PROTOCOL_LIST (TREE_TYPE (TYPE)))
#define IS_SUPER(TYPE)							\
	(TREE_CODE (TYPE) == POINTER_TYPE				\
	 && TREE_TYPE (TYPE) == objc_super_template)

#define class_chain		objc_global_trees[OCTI_CLS_CHAIN]
#define alias_chain		objc_global_trees[OCTI_ALIAS_CHAIN]
#define interface_chain		objc_global_trees[OCTI_INTF_CHAIN]
#define protocol_chain		objc_global_trees[OCTI_PROTO_CHAIN]
#define implemented_classes	objc_global_trees[OCTI_IMPL_CHAIN]

/* Chains to manage selectors that are referenced and defined in the
   module.  */

#define cls_ref_chain		objc_global_trees[OCTI_CLS_REF_CHAIN]	/* Classes referenced.  */
#define sel_ref_chain		objc_global_trees[OCTI_SEL_REF_CHAIN]	/* Selectors referenced.  */
#define objc_ivar_chain		objc_global_trees[OCTI_IVAR_CHAIN]

/* Chains to manage uniquing of strings.  */

#define class_names_chain	objc_global_trees[OCTI_CLS_NAMES_CHAIN]
#define meth_var_names_chain	objc_global_trees[OCTI_METH_VAR_NAMES_CHAIN]
#define meth_var_types_chain	objc_global_trees[OCTI_METH_VAR_TYPES_CHAIN]


/* Backend data declarations.  */

#define UOBJC_SYMBOLS_decl		objc_global_trees[OCTI_SYMBOLS_DECL]
#define UOBJC_INSTANCE_VARIABLES_decl	objc_global_trees[OCTI_NST_VAR_DECL]
#define UOBJC_CLASS_VARIABLES_decl	objc_global_trees[OCTI_CLS_VAR_DECL]
#define UOBJC_INSTANCE_METHODS_decl	objc_global_trees[OCTI_NST_METH_DECL]
#define UOBJC_CLASS_METHODS_decl	objc_global_trees[OCTI_CLS_METH_DECL]
#define UOBJC_CLASS_decl		objc_global_trees[OCTI_CLS_DECL]
#define UOBJC_METACLASS_decl		objc_global_trees[OCTI_MCLS_DECL]
#define UOBJC_SELECTOR_TABLE_decl	objc_global_trees[OCTI_SEL_TABLE_DECL]
#define UOBJC_MODULES_decl		objc_global_trees[OCTI_MODULES_DECL]
#define GNU_INIT_decl			objc_global_trees[OCTI_GNU_INIT_DECL]

/* The following are used when compiling a class implementation.
   implementation_template will normally be an interface, however if
   none exists this will be equal to objc_implementation_context...it is
   set in start_class.  */

#define objc_interface_context		objc_global_trees[OCTI_INTF_CTX]
#define objc_implementation_context	objc_global_trees[OCTI_IMPL_CTX]
#define objc_method_context		objc_global_trees[OCTI_METH_CTX]
#define objc_ivar_context		objc_global_trees[OCTI_IVAR_CTX]

#define implementation_template	objc_global_trees[OCTI_IMPL_TEMPL]
#define objc_class_template	objc_global_trees[OCTI_CLS_TEMPL]
#define objc_category_template	objc_global_trees[OCTI_CAT_TEMPL]
#define uprivate_record		objc_global_trees[OCTI_UPRIV_REC]
#define objc_protocol_template	objc_global_trees[OCTI_PROTO_TEMPL]
#define objc_selector_template	objc_global_trees[OCTI_SEL_TEMPL]
#define ucls_super_ref		objc_global_trees[OCTI_UCLS_SUPER_REF]
#define uucls_super_ref		objc_global_trees[OCTI_UUCLS_SUPER_REF]

#define umsg_nonnil_decl	objc_global_trees[OCTI_UMSG_NONNIL_DECL]
#define umsg_nonnil_stret_decl	objc_global_trees[OCTI_UMSG_NONNIL_STRET_DECL]
#define objc_storage_class	objc_global_trees[OCTI_STORAGE_CLS]
#define objc_exception_extract_decl		\
				objc_global_trees[OCTI_EXCEPTION_EXTRACT_DECL]
#define objc_exception_try_enter_decl		\
				objc_global_trees[OCTI_EXCEPTION_TRY_ENTER_DECL]
#define objc_exception_try_exit_decl		\
				objc_global_trees[OCTI_EXCEPTION_TRY_EXIT_DECL]
#define objc_exception_match_decl		\
				objc_global_trees[OCTI_EXCEPTION_MATCH_DECL]
#define objc_exception_throw_decl		\
				objc_global_trees[OCTI_EXCEPTION_THROW_DECL]
#define objc_sync_enter_decl	objc_global_trees[OCTI_SYNC_ENTER_DECL]
#define objc_sync_exit_decl	objc_global_trees[OCTI_SYNC_EXIT_DECL]
#define objc_exception_data_template		\
				objc_global_trees[OCTI_EXCDATA_TEMPL]
#define objc_setjmp_decl	objc_global_trees[OCTI_SETJMP_DECL]
#define objc_stack_exception_data		\
				objc_global_trees[OCTI_STACK_EXCEPTION_DATA_DECL]
#define objc_caught_exception	objc_global_trees[OCTI_LOCAL_EXCEPTION_DECL]	
#define objc_rethrow_exception	objc_global_trees[OCTI_RETHROW_EXCEPTION_DECL]	
#define objc_eval_once		objc_global_trees[OCTI_EVAL_ONCE_DECL]	
#define objc_catch_type		objc_global_trees[OCTI_CATCH_TYPE]

#define execclass_decl		objc_global_trees[OCTI_EXECCLASS_DECL]
/* APPLE LOCAL radar 4280641 */
#define umsg_fpret_decl		objc_global_trees[OCTI_UMSG_FPRET_DECL]

/* APPLE LOCAL begin mainline */
#define objc_assign_ivar_decl	objc_global_trees[OCTI_ASSIGN_IVAR_DECL]
#define objc_assign_ivar_fast_decl		\
				objc_global_trees[OCTI_ASSIGN_IVAR_FAST_DECL]
#define objc_assign_global_decl	objc_global_trees[OCTI_ASSIGN_GLOBAL_DECL]
#define objc_assign_strong_cast_decl		\
				objc_global_trees[OCTI_ASSIGN_STRONGCAST_DECL]
/* APPLE LOCAL end mainline */

#define objc_method_template	objc_global_trees[OCTI_METH_TEMPL]
#define objc_ivar_template	objc_global_trees[OCTI_IVAR_TEMPL]
/* APPLE LOCAL begin mainline */
#define objc_method_list_ptr	objc_global_trees[OCTI_METH_LIST_TEMPL]
#define objc_method_proto_list_ptr		\
				objc_global_trees[OCTI_METH_PROTO_LIST_TEMPL]
#define objc_ivar_list_ptr	objc_global_trees[OCTI_IVAR_LIST_TEMPL]
/* APPLE LOCAL end mainline */
#define objc_symtab_template	objc_global_trees[OCTI_SYMTAB_TEMPL]
#define objc_module_template	objc_global_trees[OCTI_MODULE_TEMPL]
#define objc_super_template	objc_global_trees[OCTI_SUPER_TEMPL]
#define objc_object_reference	objc_global_trees[OCTI_OBJ_REF]
#define objc_class_reference	objc_global_trees[OCTI_CLS_REF]
#define objc_method_prototype_template		\
				objc_global_trees[OCTI_METH_PROTO_TEMPL]
#define function1_template	objc_global_trees[OCTI_FUNCTION1_TEMPL]
#define function2_template	objc_global_trees[OCTI_FUNCTION2_TEMPL]

#define objc_object_id		objc_global_trees[OCTI_OBJ_ID]
#define objc_class_id		objc_global_trees[OCTI_CLS_ID]
#define objc_object_name		objc_global_trees[OCTI_ID_NAME]
#define objc_class_name		objc_global_trees[OCTI_CLASS_NAME]
#define constant_string_id	objc_global_trees[OCTI_CNST_STR_ID]
#define constant_string_type	objc_global_trees[OCTI_CNST_STR_TYPE]
#define constant_string_global_id		\
				objc_global_trees[OCTI_CNST_STR_GLOB_ID]
#define string_class_decl	objc_global_trees[OCTI_STRING_CLASS_DECL]
/* APPLE LOCAL 4149909 */
#define internal_const_str_type	objc_global_trees[OCTI_INTERNAL_CNST_STR_TYPE]
#define UOBJC_SUPER_decl	objc_global_trees[OCTI_SUPER_DECL]

/* APPLE LOCAL begin ObjC new abi */
#define objc_v2_class_template     objc_global_trees[OCTI_V2_CLS_TEMPL]
#define objc_v2_class_ro_template  objc_global_trees[OCTI_V2_CLS_RO_TEMPL]
#define objc_v2_protocol_template  objc_global_trees[OCTI_V2_PROTO_TEMPL]
#define objc_imp_type		       objc_global_trees[OCTI_IMP_TYPE]
#define UOBJC_V2_CLASS_decl        objc_global_trees[OCTI_V2_CLS_DECL]
#define UOBJC_V2_METACLASS_decl    objc_global_trees[OCTI_V2_MCLS_DECL]
#define UOBJC_V2_CACHE_decl	       objc_global_trees[OCTI_V2_CACHE_DECL]
#define UOBJC_V2_VTABLE_decl       objc_global_trees[OCTI_V2_VTABLE_DECL]
#define UOBJC_V2_INSTANCE_VARIABLES_decl  objc_global_trees[OCTI_V2_NST_VAR_DECL]
#define UOBJC_V2_CLASS_VARIABLES_decl	objc_global_trees[OCTI_V2_CLS_VAR_DECL]
#define UOBJC_V2_INSTANCE_METHODS_decl	objc_global_trees[OCTI_V2_INST_METH_DECL]
#define UOBJC_V2_CLASS_METHODS_decl		objc_global_trees[OCTI_V2_CLS_METH_DECL]
#define objc_v2_ivar_template	objc_global_trees[OCTI_V2_IVAR_TEMPL]
/* classes referenced.  */
#define classlist_ref_chain	       objc_global_trees[OCTI_CLASSLIST_REF_CHAIN]
/* struct message_ref_t */
#define objc_v2_message_ref_template objc_global_trees[OCTI_MESSAGE_REF_TEMPL]      
/* struct super_message_ref_t */
#define objc_v2_super_message_ref_template objc_global_trees[OCTI_SUPER_MESSAGE_REF_TEMPL]      
/* struct message_ref_t* */
#define objc_v2_selector_type      objc_global_trees[CTI_MESSAGE_SELECTOR_TYPE]	
/* struct super_super_message_ref_t */
#define objc_v2_super_selector_type      objc_global_trees[CTI_SUPER_MESSAGE_SELECTOR_TYPE]	
/* objc_msgSend_fixup_rtp */
#define umsg_fixup_decl		       objc_global_trees[OCTI_V2_UMSG_FIXUP_DECL]   
/* objc_msgSend_stret_fixup_rtp */
#define umsg_stret_fixup_decl	       objc_global_trees[OCTI_V2_UMSG_STRET_FIXUP_DECL]   
/* objc_msgSendId_fixup_rtp */
#define umsg_id_fixup_decl	       objc_global_trees[OCTI_V2_UMSG_ID_FIXUP_DECL]   
/* objc_msgSendId_stret_fixup_rtp */
#define umsg_id_stret_fixup_decl       objc_global_trees[OCTI_V2_UMSG_ID_STRET_FIXUP_DECL]   
/* objc_msgSendSuper2_fixup_rtp */
#define umsg_id_super2_fixup_decl      objc_global_trees[OCTI_V2_UMSG_SUPER2_FIXUP_DECL]
/* objc_msgSendSuper2_stret_fixup_rtp */
#define umsg_id_super2_stret_fixup_decl objc_global_trees[OCTI_V2_UMSG_SUPER2_STRET_FIXUP_DECL]
#define objc_v2_imp_type 		objc_global_trees[OCTI_V2_IMP_TYPE]
#define objc_v2_super_imp_type 	objc_global_trees[OCTI_V2_SUPER_IMP_TYPE]
#define message_ref_chain		objc_global_trees[OCTI_V2_MESSAGE_REF_CHAIN]
#define objc_v2_ivar_list_ptr	objc_global_trees[OCTI_V2_IVAR_LIST_TEMPL]
#define ivar_offset_ref_chain		objc_global_trees[OCTI_V2_IVAR_OFFSET_REF_CHAIN]
/* APPLE LOCAL end ObjC new abi */
#endif /* GCC_OBJC_ACT_H */
