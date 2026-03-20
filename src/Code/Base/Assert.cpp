#include <stdio.h>
#include <Windows.h>
#include "Assert.hpp"

void Private::AssertImpl(
  const char *expression,
  const char *file,
  int line
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
  const char *expression,
  const char *file,
  int line,
  const char *msg,
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
