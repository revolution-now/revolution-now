/****************************************************************
**user-config.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-17.
*
* Description: Main implementation of IUserConfig.
*
*****************************************************************/
#include "user-config.hpp"

// config
#include "config/user-engine-only.rds.hpp"
// NOTE: ^^^ this is the only place where this should be in-
// cluded, apart from the registrar.

// rcl
#include "error.hpp"
#include "rcl/emit.hpp"
#include "rcl/parse.hpp"

// refl
#include "refl/cdr.hpp"

// cdr
#include "cdr/converter.hpp"
#include "cdr/ext-builtin.hpp"
#include "cdr/ext.hpp"
#include "cdr/merge.hpp"

// base
#include "base/io.hpp"
#include "base/logger.hpp"

// C++ standard library
#include <fstream>

using namespace std;

namespace rn {

namespace {

using ::base::expect;
using ::base::function_ref;
using ::base::valid;
using ::base::valid_or;

/****************************************************************
** Helpers.
*****************************************************************/
void write_preamble( ostream& out ) {
  out << R"(# NOTE: This file is AUTO-GENERATED. If you want to edit it,
# please read below.
#
# Contents
# --------
# This file contains the user preferences that are changeable via
# in-game menus but that persist across sessions and games. Exam-
# ples are graphics or sound preferences, as opposed to static
# settings such as the combat strength of a unit.
#
# Please see the user.rcl file for documentation on these fields.
#
# Generating/Changing
# -------------------
# These settings are changeable in-game, and when that happens
# they will be written to this file, overwriting any changes that
# have been made manually. Therefore, you generally don't need
# or want to edit this file directly.
#
# The first time the game is run this file is auto-generated from
# the template in the user.rcl config file. Subsequently, the
# game loads this file on startup and writes changes to it when
# the player makes changes to the relevant settings in-game.
#
# Thus, if you delete this file it will be regenerated with the
# default settings on the next game run.

)";
}

valid_or<string> flush_impl( config_user_t const& conf,
                             string const& path ) {
  lg.info( "saving user settings to file {}.", path );
  CHECK( conf.validate() );
  cdr::converter::options const options{
    .write_fields_with_default_value = true };
  cdr::value cdr_val =
      cdr::run_conversion_to_canonical( conf, options );
  UNWRAP_RETURN(
      tbl, cdr::converter{}.ensure_type<cdr::table>( cdr_val ) );
  UNWRAP_RETURN( rcl_doc, rcl::doc::create( std::move( tbl ) ) );
  string const body = rcl::emit(
      rcl_doc, rcl::EmitOptions{ .flatten_keys = false } );
  // Open file last.
  ofstream out( path );
  if( !out.good() )
    return format( "failed to open {} for writing.", path );
  write_preamble( out );
  out << body;
  return valid;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
UserConfig::UserConfig() {
  // Do this just to put some valid values into the config. But
  // we may later override this from what is read from the file.
  load_from_defaults();
}

config_user_t const& UserConfig::read() const { return config_; }

bool UserConfig::modify( function_ref<WriteFn> const fn ) {
  auto copy = config_;
  fn( copy );
  if( copy == config_ ) return true;
  if( !copy.validate() ) return false;
  config_ = copy;
  return true;
}

valid_or<string> UserConfig::flush() {
  if( !settings_file_.has_value() ) return valid;
  if( config_ == settings_file_->last_snapshot &&
      settings_file_->gaps_filled == 0 )
    return valid;
  if( auto const ok =
          flush_impl( config_, settings_file_->path );
      !ok )
    return format( "failed to save user settings to file {}: {}",
                   settings_file_->path, ok.error() );
  settings_file_->last_snapshot = config_;
  return valid;
}

auto UserConfig::load_from_file( string const& path )
    -> expect<SettingsFile> {
  lg.info( "loading user settings from file {}.", path );
  auto const rcl_str = base::read_text_file_as_string( path );
  if( !rcl_str )
    return error_read_text_file_msg( path, rcl_str.error() );
  UNWRAP_RETURN( rcl_doc, rcl::parse( path, *rcl_str ) );
  auto user_tbl = rcl_doc.top_tbl();

  int const num_missing = [&] {
    cdr::value const static_val =
        cdr::run_conversion_to_canonical(
            config_user,
            cdr::converter::options{
              .write_fields_with_default_value = true } );
    UNWRAP_CHECK_T( cdr::table const& static_tbl,
                    static_val.get_if<cdr::table>() );
    int const num_missing =
        cdr::right_join( user_tbl, static_tbl );
    return num_missing;
  }();
  if( num_missing > 0 )
    lg.warn(
        "filled in {} fields which were not present in user "
        "settings file with their default values.",
        num_missing );

  cdr::value val( std::move( user_tbl ) );

  cdr::converter::options const options{
    .allow_unrecognized_fields = true,
    // It's important that this be false because the default con-
    // structed values of the fields are not necessarily the cor-
    // rect default values. This will cause a failure if we are
    // missing a field. However, we should not be missing a field
    // because we should have filled in missing fields with their
    // default values with the table merge above. So this we will
    // find any problems with that process.
    .default_construct_missing_fields = false,
  };
  // Once again just because it is important.
  CHECK( options.default_construct_missing_fields == false );
  UNWRAP_RETURN(
      config, cdr::run_conversion_from_canonical<config_user_t>(
                  val, options ) );
  GOOD_OR_RETURN( config.validate() );
  return SettingsFile{ .gaps_filled   = num_missing,
                       .path          = path,
                       .last_snapshot = std::move( config ) };
}

valid_or<string> UserConfig::try_bind_to_file(
    string const& path ) {
  if( !fs::exists( path ) ) {
    // The settings file does not exist, which might happen ei-
    // ther the first time the game is run, or if the user has
    // deleted the file. Try to create the file with defaults.
    auto const ok = flush_impl( config_, path );
    if( !ok )
      return format(
          "failed to bind to user settings file {}: {}", path,
          ok.error() );
    settings_file_.emplace(
        SettingsFile{ .gaps_filled   = 0,
                      .path          = path,
                      .last_snapshot = config_ } );
  } else {
    // The user settings file already exists.
    auto const settings_file = load_from_file( path );
    if( !settings_file )
      // We could rewrite the defaults here, but there's probably
      // no point because if we do that then we'll just be using
      // the defaults for this game session, which we are now
      // doing anyway. And not rewriting the file at least gives
      // the player the ability to fix it.
      return format(
          "failed to load from existing user settings file {}: "
          "{}",
          path, settings_file.error() );
    settings_file_.emplace( std::move( *settings_file ) );
  }
  CHECK( settings_file_.has_value() );
  config_ = settings_file_->last_snapshot;
  return valid;
}

void UserConfig::load_from_defaults() {
  lg.info( "loading user settings from defaults." );
  config_ = config_user;
  // Should have been validated when the config_user defaults
  // were loaded.
  CHECK( config_.validate() );
}

} // namespace rn
