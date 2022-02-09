/****************************************************************
**post-process.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-16.
*
* Description: Processes a parsed (valid) Rds object.
*
*****************************************************************/
#include "post-process.hpp"

// rdsc
#include "rds-util.hpp"

using namespace std;

namespace rds {

namespace {

void set_default_sumtype_features( expr::Rds& rds ) {
  perform_on_item_type<expr::Sumtype>(
      rds, []( expr::Sumtype& sumtype ) {
        if( !sumtype.features.has_value() ) {
          // If features were not specified at all, then give
          // them some sensible default.
          sumtype.features.emplace();
          sumtype.features->insert(
              expr::e_feature::formattable );
          sumtype.features->insert( expr::e_feature::equality );
        }
      } );
}

void set_default_struct_features( expr::Rds& rds ) {
  perform_on_item_type<expr::Struct>(
      rds, []( expr::Struct& strukt ) {
        if( !strukt.features.has_value() ) {
          // If features were not specified at all, then give
          // them some sensible default.
          strukt.features.emplace();
          strukt.features->insert(
              expr::e_feature::formattable );
          strukt.features->insert( expr::e_feature::equality );
        }
      } );
}

} // namespace

void post_process( expr::Rds& rds ) {
  set_default_sumtype_features( rds );
  set_default_struct_features( rds );
}

} // namespace rds