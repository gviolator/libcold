#pragma once
#include "typelist.h"

namespace cold::typelist_internal {

template<typename, typename ...>
struct Append__ ;

template<typename, typename ... >
struct AppendHead__ ;

template<typename ... T, typename ... El>
struct Append__ <TypeList<T ...>, El ...> {
  using type = TypeList<T ..., El ...>;
};

template<typename ... T, typename ... El>
struct AppendHead__ <TypeList<T ...>, El ... > {
  using type = TypeList<El ..., T ...>;
};

}

namespace cold::typelist {

template<typename TL, typename T, typename ... U>
using Append = typename cold::typelist_internal::Append__<TL, T, U ...>::type;

template<typename TL, typename T, typename ... U>
using AppendHead = typename cold::typelist_internal::AppendHead__<TL, T, U ... >::type;
}