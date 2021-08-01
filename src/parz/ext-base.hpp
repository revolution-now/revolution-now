/****************************************************************
**ext-base.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-31.
*
* Description: Parser extensions for base library types.
*
*****************************************************************/
#pragma once

// parz
#include "ext.hpp"
#include "promise.hpp"

// base
#include "base/variant.hpp"

namespace parz {

/****************************************************************
** variant
*****************************************************************/
struct VariantParser {
  template<typename... Args>
  parser<base::variant<Args...>> operator()(
      tag<base::variant<Args...>> ) const {
    using res_t = base::variant<Args...>;
    base::maybe<res_t> res;

    auto one = [&]<typename Alt>( Alt* ) -> parser<> {
      if( res.has_value() ) co_return;
      auto exp = co_await Try{ parse<Alt>() };
      if( !exp ) co_return;
      res.emplace( std::move( *exp ) );
    };
    ( co_await one( (Args*)nullptr ), ... );

    if( !res ) co_await fail{};
    co_return std::move( *res );
  }
};

inline constexpr VariantParser variant_parser{};

template<typename... Args>
parser<base::variant<Args...>> parser_for(
    tag<base::variant<Args...>> t ) {
  return variant_parser( t );
}

} // namespace parz
