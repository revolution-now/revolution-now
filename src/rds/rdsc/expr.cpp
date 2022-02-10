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

/****************************************************************
** sumtype
*****************************************************************/
maybe<e_feature> feature_from_str( std::string_view feature ) {
  if( feature == "serializable" ) return e_feature::serializable;
  if( feature == "equality" ) return e_feature::equality;
  if( feature == "validation" ) return e_feature::validation;
  return nothing;
}

} // namespace rds::expr
