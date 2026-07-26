#ifndef __ASSERT_HPP__
#define __ASSERT_HPP__

#include <Utils/Types.h>
#include <stdarg.h>
#include <debug-trap.h>

#define SkyAssert(expr) (void)(\
  (!!(expr)) || (Private::AssertImpl(#expr, __FILE__, __LINE__, 3), psnip_trap(), abort(), 0)\
)

#define SkyAssertMsg(expr, msg, ...) (void)(\
  (!!(expr))\
  || (Private::AssertMsgImpl(#expr, __FILE__, __LINE__, 3, msg, ## __VA_ARGS__), psnip_trap(), abort(), 0)\
)

namespace Private {

void AssertImpl(
  cstring expr,
  cstring file,
  u32 line,
  u32 col);

void AssertMsgImpl(
  cstring expr,
  cstring file,
  u32 line,
  u32 col,
  cstring format,
  ...);

}

using PFN_AssertHandler = void (*)(cstring expr, cstring msg, cstring file, u32 line, u32 col);

void SetAssertHandler(PFN_AssertHandler handler);
void GetAssertHandler(PFN_AssertHandler *pHandler);

#endif
