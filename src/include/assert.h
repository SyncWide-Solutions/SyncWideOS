#ifndef ASSERT_H
#define ASSERT_H

#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
#define assert(expr) \
    ((expr) ? (void)0 : __assert_func(__FILE__, __LINE__, __func__, #expr))
#endif

void __assert_func(const char *file, int line, const char *func, const char *expr);

#endif