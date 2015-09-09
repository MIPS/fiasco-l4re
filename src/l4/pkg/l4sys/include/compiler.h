/*****************************************************************************/
/**
 * \file
 * \brief   L4 compiler related defines.
 * \ingroup l4_api
 */
/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>,
 *               Frank Mehnert <fm3@os.inf.tu-dresden.de>,
 *               Jork Löser <jork@os.inf.tu-dresden.de>,
 *               Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 */
/*****************************************************************************/
#ifndef __L4_COMPILER_H__
#define __L4_COMPILER_H__

#if !defined(__ASSEMBLY__) && !defined(__ASSEMBLER__)

/**
 * \addtogroup l4sys_defines
 *
 * <c>\#include <l4/sys/compiler.h></c>
 */
/*@{*/

/**
 * L4 Inline function attribute.
 * \hideinitializer
 */
#ifndef L4_INLINE
#ifndef __cplusplus
#  ifdef __OPTIMIZE__
#    define L4_INLINE_STATIC static __inline__
#    define L4_INLINE_EXTERN extern __inline__
     /* gcc-4.3 implements c99 inline behaviour, i.e. we use the
      * 'extern  inline' there, 4.2 and below use 'static inline' */
#    if (__GNUC__ == 4 && __GNUC_MINOR__ <= 2) || __GNUC__ <= 3
#      define L4_INLINE L4_INLINE_STATIC
#    else
#      ifdef __GNUC_STDC_INLINE__
#        define L4_INLINE L4_INLINE_STATIC
#      else
#        define L4_INLINE L4_INLINE_EXTERN
#      endif
#    endif
#  else /* ! __OPTIMIZE__ */
#    define L4_INLINE static
#  endif /* ! __OPTIMIZE__ */
#else /* __cplusplus */
#  define L4_INLINE inline
#endif  /* __cplusplus */
#endif  /* L4_INLINE */

#if ((__GNUC__ * 100 + __GNUC_MINOR__) >= 405)
# define L4_DECLARE_CONSTRUCTOR(func, prio) \
  static inline __attribute__((constructor(prio))) void func ## _ctor_func(void) { func(); }
#else


/**
 * \brief Handcoded version of __attribute__((constructor(xx))).
 * \param func function declaration (prototype)
 * \param prio the prio must be 65535 - \a gcc_prio
 */
#if defined (__ARM_EABI__)
#  define L4_DECLARE_CONSTRUCTOR(func, prio) \
    static __typeof(&func) func ## _ctor__ __attribute__((used,section(".init_array." L4_stringify(prio)))) = &func;
#else
#  define L4_DECLARE_CONSTRUCTOR(func, prio) \
    static __typeof(&func) func ## _ctor__ __attribute__((used,section(".ctors." L4_stringify(prio)))) = &func;
#endif
#endif



/**
 * Start section with C types and functions.
 * \def     __BEGIN_DECLS
 * \hideinitializer
 */
/**
 * End section with C types and functions.
 * \def     __END_DECLS
 * \hideinitializer
 */
/**
 * Start section with C types and functions.
 * \def     EXTERN_C_BEGIN
 * \hideinitializer
 */
/**
 * End section with C types and functions.
 * \def     EXTERN_C_END
 * \hideinitializer
 */
/**
 * Mark C types and functions.
 * \def     EXTERN_C
 * \hideinitializer
 */

/**
 * \def L4_NOTHROW
 * \hideinitializer
 * \brief Mark a function declaration and definition as never
 *        throwing an exception. (Also for C code).
 *
 * This macro shall be used to mark C and C++ functions that never
 * throw any exception.  Note that also C functions may throw exceptions
 * according to the compilers ABI and shall be marke with L4_NOTHROW
 * if they never do.  In C++ this is equvalent to \c throw().
 *
 * \code
 * int foo() L4_NOTHROW;
 * ...
 * int foo() L4_NOTHROW
 * {
 *   ...
 *   return result;
 * }
 * \endcode
 *
 */

/**
 * \def L4_EXPORT
 * \hideinitializer
 * \brief Attribute to mark functions, variables, and data types as being
 *        exported from a library.
 *
 * All data types, functions, and global variables that shall be exported
 * from a library shall be marked with this attribute.  The default may become
 * to hide everything that is not marked as L4_EXPORT from the users of a
 * library and provide the possibility for aggressive optimization of all
 * those internal functionality of a library.
 *
 * Usage:
 * \code
 * class L4_EXPORT My_class
 * {
 *   ...
 * };
 *
 * int L4_EXPORT function(void);
 *
 * int L4_EXPORT global_data; // global data is not recommended
 * \endcode
 *
 */

/**
 * \def L4_HIDDEN
 * \hideinitializer
 * \brief Attribute to mark functions, variables, and data types as being
 *        explicitly hidden from users of a library.
 *
 * This attribute is intended for functions, data, and data types that
 * shall never be visible outside of a library.  In particular, for shared
 * libraries this may result in much faster code within the library and short
 * linking times.
 *
 * \code
 * class L4_HIDDEN My_class
 * {
 *   ...
 * };
 *
 * int L4_HIDDEN function(void);
 *
 * int L4_HIDDEN global_data; // global data is not recommended
 * \endcode
 */
#ifndef __cplusplus
#  define L4_NOTHROW__A       __attribute__((nothrow))
#  define L4_NOTHROW
#  define EXTERN_C_BEGIN
#  define EXTERN_C_END
#  define EXTERN_C
#  ifndef __BEGIN_DECLS
#    define __BEGIN_DECLS
#  endif
#  ifndef __END_DECLS
#    define __END_DECLS
#  endif
#  define L4_DEFAULT_PARAM(x)
#else /* __cplusplus */
#  define L4_NOTHROW throw()
#  define EXTERN_C_BEGIN extern "C" {
#  define EXTERN_C_END }
#  define EXTERN_C extern "C"
#  ifndef __BEGIN_DECLS
#    define __BEGIN_DECLS extern "C" {
#  endif
#  ifndef __END_DECLS
#    define __END_DECLS }
#  endif
#  define L4_DEFAULT_PARAM(x) = x
#endif /* __cplusplus */

/**
 * Noreturn function attribute.
 * \hideinitializer
 */
#define L4_NORETURN __attribute__((noreturn))

#define L4_PURE __attribute__((pure))

/**
 * No instrumentation function attribute.
 * \hideinitializer
 */
#define L4_NOINSTRUMENT __attribute__((no_instrument_function))
#ifndef L4_HIDDEN
#  define L4_HIDDEN __attribute__((visibility("hidden")))
#endif
#ifndef L4_EXPORT
#  define L4_EXPORT __attribute__((visibility("default")))
#endif
#define L4_STRONG_ALIAS(name, aliasname) L4__STRONG_ALIAS(name, aliasname)
#define L4__STRONG_ALIAS(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((alias (#name)));


#endif /* !__ASSEMBLY__ */

#include <l4/sys/linkage.h>

#if __GNUC__ == 2 && __GNUC_MINOR__ < 96
#define __builtin_expect(x, expected_value) (x)
#endif

#define L4_LIKELY(x)	__builtin_expect((x),1)   ///< Expression is likely to execute. \hideinitializer
#define L4_UNLIKELY(x)	__builtin_expect((x),0)   ///< Expression is unlikely to execute. \hideinitializer

#if (__GNUC__ == 3 && __GNUC_MINOR__ >= 3) || __GNUC__ >= 4
/* Make sure that the function is not removed by optimization. Without the
 * "used" attribute, unreferenced static functions are removed. */
#define L4_STICKY(x)	__attribute__((used)) x         ///< Mark symbol sticky (even not there) \hideinitializer
/* The deprecated attribute is available with 3.1 and higher (3.3 as here
 * is ok for us */
# if (__GNUC__ == 4 && __GNUC_MINOR__ >= 5) || __GNUC__ >= 5
# define L4_DEPRECATED(s) __attribute__((deprecated(s)))     ///< Mark symbol deprecated. \hideinitializer
# else
# define L4_DEPRECATED(s) __attribute__((deprecated))     ///< Mark symbol deprecated. \hideinitializer
# endif
#else
/* The "used" attribute is not available with older gcc versions so simply
 * make sure that gcc doesn't warn about unused functions. */
#define L4_STICKY(x)	__attribute__((unused)) x       ///< Mark symbol sticky (even not there).
#define L4_DEPRECATED(s)                                ///< Mark symbol deprecated
#endif

#ifndef __GXX_EXPERIMENTAL_CXX0X__
#ifndef static_assert
#define static_assert(x, y) \
  do { (void)sizeof(char[-(!(x))]); } while (0)
#endif
#endif

#define L4_stringify_helper(x) #x                       ///< stringify helper. \hideinitializer
#define L4_stringify(x)        L4_stringify_helper(x)   ///< stringify. \hideinitializer

#ifndef __ASSEMBLER__
/**
 * \brief Memory barrier.
 */
L4_INLINE void l4_barrier(void);

/**
 * \brief Memory barrier.
 */
L4_INLINE void l4_mb(void);

/**
 * \brief Write memory barrier.
 */
L4_INLINE void l4_wmb(void);


/* Implementations */
L4_INLINE void l4_barrier(void)
{
  __asm__ __volatile__ ("" : : : "memory");
}

L4_INLINE void l4_mb(void)
{
  __asm__ __volatile__ ("" : : : "memory");
}

L4_INLINE void l4_wmb(void)
{
  __asm__ __volatile__ ("" : : : "memory");
}
#endif

/*@}*/

#endif /* !__L4_COMPILER_H__ */
