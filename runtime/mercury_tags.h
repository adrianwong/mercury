/*
** Copyright (C) 1993-2001 The University of Melbourne.
** This file may only be copied under the terms of the GNU Library General
** Public License - see the file COPYING.LIB in the Mercury distribution.
*/

/*
** mercury_tags.h - defines macros for tagging and untagging words.
** Also defines macros for accessing the Mercury list type from C.
*/

#ifndef MERCURY_TAGS_H
#define	MERCURY_TAGS_H

#include <limits.h>		/* for `CHAR_BIT' */
#include "mercury_conf.h"	/* for `MR_LOW_TAG_BITS' */
#include "mercury_types.h"	/* for `MR_Word' */

/* DEFINITIONS FOR WORD LAYOUT */

#define	MR_WORDBITS	(CHAR_BIT * sizeof(MR_Word))

/*
** MR_TAGBITS specifies the number of bits in each word
** that we can use for tags.
*/
#ifndef MR_TAGBITS
  #ifdef MR_HIGHTAGS
    #error "MR_HIGHTAGS defined but MR_TAGBITS undefined"
  #else
    #define MR_TAGBITS	MR_LOW_TAG_BITS
  #endif
#endif

#if MR_TAGBITS > 0 && defined(MR_HIGHTAGS) && defined(MR_CONSERVATIVE_GC)
  #error "Conservative GC does not work with high tag bits"
#endif

#ifdef	MR_HIGHTAGS

#define	MR_mktag(t)	((MR_Word)(t) << (MR_WORDBITS - MR_TAGBITS))
#define	MR_unmktag(w)	((MR_Word)(w) >> (MR_WORDBITS - MR_TAGBITS))
#define	MR_tag(w)	((w) & ~(~(MR_Word)0 >> MR_TAGBITS))
#define	MR_mkbody(i)	(i)
#define	MR_unmkbody(w)	(w)
#define	MR_body(w, t)	((w) & (~(MR_Word)0 >> MR_TAGBITS))
#define	MR_strip_tag(w)	((w) & (~(MR_Word)0 >> MR_TAGBITS))

#else /* ! MR_HIGHTAGS */

#define	MR_mktag(t)	(t)
#define	MR_unmktag(w)	(w)
#define	MR_tag(w)	((w) & ((1 << MR_TAGBITS) - 1))
#define	MR_mkbody(i)	((i) << MR_TAGBITS)
#define	MR_unmkbody(w)	((MR_Word) (w) >> MR_TAGBITS)
#define	MR_body(w, t)	((MR_Word) (w) - (t))
#define	MR_strip_tag(w)	((w) & (~(MR_Word)0 << MR_TAGBITS))

#endif /* ! MR_HIGHTAGS */

/*
** the result of MR_mkword() is cast to (MR_Word *), not to (MR_Word)
** because MR_mkword() may be used in initializers for static constants
** and casts from pointers to integral types are not valid
** constant-expressions in ANSI C.  It cannot be (const MR_Word *) because
** some ANSI C compilers won't allow assignments where the RHS is of type
** const and the LHS is not declared const.
*/

#define	MR_mkword(t, p)			((MR_Word *)((char *)(p) + (t)))

#define	MR_field(t, p, i)		((MR_Word *) MR_body((p), (t)))[i]
#define	MR_const_field(t, p, i)		((const MR_Word *) MR_body((p), (t)))[i]

#define	MR_mask_field(p, i)		((MR_Word *) MR_strip_tag(p))[i]
#define	MR_const_mask_field(p, i)	((const MR_Word *) MR_strip_tag(p))[i]

/*
** The hl_ variants are the same, except their return type is MR_Box
** rather than MR_Word.  These are used by the MLDS->C back-end.
*/
#define	MR_hl_field(t, p, i)		((MR_Box *) MR_body((p), (t)))[i]
#define	MR_hl_const_field(t, p, i)	((const MR_Box *) MR_body((p), (t)))[i]

#define	MR_hl_mask_field(p, i)		((MR_Box *) MR_strip_tag(p))[i]
#define	MR_hl_const_mask_field(p, i)	((const MR_Box *) MR_strip_tag(p))[i]

/*
** the following macros are used by handwritten C code that needs to access 
** Mercury data structures. The definitions of these macros depend on the data 
** representation scheme used by compiler/make_tags.m.
*/

#ifdef MR_RESERVE_TAG
  #define MR_RAW_TAG_VAR               0     /* for Prolog-style variables */
  #define MR_FIRST_UNRESERVED_RAW_TAG  1
#else
  #define MR_FIRST_UNRESERVED_RAW_TAG  0
#endif

#if MR_TAGBITS == 0 && \
	(MR_NUM_RESERVED_ADDRESSES > 0 || MR_NUM_RESERVED_OBJECTS > 0)
  /*
  ** In this case, we represent the empty list as a reserved address,
  ** rather than using tag bits.
  */
  #define MR_RAW_TAG_CONS       MR_FIRST_UNRESERVED_RAW_TAG
#else
  #define MR_RAW_TAG_NIL        MR_FIRST_UNRESERVED_RAW_TAG
  #define MR_RAW_TAG_CONS       (MR_FIRST_UNRESERVED_RAW_TAG + 1)
#endif

#define MR_RAW_UNIV_TAG         MR_FIRST_UNRESERVED_RAW_TAG

#define	MR_TAG_NIL		MR_mktag(MR_RAW_TAG_NIL)
#define	MR_TAG_CONS		MR_mktag(MR_RAW_TAG_CONS)

#ifdef MR_RESERVE_TAG
  #define MR_TAG_VAR		MR_mktag(MR_RAW_TAG_VAR)
#endif

#define	MR_UNIV_TAG		MR_mktag(MR_RAW_UNIV_TAG)

#if MR_TAGBITS > 0 || (MR_TAGBITS == 0 && \
	(MR_NUM_RESERVED_ADDRESS > 0 || MR_NUM_RESERVED_OBJECTS > 0))
  /*
  ** Cons cells are represented using two words.
  */
  #if MR_TAGBITS == 0 && MR_NUM_RESERVED_ADDRESSES > 0
    /*
    ** We represent empty lists as null pointers.
    */
    #define MR_list_empty()		((MR_Word) NULL)
    #define MR_list_is_empty(list)	((list) == MR_list_empty())
  #elif MR_TAGBITS == 0 && MR_NUM_RESERVED_OBJECTS > 0
    /*
    ** We represent empty lists as the address of a reserved object,
    ** which will be generated by the compiler in the code for library/list.m.
    ** (The mangled name `f_111_...' of this object
    ** is the mangled form of the name `obj_[]_0'.)
    */
    extern const struct mercury__list__list_1_s
    	mercury__list__list_1__f_111_98_106_95_91_93_95_48;
    #define MR_list_empty()
	((MR_Word) (& mercury__list__list_1__f_111_98_106_95_91_93_95_48))
    #define MR_list_is_empty(list)	((list) == MR_list_empty())
  #else
    /*
    ** We use the primary tag to distinguish between empty and non-empty lists.
    */
    #define MR_list_is_empty(list) (MR_tag(list) == MR_TAG_NIL)
    #define MR_list_empty()	((MR_Word) MR_mkword(MR_TAG_NIL, MR_mkbody(0)))
  #endif
  #define MR_list_head(list)	MR_field(MR_TAG_CONS, (list), 0)
  #define MR_list_tail(list)	MR_field(MR_TAG_CONS, (list), 1)
  #define MR_list_cons(head,tail) ((MR_Word) MR_mkword(MR_TAG_CONS, \
					  MR_create2((head),(tail))))
  #define MR_list_empty_msg(proclabel)	\
				MR_list_empty()
  #define MR_list_cons_msg(head,tail,proclabel) \
				((MR_Word) MR_mkword(MR_TAG_CONS, \
					MR_create2_msg((head),(tail), \
						proclabel, "list:list/1")))

#else
  /*
  ** MR_TAGBITS == 0 && 
  ** MR_NUM_RESERVED_ADDRESS == 0 &&
  ** MR_NUM_RESERVED_OBJECTS == 0
  **
  ** In this case, cons cells are represented using three words.
  ** The first word is a secondary tag that we use to distinguish between
  ** empty and non-empty lists.
  */

  #define	MR_list_is_empty(list)	(MR_field(MR_mktag(0), (list), 0) \
					== MR_RAW_TAG_NIL)
  #define	MR_list_head(list)	MR_field(MR_mktag(0), (list), 1)
  #define	MR_list_tail(list)	MR_field(MR_mktag(0), (list), 2)
  #define	MR_list_empty()		((MR_Word) MR_mkword(MR_mktag(0), \
					MR_create1(MR_RAW_TAG_NIL)))
  #define	MR_list_cons(head,tail)	((MR_Word) MR_mkword(MR_mktag(0), \
					MR_create3(MR_RAW_TAG_CONS, \
						(head), (tail))))
  #define	MR_list_empty_msg(proclabel) \
				((MR_Word) MR_mkword(MR_mktag(0), \
					MR_create1_msg(MR_RAW_TAG_NIL, \
						proclabel, "list:list/1")))
  #define	MR_list_cons_msg(head,tail,proclabel) \
				((MR_Word) MR_mkword(MR_mktag(0), \
					MR_create3_msg(MR_RAW_TAG_CONS, \
						(head), (tail), \
						proclabel, "list:list/1")))

#endif

/*
** Convert an enumeration declaration into one which assigns the same
** values to the enumeration constants as Mercury's tag allocation scheme
** assigns. (This is necessary because in .rt grades Mercury enumerations are
** not assigned the same values as 'normal' C enumerations).
** 
*/

#ifdef MR_RESERVE_TAG

	#define MR_CONVERT_C_ENUM_CONSTANT(x) \
		MR_mkword(MR_mktag(MR_FIRST_UNRESERVED_RAW_TAG), MR_mkbody(x))

		/*
		** We generate three enumeration constants:
		** 	- the first one, with '_val' pasted at the end of its
		**	  name, to give us a name for the current *unconverted*
		**	  enumeration value
		**	- the converted enumeration value
		**	- a '_dummy' value to reset the unconverted enumeration
		**	  value
		*/
	#define MR_DEFINE_MERCURY_ENUM_CONST(x)	\
		MR_PASTE2(x, _val),	\
		x = MR_CONVERT_C_ENUM_CONSTANT(MR_PASTE2(x, _val)), \
		MR_PASTE2(x, _dummy) = MR_PASTE2(x, _val)

#else

	#define MR_CONVERT_C_ENUM_CONSTANT(x)   (x)

	#define MR_DEFINE_MERCURY_ENUM_CONST(x)	x

#endif


#endif	/* not MERCURY_TAGS_H */
