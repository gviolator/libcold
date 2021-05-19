#pragma once

#if defined(_MSC_VER)
#define ABSTRACT_TYPE __declspec(novtable)
#else
#define ABSTRACT_TYPE
#endif


#define INTERFACE_API ABSTRACT_TYPE
