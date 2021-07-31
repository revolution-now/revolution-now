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

parz::parser<table> parser_for( parz::tag<table> );
parz::parser<value> parser_for( parz::tag<value> );
parz::parser<doc>   parser_for( parz::tag<doc> );

} // namespace cl
