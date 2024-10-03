/****************************************************************
**registrar-helper.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-05.
*
* Description: Helpers used for registering config modules.
*
*****************************************************************/
#pragma once

// config
#include "rds/config-helper.hpp"

// refl
#include "refl/cdr.hpp"

#define INSTANTIATE_CONFIG( ns, name )       \
  detail::empty_registrar register_config(   \
      ::ns::config_##name##_t* o ) {         \
    return register_config_impl( #name, o ); \
  }

namespace rds {

inline cdr::converter::options const& converter_options() {
  static cdr::converter::options opts{
    .allow_unrecognized_fields        = false,
    .default_construct_missing_fields = false,
  };
  return opts;
}

template<cdr::FromCanonical S>
detail::empty_registrar register_config_impl(
    std::string const& name, S* global ) {
  detail::register_config_erased(
      name,
      [global]( cdr::value const& o ) -> PopulatorErrorType {
        UNWRAP_RETURN( res,
                       cdr::run_conversion_from_canonical<S>(
                           o, converter_options() ) );
        *global = std::move( res );
        return base::valid;
      } );
  return {};
}

} // namespace rds
