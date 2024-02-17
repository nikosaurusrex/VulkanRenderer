#ifndef COMMON_H
#define COMMON_H

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <any>
#include <set>
#include <string>
#include <typeindex>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

using string = std::string;

template <typename T, typename U = std::allocator<T>>
using array = std::vector<T, U>;

template <typename K, typename V>
using map = std::unordered_map<K, V>;

template <typename A, typename B>
using pair = std::pair<A, B>;

template <typename T>
using queue = std::queue<T>;

template <typename T>
using set = std::set<T>;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t  s64;
typedef int32_t  s32;
typedef int16_t  s16;
typedef int8_t   s8;

typedef float   f32;
typedef double  f64;

void LogFatal(const char *format, ...);
void LogError(const char *format, ...);
void LogDev(const char *format, ...);
void LogInfo(const char *format, ...);

char *ReadEntireFile(const char *file_path);

#endif
