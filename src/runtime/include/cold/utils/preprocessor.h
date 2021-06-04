#pragma once


#define CONCAT_IMPL__(s1, s2) s1##s2

#define CONCATENATE(s1, s2) CONCAT_IMPL__(s1, s2)

#ifdef __COUNTER__
#define ANONYMOUS_VARIABLE_NAME(Prefix) CONCATENATE(Prefix, __COUNTER__)
#else
#define ANONYMOUS_VARIABLE_NAME(Prefix) CONCATENATE(Prefix, _LINE__)
#endif


#define WFILE CONCATENATE(L, __FILE__)
#define WFUNCTION CONCATENATE(L, __FUNCTION__)
