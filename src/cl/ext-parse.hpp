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

// string_val
parz::parser<string_val> parser_for( parz::lang<cl_lang>,
                                     parz::tag<string_val> );

// key_val
parz::parser<key_val> parser_for( parz::lang<cl_lang>,
                                  parz::tag<key_val> );

} // namespace cl
