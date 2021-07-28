/****************************************************************
**rds-util.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-16.
*
* Description: Utilities for the RDS compiler.
*
*****************************************************************/
#include "rds-util.hpp"

// base
#include "base/function-ref.hpp"
#include "base/meta.hpp"

using namespace std;

namespace rds {

void perform_on_sumtypes(
    expr::Rds*                                 rds,
    base::function_ref<void( expr::Sumtype* )> func ) {
  for( expr::Item& item : rds->items ) {
    for( expr::Construct& construct : item.constructs ) {
      std::visit( mp::overload{ [&]( expr::Sumtype& sumtype ) {
                                 func( &sumtype );
                               },
                                []( auto const& ) {} },
                  construct );
    }
  }
}

void perform_on_sumtypes(
    expr::Rds const&                                 rds,
    base::function_ref<void( expr::Sumtype const& )> func ) {
  for( expr::Item const& item : rds.items ) {
    for( expr::Construct const& construct : item.constructs ) {
      std::visit(
          mp::overload{ [&]( expr::Sumtype const& sumtype ) {
                         func( sumtype );
                       },
                        []( auto const& ) {} },
          construct );
    }
  }
}

} // namespace rds