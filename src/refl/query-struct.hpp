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

// C++ standard library
#include <array>

namespace refl {

/****************************************************************
** Member type list.
*****************************************************************/
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

/****************************************************************
** Reflected Variants
*****************************************************************/
template<typename V>
constexpr auto const& alternative_names() {
  return [&]<ReflectedStruct... Ts>(
             base::variant<Ts...>* ) -> auto const& {
    static constexpr std::array<std::string_view,
                                sizeof...( Ts )>
        kNames{ traits<Ts>::name... };
    return kNames;
  }( (V*){} );
}

/****************************************************************
** Field iteration.
*****************************************************************/
// NOTE: if you are looking for a method for to run over the
// fields of a struct, use trv::traverse:
//
//   traverse( o, [&]( auto& field, string_view const name ) {
//     ...
//   } );
//

} // namespace refl
