/****************************************************************
**expr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-11.
*
* Description: Expression objects for the RDS language.
*
*****************************************************************/
#include "expr.hpp"

// base
#include "base/maybe.hpp"

using namespace std;

namespace rds::expr {

using ::base::maybe;
using ::base::nothing;

maybe<e_feature> feature_from_str( std::string_view feature ) {
  if( feature == "equality" ) return e_feature::equality;
  if( feature == "validation" ) return e_feature::validation;
  if( feature == "offsets" ) return e_feature::offsets;
  if( feature == "nodiscard" ) return e_feature::nodiscard;
  return nothing;
}

} // namespace rds::expr
