/****************************************************************
**concepts.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-31.
*
* Description: Concepts used by the parz library.
*
*****************************************************************/
#pragma once

namespace parz {

template<typename T>
concept Parser = requires {
  typename T::value_type;
};

} // namespace parz
