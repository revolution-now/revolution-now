/****************************************************************
**io.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-03.
*
* Description: File IO utilities.
*
*****************************************************************/
#pragma once

// base
#include "expect.hpp"
#include "fs.hpp"
#include "maybe.hpp"

// C++ standard library
#include <memory>
#include <string>

namespace base {

enum class e_error_read_text_file {
  file_does_not_exist,
  alloc_failure, // failed to allocate memory for buffer.
  open_file_failure,
  incomplete_read // failed to read all bytes in file.
};

/****************************************************************
** Read ASCII text file to null-terminated C string.
*****************************************************************/
// This file assumes it is reading in an ascii text file (UTF-8
// should work also, but will not be validated) where line end-
// ings may be converted during the reading process to '\n'.
// Also, the file is expected to not have any '\0's in it, al-
// though this is not checked.
//
// If the `o_size` parameter is provided, it will be populated
// with the length of the resulting C string as would be computed
// by ::strlen. This may be different (less) than the number of
// bytes read in from the file due to line ending conversions.
// (windows line endings are converted to UNIX line endings).
//
// The returned buffer will have a null character attached to the
// end, so can be used as a C string, but the size returned will
// not include this zero.
//
// If the return `expect` has a value, then the unique_ptr will
// be non-null, and its result can be extracted as a
// null-terminated C string like so:
//
//   size_t size;
//   auto data = read_text_file( "/a/b/c.txt", size );
//   CHECK( data.has_value() );
//   char const* c_string = data->get();
//
// If the returned value is an error then the size output para-
// meter will not be changed.
//
expect<std::unique_ptr<char[]>, e_error_read_text_file>
read_text_file( fs::path const& p,
                maybe<size_t&>  o_size = nothing );

expect<std::string, e_error_read_text_file>
read_text_file_as_string( fs::path const& p );

} // namespace base