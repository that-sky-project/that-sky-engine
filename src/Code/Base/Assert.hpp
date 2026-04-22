#ifndef __ASSERT_HPP__
#define __ASSERT_HPP__

#include <stdarg.h>
#include <intrin.h>

class Private {
public:
  static void AssertImpl(
    const char *expression,
    const char *file,
    int line);

  static void AssertMsgImpl(
    const char *expression,
    const char *file,
    int line,
    const char *msg,
    ...);
};

#define SkyAssert(expr) (void)(\
  (!!(expr)) || (Private::AssertImpl(#expr, __FILE__, __LINE__), __debugbreak(), 0)\
)

#define SkyAssertMsg(expr, msg, ...) (void)(\
  (!!(expr))\
  || (Private::AssertMsgImpl(#expr, __FILE__, __LINE__, msg, ## __VA_ARGS__), __debugbreak(), 0)\
)

#endif
