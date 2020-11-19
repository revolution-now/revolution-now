/****************************************************************
**post-process.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-16.
*
* Description: Processes a parsed (valid) Rnl object.
*
*****************************************************************/
#include "post-process.hpp"

// rnlc
#include "rnl-util.hpp"

using namespace std;

namespace rnl {

namespace {

void set_default_sumtype_features( expr::Rnl& rnl ) {
  perform_on_sumtypes( &rnl, []( expr::Sumtype* sumtype ) {
    if( !sumtype->features.has_value() ) {
      // If features were not specified at all, then give them
      // some sensible default.
      sumtype->features.emplace();
      sumtype->features->push_back(
          expr::e_sumtype_feature::formattable );
      sumtype->features->push_back(
          expr::e_sumtype_feature::equality );
    }
  } );
}

} // namespace

void post_process( expr::Rnl& rnl ) {
  set_default_sumtype_features( rnl );
}

} // namespace rnl