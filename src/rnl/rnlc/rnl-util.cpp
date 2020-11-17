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

using namespace std;

namespace rnl {

void perform_on_sumtypes(
    expr::Rnl*                               rnl,
    tl::function_ref<void( expr::Sumtype* )> func ) {
  for( expr::Item& item : rnl->items ) {
    for( expr::Construct& construct : item.constructs ) {
      switch_( construct ) {
        case_( expr::Sumtype ) { func( &val ); }
        switch_non_exhaustive;
      }
    }
  }
}

void perform_on_sumtypes(
    expr::Rnl const&                               rnl,
    tl::function_ref<void( expr::Sumtype const& )> func ) {
  for( expr::Item const& item : rnl.items ) {
    for( expr::Construct const& construct : item.constructs ) {
      switch_( construct ) {
        case_( expr::Sumtype ) { func( val ); }
        switch_non_exhaustive;
      }
    }
  }
}

} // namespace rnl