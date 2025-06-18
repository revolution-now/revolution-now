/****************************************************************
**golden.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-03-16.
*
* Description: Unit testing helper for serializing and comparing
*              reflected data to/with golden files.
*
*****************************************************************/
#pragma once

// rcl
#include "rcl/emit.hpp"
#include "rcl/parse.hpp"

// cdr
#include "cdr/converter.hpp"
#include "cdr/ext.hpp"

// base
#include "base/fs.hpp"
#include "base/io.hpp"
#include "base/to-str-ext-std.hpp"

// C++ standard library
#include <fstream>

namespace rcl {

/****************************************************************
** Golden.
*****************************************************************/
// This is a helper class used only in unit tests in order to
// make it as easy as possible to write a test case where a re-
// flected data structure (generated during the test) is compared
// against a "golden" file containing an rcl representation of
// what the data is supposed to look like.
//
// The class will detect the module name of the unit test and
// will choose an appropriate folder and file name to hold the
// serialized form of the object. If the golden file does not
// exist then it will be written with the current value of the
// data and success will be reported. If the file already exists
// then its contents will be read and compared with the live
// value of the data. If the live version does not match the
// golden file then the live version will be written to another
// file alongside the golden file for easy inspection. If you
// want to replace the golden file with the current value, just
// delete the golden file and it will be replaced.
//
// See this module's unit test for an example of usage.
//
template<cdr::Canonical T>
struct Golden {
  Golden( T const& ref ATTR_LIFETIMEBOUND,
          std::string const& tag,
          std::source_location loc =
              std::source_location::current() );

  base::valid_or<std::string> is_golden() const;

 private:
  T load_from_golden() const;
  void save_to_golden( fs::path const& p ) const;

 private:
  T const& ref_;
  fs::path file_;
};

/****************************************************************
** Public API.
*****************************************************************/
// This should be called from a unit test module and will return
// the golden file to use given a tag/stem. The aforementioned
// tag/stem should be unique to a given test case.
fs::path golden_file( std::string const& tag,
                      std::source_location const& loc );

/****************************************************************
** Golden Implementation.
*****************************************************************/
template<cdr::Canonical T>
Golden<T>::Golden( T const& ref, std::string const& tag,
                   std::source_location loc )
  : ref_( ref ), file_( golden_file( tag, loc ) ) {}

template<cdr::Canonical T>
base::valid_or<std::string> Golden<T>::is_golden() const {
  if( !fs::exists( file_ ) ) {
    save_to_golden( file_ );
    return base::valid;
  }
  T const golden = load_from_golden();
  if( golden == ref_ ) return base::valid;
  fs::path new_output_file = file_;
  new_output_file += ".new";
  save_to_golden( new_output_file );
  return fmt::format(
      "{}\n\n!=\n\n{}\n\nNew output written to file {}.", ref_,
      golden, new_output_file );
}

template<cdr::Canonical T>
T Golden<T>::load_from_golden() const {
  UNWRAP_CHECK( rcl_str, base::read_text_file_as_string(
                             file_.string() ) );
  UNWRAP_CHECK( rcl_doc, rcl::parse( file_.string(), rcl_str ) );
  cdr::converter::options const options{
    .allow_unrecognized_fields        = true,
    .default_construct_missing_fields = true,
  };
  UNWRAP_CHECK( o, cdr::run_conversion_from_canonical<T>(
                       rcl_doc.top_val(), options ) );
  return std::move( o );
}

template<cdr::Canonical T>
void Golden<T>::save_to_golden( fs::path const& p ) const {
  cdr::converter::options const options{
    .write_fields_with_default_value = false };
  cdr::value cdr_val =
      cdr::run_conversion_to_canonical( ref_, options );
  UNWRAP_CHECK(
      tbl, cdr::converter{}.ensure_type<cdr::table>( cdr_val ) );
  UNWRAP_CHECK( rcl_doc, rcl::doc::create( std::move( tbl ) ) );
  std::string const body = rcl::emit( rcl_doc );
  // Open file last.
  std::ofstream out( p );
  BASE_CHECK( out.good(), "failed to open {} for writing.", p );
  out << body;
}

} // namespace rcl
