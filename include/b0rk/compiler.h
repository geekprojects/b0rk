#ifndef __B0RK_COMPILER_H_
#define __B0RK_COMPILER_H_

#ifdef __GNUC__
#define B0RK_LIKELY(x) (__builtin_expect((x), 0))
#define B0RK_UNLIKELY(x) (__builtin_expect((x), 0))
#else
#define B0RK_LIKELY(x)
#define B0RK_UNLIKELY(x)
#endif

#endif
