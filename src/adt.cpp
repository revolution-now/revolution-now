/****************************************************************
**adt.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-18.
*
* Description: Algebraic Data Types.
*
*****************************************************************/
#include "adt.hpp"

// Revolution Now
#include "fmt-helper.hpp"
#include "logging.hpp"
#include "variant.hpp"

// base-util
#include "base-util/string.hpp"

namespace rn {

std::string_view remove_rn_ns( std::string_view sv ) {
  constexpr std::string_view rn_ = "rn::";
  if( util::starts_with( sv, rn_ ) )
    sv.remove_prefix( rn_.size() );
  return sv;
}

namespace {} // namespace

} // namespace rn
