/****************************************************************
**string.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-28.
*
* Description: String manipulation utilities.
*
*****************************************************************/
#pragma once

#include "config.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace base {

// Trim leading and trailing whitespace.
std::string trim( std::string_view sv );

// Capitalize the first letter of each word.
std::string capitalize_initials( std::string_view sv );

// Replace all occurrences of `from` with `to` in `sv`.
std::string str_replace( std::string_view sv,
                         std::string_view from,
                         std::string_view to );

// Iterate through the from/to pairs and call str_replace.
std::string str_replace_all(
    std::string_view sv,
    std::initializer_list<std::pair<std::string, std::string>>
        pairs );

// Split a string on a character.
std::vector<std::string> str_split( std::string_view sv,
                                    char c );

// Split a string on any character from the list. This does not
// split on the `chars` string as a whole, it splits on any of
// the individual characters in the `chars`.
std::vector<std::string> str_split_on_any(
    std::string_view sv, std::string_view chars );

// Joins the elements in `what` with `sep` between them.
std::string str_join( std::vector<std::string> const& v,
                      std::string_view const sep );

// Is `needle` in `haystack`.
bool str_contains( std::string_view haystack,
                   std::string_view needle );

// For an ascii string (not UTF-8) this will convert all charac-
// ters to lowercase.
std::string ascii_str_to_lower( std::string_view sv );

} // namespace base