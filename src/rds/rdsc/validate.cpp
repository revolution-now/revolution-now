/****************************************************************
**validate.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-13.
*
* Description: [FILL ME IN]
*
*****************************************************************/
#include "validate.hpp"

// rdsc
#include "rds-util.hpp"

// base
#include "base/lambda.hpp"

// {fmt}
#include "fmt/format.h"

// C++ standard library.
#include <string_view>
#include <unordered_set>

using namespace std;

namespace rds {

namespace {

struct Validator {
  vector<string> errors_;

  template<typename Arg1, typename... Args>
  void error( string_view fmt_str, Arg1&& arg1,
              Args&&... args ) {
    errors_.push_back( fmt::format( fmt::runtime( fmt_str ),
                                    forward<Arg1>( arg1 ),
                                    forward<Args>( args )... ) );
  }

  void error( string_view msg ) { errors_.emplace_back( msg ); }

  void validate_sumtype( expr::Sumtype const& sumtype ) {
    using F = expr::e_feature;
    if( sumtype.features.has_value() ) {
      unordered_set<F> const& features = *sumtype.features;

      for( expr::e_feature feat : features ) {
        switch( feat ) {
          case expr::e_feature::equality: break;
          case expr::e_feature::offsets: break;
          case expr::e_feature::validation:
            break;
            // case expr::e_feature::some_new_feature:
            //   error( "error msg goes here." );
            //   break;
        }
      }
    }
  }

  void validate_sumtypes( expr::Rds const& rds ) {
    perform_on_item_type<expr::Sumtype>(
        rds, LC( validate_sumtype( _ ) ) );
  }

  void validate_configs( expr::Rds const& rds ) {
    int config_count = 0;
    perform_on_item_type<expr::Config>(
        rds, [&]( expr::Config const& ) { ++config_count; } );
    if( config_count > 1 )
      error(
          "at most one config instantiation is allowed per Rds "
          "file." );
  }
};

} // namespace

vector<string> validate( expr::Rds const& rds ) {
  Validator validator;
  validator.validate_sumtypes( rds );
  validator.validate_configs( rds );
  return move( validator.errors_ );
}

} // namespace rds