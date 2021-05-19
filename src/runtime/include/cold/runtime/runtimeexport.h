#pragma once

#if defined(_MSC_VER)

#if defined(RUNTIME_BUILD)

#define RUNTIME_EXPORT __declspec(dllexport)

#else

#define RUNTIME_EXPORT __declspec(dllimport)

#endif

#elif __GNUC__ >= 4

#if defined(RUNTIME_BUILD)
#define RUNTIME_EXPORT __attribute__((visibility("default")))
#else
#define RUNTIME_EXPORT /* nothing */
#endif
#endif

#define DEFAULT_CALLBACK __cdecl

