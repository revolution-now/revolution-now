/****************************************************************
**golden.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-03-16.
*
* Description: Unit testing helper for serializing and comparing
*              reflected data to/with golden files.
*
*****************************************************************/
#include "golden.hpp"

using namespace std;

namespace rcl {

namespace {

// TODO: may want to dedupe this.
fs::path const& data_dir() {
  static fs::path data{ "test/data" };
  return data;
}

} // namespace

/****************************************************************
** Golden.
*****************************************************************/
fs::path golden_file( string const&          tag,
                      source_location const& loc ) {
  // TODO: need to add support for sub folders of test/.
  auto const stem = fs::path( loc.file_name() ).stem().string();
  static string_view const kSuffix = "-test";
  string_view              short_stem( stem );
  BASE_CHECK( short_stem.ends_with( kSuffix ) );
  short_stem.remove_suffix( kSuffix.size() );
  return data_dir() / "golden" / short_stem /
         fmt::format( "{}.rcl", tag );
}

} // namespace rcl
