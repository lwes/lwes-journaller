/* GLOBAL.H - RSAREF types and constants
 */

/* Copyright (C) RSA Laboratories, a division of RSA Data Security,
     Inc., created 1991. All rights reserved.
 */

#include <config.h>

#ifndef _GLOBAL_H_
#define _GLOBAL_H_ 1

/* PROTOTYPES should be set to one if and only if the compiler supports
     function argument prototyping.
   The following makes PROTOTYPES default to 1 if it has not already been
     defined as 0 with C compiler flags.
 */
#ifndef PROTOTYPES
#define PROTOTYPES 1
#endif

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
/* UINT4 defines a four byte word */

#include <sys/types.h>

#ifdef HAVE_U_INT16_T
#define FIXED_SIZE_TYPES 1
typedef u_int16_t UINT2;
typedef u_int32_t UINT4;
#elif HAVE_UINT16_T
#define FIXED_SIZE_TYPES 1
typedef uint16_t UINT2;
typedef uint32_t UINT4;
#endif

#ifndef FIXED_SIZE_TYPES
#error need fixed size integer typedefs
#endif

#ifndef NULL_PTR
#define NULL_PTR ((POINTER)0)
#endif

#ifndef UNUSED_ARG
#define UNUSED_ARG(x) x = *(&x);
#endif

/* PROTO_LIST is defined depending on how PROTOTYPES is defined above.
   If using PROTOTYPES, then PROTO_LIST returns the list, otherwise it
     returns an empty list.  
 */
#if PROTOTYPES
#define PROTO_LIST(list) list
#else
#define PROTO_LIST(list) ()
#endif

#endif /* end _GLOBAL_H_ */
