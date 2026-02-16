/****************************************************************
**map-matrix.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-02-16.
*
* Description: Type that makes it easier to fwd decl. the matrix
*              that holds the map.
*
*****************************************************************/
#pragma once

// ss
#include "map-square.rds.hpp"

// gfx
#include "gfx/matrix.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

// cdr
#include "cdr/ext.hpp"

// traverse
#include "traverse/ext.hpp"
#include "traverse/type-ext.hpp"

// base
#include "base/adl-tag.hpp"
#include "base/to-str.hpp"

namespace lua {
struct state;
}

namespace rn {

/****************************************************************
** MapMatrix.
*****************************************************************/
// This derived class doesn't serve much of a purpose other than
// just allowing us to pass it around via forward-declared refer-
// ence, which we wouldn't be able to do if it were Matrix<Map-
// Square>.
struct MapMatrix : public gfx::Matrix<MapSquare> {
  using Base = gfx::Matrix<MapSquare>;

  using Base::Base;

  MapMatrix( Base&& b ) : Base( std::move( b ) ) {}

  Base const& as_base() const& { return *this; }
  Base& as_base() & { return *this; }
  Base&& as_base() && { return std::move( *this ); }
  Base const&& as_base() const&& { return std::move( *this ); }

  // Implement base::Show.
  friend void to_str( MapMatrix const& o, std::string& out,
                      base::tag<MapMatrix> ) {
    to_str( o.as_base(), out, base::tag<Base>{} );
  }

  // Implement trv::Traversable.
  friend void traverse( MapMatrix const& o, auto& fn,
                        trv::tag_t<MapMatrix> ) {
    return traverse( o.as_base(), fn,
                     trv::tag_t<Matrix<MapSquare>>{} );
  }

  // Implement cdr::ToCanonical.
  friend cdr::value to_canonical( cdr::converter& conv,
                                  MapMatrix const& m,
                                  cdr::tag_t<MapMatrix> );

  // Implement cdr::FromCanonical.
  friend cdr::result<MapMatrix> from_canonical(
      cdr::converter& conv, cdr::value const& v,
      cdr::tag_t<MapMatrix> );

  // Lua usertype.
  friend void define_usertype_for( lua::state& st,
                                   lua::tag<MapMatrix> ) {
    MapMatrix::Base::define_usertype_for_template<MapMatrix>(
        st );
  }
};

} // namespace rn

/****************************************************************
** TypeTraverse Specializations.
*****************************************************************/
namespace trv {
template<template<typename> typename O>
struct TypeTraverse<O, ::rn::MapMatrix> {
  using type = trv::list<                     //
      typename O<::rn::MapMatrix::Base>::type //
      >;
};
}

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS( ::rn::MapMatrix, owned_by_cpp ){};
}
