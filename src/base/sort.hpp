/****************************************************************
**sort.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-09-20.
*
* Description: Helpers related to sorting.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// C++ standard library.
#include <algorithm>
#include <vector>

namespace base {

template<typename Container>
auto sorted( Container const& unsorted ) {
  auto res = std::vector( unsorted.begin(), unsorted.end() );
  std::sort( res.begin(), res.end() );
  return res;
}

} // namespace base
