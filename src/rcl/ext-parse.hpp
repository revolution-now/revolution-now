/****************************************************************
**ext-parse.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: parz parser extension for rcl model.
*
*****************************************************************/
#pragma once

// rcl
#include "model.hpp"

// parz
#include "parz/ext.hpp"

namespace rcl {

// doc
parz::parser<rawdoc> parser_for( parz::lang<rcl_lang>,
                                 parz::tag<rawdoc> );

// table
parz::parser<table> parser_for( parz::lang<rcl_lang>,
                                parz::tag<table> );

// list
parz::parser<list> parser_for( parz::lang<rcl_lang>,
                               parz::tag<list> );

// string_val
parz::parser<string_val> parser_for( parz::lang<rcl_lang>,
                                     parz::tag<string_val> );

// boolean
parz::parser<boolean> parser_for( parz::lang<rcl_lang>,
                                  parz::tag<boolean> );

// number
parz::parser<number> parser_for( parz::lang<rcl_lang>,
                                 parz::tag<number> );

// key_val
parz::parser<key_val> parser_for( parz::lang<rcl_lang>,
                                  parz::tag<key_val> );

} // namespace rcl
