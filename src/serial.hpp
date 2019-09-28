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

// base-util
#include "base-util/macros.hpp"
#include "base-util/pp.hpp"

// C++ standard library
#include <cstddef>

namespace rn {

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
  Str s{"test"};
  int i{5};

  // Must be last.
  SERIALIZABLE( s, i );
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
** Testing
*****************************************************************/
void test_serial();

} // namespace rn
