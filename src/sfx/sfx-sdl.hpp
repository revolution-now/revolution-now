/****************************************************************
**sfx-sdl.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-27.
*
* Description: Implementation of ISfx using an SDL Mixer backend.
*
*****************************************************************/
#pragma once

// sfx
#include "isfx.hpp"

namespace sfx {

/****************************************************************
** SfxSDL
*****************************************************************/
struct SfxSDL : ISfx {
  SfxSDL();
  ~SfxSDL() override;

 public: // ISfx
  void play_sound_effect( rn::e_sfx sound ) const override;

 public:
  // Must be called before this is used.
  void init_mixer();
  void deinit_mixer();
  // Must be called before this is used.
  void load_all_sfx();
  void free_all_sfx();

 private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
};

static_assert( !std::is_abstract_v<SfxSDL> );

} // namespace sfx
