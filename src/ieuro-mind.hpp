/****************************************************************
**ieuro-mind.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-31.
*
* Description: Interface for asking for orders and behaviors for
*              european (non-ref) units.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// ss
#include "ss/unit-id.hpp"

// C++ standard library
#include <unordered_set>

namespace rn {

enum class e_nation;

/****************************************************************
** IEuroMind
*****************************************************************/
struct IEuroMind {
  virtual ~IEuroMind() = default;

  virtual e_nation nation() const = 0;
};

/****************************************************************
** NoopEuroMind
*****************************************************************/
// Minimal implementation does not either nothing or the minimum
// necessary to fulfill the contract of each request.
struct NoopEuroMind final : IEuroMind {
  NoopEuroMind( e_nation nation );

  // Implement IEuroMind.
  e_nation nation() const override;

 private:
  e_nation nation_ = {};
};

} // namespace rn
