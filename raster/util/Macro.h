/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

/*
 * Introduces an identifier starting with str and ending with a number
 * that varies with the line.
 */
#ifndef RDD_ANONYMOUS_VARIABLE
#define RDD_CONCATENATE_IMPL(s1, s2) s1##s2
#define RDD_CONCATENATE(s1, s2) RDD_CONCATENATE_IMPL(s1, s2)
#ifdef __COUNTER__
#define RDD_ANONYMOUS_VARIABLE(str) RDD_CONCATENATE(str, __COUNTER__)
#else
#define RDD_ANONYMOUS_VARIABLE(str) RDD_CONCATENATE(str, __LINE__)
#endif
#endif

/*
 * Stringize
 */
#define RDD_STRINGIZE(x) #x
#define RDD_STRINGIZE2(x) RDD_STRINGIZE(x)

/*
 * ALIGNED
 */
#define RDD_ALIGNED(size) __attribute__((__aligned__(size)))

/*
 * LIKELY
 */
#undef LIKELY
#undef UNLIKELY

#if defined(__GNUC__) && __GNUC__ >= 4
#define LIKELY(x)   (__builtin_expect((x), 1))
#define UNLIKELY(x) (__builtin_expect((x), 0))
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif

/*
 * Array size.
 */
#define NELEMS(v) (sizeof(v) / sizeof((v)[0]))

/*
 * Unit conversion.
 */
#define B2G(i) (float(i) / 1073741824L) // 1024*1024*1024

