/****************************************************************
**serial.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-28.
*
* Description: Serialization.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "enum.hpp"
#include "errors.hpp"

// base-util
#include "base-util/macros.hpp"
#include "base-util/pp.hpp"

// Flatbuffers
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/minireflect.h"

// C++ standard library
#include <cstddef>

namespace fb = ::flatbuffers;

namespace rn::serial {

#define SERIALIZABL_SAVE_ONE( name ) \
  ar.save( TO_STRING( name ), DEFER( name ) )
#define SERIALIZABL_SIZE_CHECK( name )                   \
  decltype( DEFER( name ) ) PP_JOIN( name, __ );         \
  static_assert( !std::is_reference_v<decltype( name )>, \
                 "cannot serialize references." )

#define SERIALIZABL_IMPL( ... )                            \
  std::byte last_member__;                                 \
  /* This goes out here so it doesn't depend on the */     \
  /* Archiver template argument, that way the static */    \
  /* assert can be checked while compiling the header */   \
  /* and does not need to wait until instantiation of */   \
  /* the serialize method in a cpp. */                     \
  struct serialize_size_checker__ {                        \
    PP_MAP_SEMI( SERIALIZABL_SIZE_CHECK, __VA_ARGS__ )     \
    std::byte last_member__;                               \
  };                                                       \
  template<typename Archiver>                              \
  void serialize( Archiver&& ar ) const {                  \
    PP_MAP_SEMI( SERIALIZABL_SAVE_ONE, __VA_ARGS__ )       \
    using Parent_t = std::decay_t<decltype( *this )>;      \
    static_assert( offsetof( Parent_t, last_member__ ) ==  \
                       offsetof( serialize_size_checker__, \
                                 last_member__ ),          \
                   "some members are missing from the "    \
                   "serialization list." );                \
  }

#define SERIALIZABLE( ... ) \
  EVAL( SERIALIZABL_IMPL( __VA_ARGS__ ) )

struct SaveableChild {
  Str         s{"test"};
  int         i{5};
  char        cc{'g'};
  Vec<double> vd{5.1};

  // Must be last.
  SERIALIZABLE( s, i, cc, vd );
};

struct SaveableParent {
  int                x{3};
  int                y{42};
  SaveableChild      c{};
  Vec<double>        ds{3.14, 2.71};
  Vec<SaveableChild> cs{{}, {}};

  // Must be last.
  SERIALIZABLE( x, y, c, ds, cs );
};

struct A {
  int m{-1};
};

template<typename Archiver>
inline void serialize( Archiver&& ar, A const& a ) {
  ar.save( "m", a.m );
}

enum class e_( saveable, //
               red,      //
               blue,     //
               green     //
);

struct SaveableOmni {
  A                   a;
  Vec<SaveableParent> os{{}, {}, {}};
  e_saveable          color{e_saveable::blue};

  // Must be last.
  SERIALIZABLE( a, os, color );
};

/****************************************************************
** Binary Blobs
*****************************************************************/
class ByteBuffer {
  using byte_t = uint8_t;

public:
  ByteBuffer( byte_t* buf, int size )
    : size_( size ), buf_( buf ) {}

  static expect<ByteBuffer> read( fs::path const& file );

  int size() const { return size_; }

  byte_t const* get() const { return buf_.get(); }
  byte_t*       get() { return buf_.get(); }

private:
  int                       size_{0};
  std::unique_ptr<byte_t[]> buf_{};
};

class BinaryBlob {
public:
  BinaryBlob( uint8_t* buf, int size, int offset )
    : buf_( buf, size ), offset_( offset ) {}

  BinaryBlob( ByteBuffer&& buf )
    : buf_( std::move( buf ) ), offset_( 0 ) {}

  static expect<BinaryBlob> read( fs::path const& path );

  expect<> write( fs::path const& path ) const;

  // Must move argument.
  static BinaryBlob from_builder(
      fb::FlatBufferBuilder builder );

  template<typename FB>
  std::string to_json() const {
    fb::ToStringVisitor tostring_visitor( //
        /*delimiter=*/"\n",               //
        /*quotes=*/true,                  //
        /*indent=*/"  ",                  //
        /*vdelimited=*/true               //
    );
    fb::IterateFlatBuffer( get(), FB::MiniReflectTypeTable(),
                           &tostring_visitor );
    return tostring_visitor.s;
  }

  uint8_t const* get() const { return buf_.get() + offset_; }
  uint8_t*       get() { return buf_.get() + offset_; }

  int size() const { return buf_.size() - offset_; }

private:
  ByteBuffer buf_;
  int        offset_;
};

/****************************************************************
** Testing
*****************************************************************/
void test_serial();

} // namespace rn::serial
