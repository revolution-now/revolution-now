/****************************************************************
**fs.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-05.
*
* Description: std::filesystem wrappers.
*
*****************************************************************/
#include "fs.hpp"

// base
#include "to-str-ext-std.hpp"

using namespace std;

namespace base {

/****************************************************************
** Public API.
*****************************************************************/
valid_or<string> copy_file_overwriting_destination(
    fs::path const& from, fs::path const& to ) {
  auto const options = fs::copy_options::overwrite_existing;
  try {
    if( !fs::copy_file( from, to, options ) )
      return fmt::format(
          "failed to copy file \"{}\" to \"{}\".", from, to );
    return valid;
  } catch( fs::filesystem_error const& e ) {
    return fmt::format(
        "failed to copy file \"{}\" to \"{}\": code: {}, error: "
        "{}",
        from, to, e.code().value(), e.what() );
  } catch( exception const& e ) {
    return fmt::format(
        "failed to copy file \"{}\" to \"{}\": {}", from, to,
        e.what() );
  }
}

} // namespace base
