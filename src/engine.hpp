/****************************************************************
**engine.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-28.
*
* Description: Controller of the main game engine components.
*
*****************************************************************/
#pragma once

// rds
#include "engine.rds.hpp"

// Revolution Now
#include "iengine.hpp"

// C++ standard library
#include <type_traits>

namespace rn {

/****************************************************************
** Engine
*****************************************************************/
struct Engine : IEngine {
  Engine();
  ~Engine() override;

  void init( e_engine_mode mode );

  void deinit();

 public: // IEngine
  vid::IVideo& video() override;

  sfx::ISfx& sfx() override;

  vid::WindowHandle const& window() override;

  rr::Renderer& renderer_use_only_when_needed() override;

  rr::IRendererSettings& renderer_settings() override;

  gfx::Resolutions& resolutions() override;

  rr::ITextometer& textometer() override;

  void pause() override;

 private:
  // This object is self-referential.
  Engine( Engine&& )      = delete;
  Engine( Engine const& ) = delete;

  struct Impl;
  Impl& impl();
  std::unique_ptr<Impl> pimpl_;
};

static_assert( !std::is_abstract_v<Engine> );

} // namespace rn
