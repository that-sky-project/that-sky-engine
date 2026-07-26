#include <stdio.h>
#include "Utils/Types.h"
#include "Utils/Assert.hpp"

static PFN_AssertHandler g_assertHandler = nullptr;

void Private::AssertImpl(
  cstring expr,
  cstring file,
  u32 line,
  u32 col
) {
  if (g_assertHandler)
    g_assertHandler(expr, "", file, line, col);
}

void Private::AssertMsgImpl(
  cstring expr,
  cstring file,
  u32 line,
  u32 hash,
  cstring format,
  ...
) {
  char buffer[2056];

  va_list va;
  va_start(va, format);
  vsnprintf(buffer, sizeof(buffer), format, va);
  va_end(va);

  if (g_assertHandler)
    g_assertHandler(expr, buffer, file, line, hash);
}

/*
void Private::AssertImpl(
  cstring expr,
  cstring file,
  u32 line,
  u32 hash
) {
  char buf[1024];

  snprintf(
    buf,
    sizeof(buf),
    "File: %s, Line: %d\nExpression: %s",
    file,
    line,
    expression);

  MessageBoxA(
    nullptr,
    buf,
    "Assertion failed",
    MB_ICONERROR);
}

void Private::AssertMsgImpl(
  cstring expr,
  cstring file,
  u32 line,
  u32 hash,
  cstring format,
  ...
) {
  char buf1[1024];
  char buf2[1024];

  va_list v;
  va_start(v, msg);
  
  vsnprintf(
    buf2,
    sizeof(buf2),
    msg,
    v);
  
  va_end(v);

  snprintf(
    buf1,
    sizeof(buf1),
    "%s\nFile: %s, Line: %d\nExpression: %s",
    buf2,
    file,
    line,
    expression);

  MessageBoxA(
    nullptr,
    buf1,
    "Assertion failed",
    MB_ICONERROR);
}
*/

void SetAssertHandler(PFN_AssertHandler handler) { g_assertHandler = handler; }
void GetAssertHandler(PFN_AssertHandler *pHandler) { if (pHandler) *pHandler = g_assertHandler; }
