/****************************************************************
**ext-parse.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: parz parser extension for cl model.
*
*****************************************************************/
#pragma once

// cl
#include "model.hpp"

// parz
#include "parz/ext.hpp"

namespace cl {

// doc
parz::parser<doc> parser_for( parz::lang<cl_lang>,
                              parz::tag<doc> );

// table
parz::parser<table> parser_for( parz::lang<cl_lang>,
                                parz::tag<table> );

// list
parz::parser<list> parser_for( parz::lang<cl_lang>,
                               parz::tag<list> );

// string_val
parz::parser<string_val> parser_for( parz::lang<cl_lang>,
                                     parz::tag<string_val> );

// boolean
parz::parser<boolean> parser_for( parz::lang<cl_lang>,
                                  parz::tag<boolean> );

// number
parz::parser<number> parser_for( parz::lang<cl_lang>,
                                 parz::tag<number> );

// key_val
parz::parser<key_val> parser_for( parz::lang<cl_lang>,
                                  parz::tag<key_val> );

} // namespace cl
