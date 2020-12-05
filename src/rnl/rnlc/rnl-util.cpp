/****************************************************************
**rnl-util.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-16.
*
* Description: Utilities for the RNL compiler.
*
*****************************************************************/
#include "rnl-util.hpp"

// base
#include "base/meta.hpp"

// Abseil
#include "absl/functional/function_ref.h"

using namespace std;

namespace rnl {

void perform_on_sumtypes(
    expr::Rnl*                                rnl,
    absl::FunctionRef<void( expr::Sumtype* )> func ) {
  for( expr::Item& item : rnl->items ) {
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
    expr::Rnl const&                                rnl,
    absl::FunctionRef<void( expr::Sumtype const& )> func ) {
  for( expr::Item const& item : rnl.items ) {
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

} // namespace rnl