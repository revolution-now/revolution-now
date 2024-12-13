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

/****************************************************************
** Golden.
*****************************************************************/
fs::path golden_file( string const& tag,
                      source_location const& loc ) {
  auto const rel_src_path = fs::relative(
      loc.file_name(), fs::current_path() / "test" );
  auto const stem = rel_src_path.stem().string();
  auto const sub  = rel_src_path.parent_path();

  static string_view const kSuffix = "-test";
  string_view short_stem( stem );
  CHECK( short_stem.ends_with( kSuffix ) );
  short_stem.remove_suffix( kSuffix.size() );
  static fs::path golden{ "test/data/golden" };
  return golden / sub / short_stem /
         fmt::format( "{}.rcl", tag );
}

} // namespace rcl
