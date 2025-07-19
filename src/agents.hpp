/****************************************************************
**agents.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-09-07.
*
* Description: Stuff for managing agent implementations.
*
*****************************************************************/
#pragma once

// C++ standard library
#include <memory>
#include <unordered_map>

namespace rn {

struct IEngine;
struct IEuroAgent;
struct IGui;
struct INativeAgent;
struct IRand;
struct Planes;
struct SS;

enum class e_tribe;
enum class e_player;

/****************************************************************
** NativeAgents
*****************************************************************/
struct NativeAgents {
  // Use unordered_map instead of enum_map so that we can use a
  // forward-declared enum key.
  using AgentsMap =
      std::unordered_map<e_tribe, std::unique_ptr<INativeAgent>>;

  NativeAgents() = default;

  NativeAgents& operator=( NativeAgents&& ) noexcept;

  // Have this defined in the cpp allows us to use the
  // forward-declared INativeAgent in a unique_ptr.
  ~NativeAgents();

  NativeAgents( AgentsMap agents );

  INativeAgent& operator[]( e_tribe tribe ) const;

 private:
  // We don't use enum map here because it has some constraints
  // that don't work with forward-declared enums.
  AgentsMap agents_;
};

/****************************************************************
** EuroAgents
*****************************************************************/
struct EuroAgents {
  // Use unordered_map instead of enum_map so that we can use a
  // forward-declared enum key.
  using AgentsMap =
      std::unordered_map<e_player, std::unique_ptr<IEuroAgent>>;

  EuroAgents() = default;

  EuroAgents& operator=( EuroAgents&& ) noexcept;

  // Have this defined in the cpp allows us to use the
  // forward-declared IEuroAgent in a unique_ptr.
  ~EuroAgents();

  EuroAgents( AgentsMap agents );

  IEuroAgent& operator[]( e_player player ) const;

  AgentsMap const& map() const;

  void update( e_player const player,
               std::unique_ptr<IEuroAgent> agent );

 private:
  // We don't use enum map here because it has some constraints
  // that don't work with forward-declared enums.
  AgentsMap agents_;
};

/****************************************************************
** Public API.
*****************************************************************/
std::unique_ptr<IEuroAgent> create_euro_agent( IEngine& engine,
                                               SS& ss,
                                               Planes& planes,
                                               IGui& gui,
                                               e_player player );

EuroAgents create_euro_agents( IEngine& engine, SS& ss,
                               Planes& planes, IGui& gui );

NativeAgents create_native_agents( SS& ss, IRand& rand );

} // namespace rn
