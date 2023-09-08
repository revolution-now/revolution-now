/****************************************************************
**minds.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-09-07.
*
* Description: Stuff for managing mind implementations.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// C++ standard library
#include <memory>
#include <unordered_map>

namespace rn {

struct IEuroMind;
struct IGui;
struct INativeMind;
struct IRand;
struct SS;

enum class e_tribe;
enum class e_nation;

/****************************************************************
** NativeMinds
*****************************************************************/
struct NativeMinds {
  using MindsMap =
      std::unordered_map<e_tribe, std::unique_ptr<INativeMind>>;

  NativeMinds() = default;

  NativeMinds& operator=( NativeMinds&& ) noexcept;

  // Have this defined in the cpp allows us to use the
  // forward-declared INativeMInd in a unique_ptr.
  ~NativeMinds();

  NativeMinds( MindsMap minds );

  INativeMind& operator[]( e_tribe tribe ) const;

 private:
  // We don't use enum map here because it has some constraints
  // that don't work with forward-declared enums.
  MindsMap minds_;
};

/****************************************************************
** EuroMinds
*****************************************************************/
struct EuroMinds {
  using MindsMap =
      std::unordered_map<e_nation, std::unique_ptr<IEuroMind>>;

  EuroMinds() = default;

  EuroMinds& operator=( EuroMinds&& ) noexcept;

  // Have this defined in the cpp allows us to use the
  // forward-declared IEuroMind in a unique_ptr.
  ~EuroMinds();

  EuroMinds( MindsMap minds );

  IEuroMind& operator[]( e_nation nation ) const;

 private:
  // We don't use enum map here because it has some constraints
  // that don't work with forward-declared enums.
  MindsMap minds_;
};

/****************************************************************
** Public API.
*****************************************************************/
EuroMinds create_euro_minds( SS& ss, IGui& gui );

NativeMinds create_native_minds( SS& ss, IRand& rand );

} // namespace rn
