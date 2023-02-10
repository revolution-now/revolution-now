/****************************************************************
**markup.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-09.
*
* Description: Unit tests for the src/markup.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/markup.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::base::expect;
using ::base::unexpected;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[markup] parse_markup" ) {
  string_view          input;
  expect<MarkedUpText> expected = unexpected<MarkedUpText>( "" );

  auto f = [&] { return parse_markup( input ); };

  input    = "";
  expected = MarkedUpText{ .chunks = {} };
  REQUIRE( f() == expected );

  input    = "a";
  expected = MarkedUpText{
      .chunks = {
          { { .text = "a", .style = MarkupStyle{} } } } };
  REQUIRE( f() == expected );

  input    = "ab";
  expected = MarkedUpText{
      .chunks = {
          { { .text = "ab", .style = MarkupStyle{} } } } };
  REQUIRE( f() == expected );

  input    = "abcd efgh";
  expected = MarkedUpText{
      .chunks = { { { .text  = "abcd efgh",
                      .style = MarkupStyle{} } } } };
  REQUIRE( f() == expected );

  input    = "[]";
  expected = MarkedUpText{};
  REQUIRE( f() == expected );

  input    = "a[]";
  expected = MarkedUpText{ .chunks = { { { .text = "a" } } } };
  REQUIRE( f() == expected );

  input    = "[]a";
  expected = MarkedUpText{ .chunks = { { { .text = "a" } } } };
  REQUIRE( f() == expected );

  input    = "[a]";
  expected = MarkedUpText{
      .chunks = { { { .text  = "a",
                      .style = { .highlight = true } } } } };
  REQUIRE( f() == expected );

  input    = "[ab]";
  expected = MarkedUpText{
      .chunks = { { { .text  = "ab",
                      .style = { .highlight = true } } } } };
  REQUIRE( f() == expected );

  input    = "[a]b";
  expected = MarkedUpText{
      .chunks = {
          { { .text = "a", .style = { .highlight = true } },
            { .text = "b" } } } };
  REQUIRE( f() == expected );

  input    = "a[b]";
  expected = MarkedUpText{
      .chunks = { { { .text = "a" },
                    { .text  = "b",
                      .style = { .highlight = true } } } } };
  REQUIRE( f() == expected );

  input    = "[a][b]";
  expected = MarkedUpText{
      .chunks = {
          { { .text = "a", .style = { .highlight = true } },
            { .text  = "b",
              .style = { .highlight = true } } } } };
  REQUIRE( f() == expected );

  input    = "[a]bc[d e]fg[h]";
  expected = MarkedUpText{
      .chunks = {
          { { .text = "a", .style = { .highlight = true } },
            { .text = "bc" },
            { .text = "d e", .style = { .highlight = true } },
            { .text = "fg" },
            { .text  = "h",
              .style = { .highlight = true } } } } };
  REQUIRE( f() == expected );

  input    = "[a][bc][d e][fg][h]";
  expected = MarkedUpText{
      .chunks = {
          { { .text = "a", .style = { .highlight = true } },
            { .text = "bc", .style = { .highlight = true } },
            { .text = "d e", .style = { .highlight = true } },
            { .text = "fg", .style = { .highlight = true } },
            { .text  = "h",
              .style = { .highlight = true } } } } };
  REQUIRE( f() == expected );

  input    = "[abcd efgh]";
  expected = MarkedUpText{
      .chunks = { { { .text  = "abcd efgh",
                      .style = { .highlight = true } } } } };
  REQUIRE( f() == expected );

  input    = "[abcd efgh][]";
  expected = MarkedUpText{
      .chunks = { { { .text  = "abcd efgh",
                      .style = { .highlight = true } } } } };
  REQUIRE( f() == expected );

  input    = "abcd [ef]gh";
  expected = MarkedUpText{
      .chunks = {
          { { .text = "abcd " },
            { .text = "ef", .style = { .highlight = true } },
            { .text = "gh" } } } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[markup] remove_markup" ) {
  auto f = remove_markup;

  REQUIRE( f( "" ) == "" );
  REQUIRE( f( "[]" ) == "" );
  REQUIRE( f( "[abc]" ) == "abc" );
  REQUIRE( f( "[a]bc[d e]fg[h]" ) == "abcd efgh" );
  REQUIRE( f( "abc[d e]fgh" ) == "abcd efgh" );
}

} // namespace
} // namespace rn
