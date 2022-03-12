/****************************************************************
**query-struct.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-11.
*
* Description: Helpers for querying struct reflection info.
*
*****************************************************************/
#pragma once

// refl
#include "ext.hpp"

// base
#include "base/meta.hpp"

namespace refl {

namespace detail {

template<typename T>
struct member_type_list_impl;

template<typename... FieldStruct>
struct member_type_list_impl<std::tuple<FieldStruct...>> {
  using type = mp::list<typename FieldStruct::type...>;
};

template<typename T>
using member_type_list_impl_t =
    typename member_type_list_impl<T>::type;

}

template<ReflectedStruct S>
using member_type_list_t = detail::member_type_list_impl_t<
    std::remove_cvref_t<decltype( traits<S>::fields )>>;

} // namespace refl
