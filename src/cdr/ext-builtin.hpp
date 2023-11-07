/****************************************************************
**ext-builtin.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-27.
*
* Description: Cdr conversions for builtin types.
*
*****************************************************************/
#pragma once

// cdr
#include "ext.hpp"

// C++ standard library
#include <cstdint>

namespace cdr {

/****************************************************************
** char
*****************************************************************/
value to_canonical( converter& conv, char o, tag_t<char> );
result<char> from_canonical( converter& conv, value const& v,
                             tag_t<char> );

/****************************************************************
** int8_t (not the same as char)
*****************************************************************/
value to_canonical( converter& conv, int8_t o, tag_t<int8_t> );
result<int8_t> from_canonical( converter& conv, value const& v,
                               tag_t<int8_t> );

/****************************************************************
** int16_t (not the same as char)
*****************************************************************/
value to_canonical( converter& conv, int16_t o, tag_t<int16_t> );
result<int16_t> from_canonical( converter& conv, value const& v,
                                tag_t<int16_t> );

/****************************************************************
** int64_t (not the same as char)
*****************************************************************/
value to_canonical( converter& conv, int64_t o, tag_t<int64_t> );
result<int64_t> from_canonical( converter& conv, value const& v,
                                tag_t<int64_t> );

/****************************************************************
** uint8_t (unsigned char most of the time)
*****************************************************************/
value to_canonical( converter& conv, uint8_t o, tag_t<uint8_t> );
result<uint8_t> from_canonical( converter& conv, value const& v,
                                tag_t<uint8_t> );

/****************************************************************
** uint16_t
*****************************************************************/
value            to_canonical( converter& conv, uint16_t o,
                               tag_t<uint16_t> );
result<uint16_t> from_canonical( converter& conv, value const& v,
                                 tag_t<uint16_t> );

/****************************************************************
** uint32_t
*****************************************************************/
value            to_canonical( converter& conv, uint32_t o,
                               tag_t<uint32_t> );
result<uint32_t> from_canonical( converter& conv, value const& v,
                                 tag_t<uint32_t> );

/****************************************************************
** uint64_t
*****************************************************************/
value            to_canonical( converter& conv, uint64_t o,
                               tag_t<uint64_t> );
result<uint64_t> from_canonical( converter& conv, value const& v,
                                 tag_t<uint64_t> );

/****************************************************************
** int
*****************************************************************/
value       to_canonical( converter& conv, int o, tag_t<int> );
result<int> from_canonical( converter& conv, value const& v,
                            tag_t<int> );

/****************************************************************
** bool
*****************************************************************/
value to_canonical( converter& conv, bool o, tag_t<bool> );
result<bool> from_canonical( converter& conv, value const& v,
                             tag_t<bool> );

/****************************************************************
** double
*****************************************************************/
value to_canonical( converter& conv, double o, tag_t<double> );
result<double> from_canonical( converter& conv, value const& v,
                               tag_t<double> );

} // namespace cdr
