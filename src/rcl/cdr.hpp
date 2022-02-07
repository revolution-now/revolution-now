/****************************************************************
**cdr.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-07.
*
* Description: Conversion from Rcl model to Cdr model.
*
*****************************************************************/
#pragma once

// rcl
#include "model.hpp"

// cdr
#include "cdr/ext.hpp"

namespace rcl {

// Convert an rcl::value to a cdr::value.
cdr::value to_canonical( cdr::converter&   conv,
                         rcl::value const& o,
                         cdr::tag_t<rcl::value> );

// Convert an cdr::value to an rcl::value. This will never fail;
// it only returns a cdr::result because that is the required
// type signature.
//
// WARNING: after calling this, you must extract the top-level
// table from the value and put it into an rcl::doc so that it
// can run the post-processing on it, which is required for the
// tables and lists to maintain their invariants.
cdr::result<rcl::value> from_canonical( cdr::converter&   conv,
                                        cdr::value const& v,
                                        cdr::tag_t<rcl::value> );

} // namespace rcl
