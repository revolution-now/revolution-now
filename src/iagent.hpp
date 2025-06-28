/****************************************************************
**iagent.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-08-12.
*
* Description: Base interface for the I*Agent interfaces.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "wait.hpp"

namespace rn {

/****************************************************************
** IAgent
*****************************************************************/
struct IAgent {
  IAgent() = default;

  virtual ~IAgent() = default;

  virtual wait<> message_box( std::string const& msg ) = 0;

  // For convenience.  Should not be overridden.
  template<typename Arg, typename... Rest>
  wait<> message_box(
      // The type_identity prevents the compiler from using the
      // first arg to try to infer Arg/Rest (which would fail);
      // it will defer that, then when it gets to the end it will
      // have inferred those parameters through other args.
      fmt::format_string<std::type_identity_t<Arg>, Rest...> fmt,
      Arg&& arg, Rest&&... rest ) {
    return message_box(
        fmt::format( fmt, std::forward<Arg>( arg ),
                     std::forward<Rest>( rest )... ) );
  }
};

/****************************************************************
** NoopAgent
*****************************************************************/
// Minimal implementation does not either nothing or the minimum
// necessary to fulfill the contract of each request.
struct NoopAgent final : IAgent {
  NoopAgent() = default;

  // Implement IAgent.
  wait<> message_box( std::string const& msg ) override;
};

} // namespace rn
