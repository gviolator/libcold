#pragma once

#pragma once

#if defined(_MSC_VER)

#if defined(NETWORK_BUILD)
#define NETWORK_EXPORT __declspec(dllexport)
#else
#define NETWORK_EXPORT __declspec(dllimport)
#endif

#elif __GNUC__ >= 4

#if defined(RUNTIME_BUILD)
#define NETWORK_EXPORT __attribute__((visibility("default")))
#else
#define NETWORK_EXPORT /* nothing */
#endif
#endif



