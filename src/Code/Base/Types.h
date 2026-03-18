#ifndef __TYPES_HPP__
#define __TYPES_HPP__

#include <stdint.h>

typedef int8_t i08;
typedef uint8_t u08;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

typedef void nil;
typedef void *any;

typedef const char *CString;
typedef CString cstring;

#ifdef __cplusplus

#include <string>

typedef std::string TgcString;

#define rcast(T) reinterpret_cast<T>
#define scast(T) static_cast<T>
#define dcast(T) dynamic_cast<T>

#endif

#endif
