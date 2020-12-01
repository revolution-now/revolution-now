/****************************************************************
**tx.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-09.
*
* Description: Texture Wrapper.  This is to shield the rest of
* the codebase from the underlying texture representation as well
* as to provide RAII.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "color.hpp"
#include "coord.hpp"
#include "fmt-helper.hpp"
#include "matrix.hpp"

namespace rn {

class Texture;

enum class e_flip { none, horizontal, vertical };

enum class e_tx_blend_mode {
  none,  // no blending:
         //   dstRGBA = srcRGBA
  blend, // alpha blending:
         //   dstRGB = (srcRGB * srcA) + (dstRGB * (1-srcA))
         //   dstA = srcA + (dstA * (1-srcA))
  add,   // additive blending:
         //   dstRGB = (srcRGB * srcA) + dstRGB
         //   dstA = dstA
  mod,   // color modulate:
         //   dstRGB = srcRGB * dstRGB
         //   dstA = dstA
};

// RAII wrapper and type eraser for SDL Surface.
class Surface {
public:
  explicit Surface( void* sf );
  Surface( Surface&& sf ) noexcept;
  Surface& operator=( Surface&& rhs ) noexcept;

  NON_COPYABLE( Surface );

  ~Surface();

  void const* get() const { return sf_; }
  void*       get() { return sf_; }

  void free();

  // For convenience.
  Delta size() const;
  Rect  rect() const { return Rect::from( Coord{}, size() ); }

  // This will convert a surface to the game's display. NOTE: it
  // will free the input surface if release_input is true.
  void optimize();

  void lock();
  void unlock();

  static Surface load_image( const char* file );

  bool save_png( fs::path const& file );

  static Surface create( Delta delta );

  // Renders this surface to a new surface of the given size,
  // scaling as needed.
  Surface scaled( Delta target_size );

  void swap( Surface& rhs ) noexcept {
    ::std::swap( sf_, rhs.sf_ );
  }

  friend void swap( Surface& lhs, Surface& rhs ) noexcept {
    lhs.swap( rhs );
  }

private:
  friend class Texture;

  void* sf_{ nullptr };
};
NOTHROW_MOVE( Surface );

// RAII wrapper and type eraser for SDL Texture.
class Texture {
public:
  Texture() = default; // try to get rid of this.
  explicit Texture( void* tx );
  Texture( Texture&& tx ) noexcept;
  Texture& operator=( Texture&& rhs ) noexcept;

  NON_COPYABLE( Texture );

  ~Texture();

  void const* get() const { return tx_; }
  void*       get() { return tx_; }

  // NOTE: this will NOT compare the contents of the textures, it
  // will return `false` if the textures are different objects,
  // even if they have the same size and contents. It will return
  // true only if we are comparing the same object with itself,
  // or if we are comparing two textures that both refer to the
  // screen.
  bool operator==( Texture const& rhs ) const;

  void free();

  // Size of the texture in pixels; if this is the screen texture
  // then it will yield logical size.
  Delta size() const;

  Rect rect() const { return Rect::from( Coord{}, size() ); }

  int id() const { return id_; }

  void set_render_target() const;

  Surface to_surface( Opt<Delta> override_size = nothing ) const;

  // Texture that is a proxy for the screen.
  static Texture& screen();
  static Texture  create( Delta size );

  bool is_screen() const { return tx_ == nullptr; }

  static Texture from_surface( Surface const& surface );
  static Texture load_image( const char* file );
  static Texture load_image( fs::path const& path );

  bool save_png( fs::path const& file );

  void set_blend_mode( e_tx_blend_mode mode );
  void set_alpha_mod( uint8_t alpha );

  // WARNING: Very slow function, should not be done in real
  // time. This is because it reads data from a texture.
  Matrix<Color> pixels() const;

  void copy_to( Texture& to ) const;
  void copy_to( Texture& to, Coord target_location ) const;
  void copy_to( Texture& to, Opt<Rect> src,
                Opt<Rect> dst ) const;
  void copy_to( Texture& to, Rect const& src, Rect const& dst,
                double angle, e_flip flip ) const;

  void fill( Color const& color = { 0, 0, 0, 0 } );

  // Estimated memory size (in megabytes) of a texture with
  // given pixel dimensions.
  static double mem_usage_mb( Delta size );
  // Estimated memory size of this texture in megabytes.
  double mem_usage_mb() const;

  void swap( Texture& rhs ) noexcept {
    ::std::swap( tx_, rhs.tx_ );
    ::std::swap( id_, rhs.id_ );
  }

  friend void swap( Texture& lhs, Texture& rhs ) noexcept {
    lhs.swap( rhs );
  }

private:
  void* tx_{ nullptr };
  // globally unique id and monotonically increasing. 0 is for
  // default texture.
  int id_{ 0 };
};
NOTHROW_MOVE( Texture );

/****************************************************************
** Debugging
*****************************************************************/
// Holds the number of Texture objects that hold live textures.
// Monitoring this helps to track texture leaks.
int live_texture_count();

} // namespace rn

DEFINE_FORMAT( ::rn::Texture, "Texture[id={}]", o.id() );
