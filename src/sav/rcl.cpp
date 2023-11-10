/****************************************************************
**rcl.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-07.
*
* Description: API for loading and saving games in the classic
*              format of the OG but represented in rcl/json.
*
*****************************************************************/
#include "rcl.hpp"

// sav
#include "error.hpp"
#include "rcl/model.hpp"
#include "sav-struct.hpp"

// rcl
#include "rcl/emit.hpp"
#include "rcl/parse.hpp"

// cdr
#include "cdr/converter.hpp"
#include "cdr/ext-base.hpp"
#include "cdr/ext-builtin.hpp"
#include "cdr/ext-std.hpp"

// base
#include "base/function-ref.hpp"
#include "base/timer.hpp"

// C++ standard library
#include <fstream>

using namespace std;

namespace sav {

namespace {

using ::base::expect;
using ::base::function_ref;
using ::base::valid;
using ::base::valid_or;

using LoadFn = function_ref<expect<rcl::doc>(
    rcl::ProcessingOptions const& )>;

valid_or<string> load_rcl_impl( ColonySAV& out, LoadFn load ) {
  cdr::converter::options const cdr_opts{
      .allow_unrecognized_fields        = false,
      .default_construct_missing_fields = true,
  };
  base::ScopedTimer timer( "load-ColonySAV-from-rcl" );
  timer.checkpoint( "rcl parse" );
  rcl::ProcessingOptions const proc_opts{
      .run_key_parse = true, .unflatten_keys = true };
  UNWRAP_RETURN( rcl_doc, load( proc_opts ) );
  timer.checkpoint( "from_canonical" );
  UNWRAP_RETURN( converted,
                 run_conversion_from_canonical<ColonySAV>(
                     rcl_doc.top_val(), cdr_opts ) );
  out = std::move( converted );
  return valid;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
valid_or<string> load_rcl_from_string(
    string const& filename_for_error_reporting, string const& in,
    ColonySAV& out ) {
  return load_rcl_impl(
      out, [&]( rcl::ProcessingOptions const& proc_opts ) {
        return rcl::parse( filename_for_error_reporting, in,
                           proc_opts );
      } );
}

valid_or<string> load_rcl_from_file( std::string const& path,
                                     ColonySAV&         out ) {
  return load_rcl_impl(
      out, [&]( rcl::ProcessingOptions const& proc_opts ) {
        return rcl::parse_file( path, proc_opts );
      } );
}

string save_rcl_to_string( ColonySAV const& in,
                           e_rcl_dialect    dialect ) {
  cdr::converter::options const cdr_opts{
      .write_fields_with_default_value = true };
  base::ScopedTimer timer( "save-ColonySAV-to-rcl" );
  timer.checkpoint( "to_canonical" );
  cdr::value cdr_val =
      cdr::run_conversion_to_canonical( in, cdr_opts );
  cdr::converter conv( cdr_opts );
  UNWRAP_CHECK( tbl, conv.ensure_type<cdr::table>( cdr_val ) );
  rcl::ProcessingOptions proc_opts{ .run_key_parse  = false,
                                    .unflatten_keys = false };
  timer.checkpoint( "create doc" );
  UNWRAP_CHECK(
      rcl_doc, rcl::doc::create( std::move( tbl ), proc_opts ) );
  timer.checkpoint( "emit rcl" );
  switch( dialect ) {
    case e_rcl_dialect::standard: {
      rcl::EmitOptions const emit_opts{
          .flatten_keys = true,
      };
      return rcl::emit( rcl_doc, emit_opts );
    }
    case e_rcl_dialect::json:
      rcl::JsonEmitOptions const emit_opts{ .key_order_tag =
                                                "__key_order" };
      return rcl::emit_json( rcl_doc, emit_opts );
  }
}

valid_or<string> save_rcl_to_file( string const&    path,
                                   ColonySAV const& in,
                                   e_rcl_dialect    dialect ) {
  string const rcl_output = save_rcl_to_string( in, dialect );
  ofstream     out{ path };
  if( !out.good() )
    return fmt::format( "failed to open {} for writing.", path );
  // out << "# TODO: title."
  //     << "\n";
  out << rcl_output;
  return valid;
}

} // namespace sav
