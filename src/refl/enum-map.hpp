/****************************************************************
**enum-map.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-10.
*
* Description: Map data structure for exhaustive enum keys.
*
*****************************************************************/
#pragma once

// refl
#include "cdr.hpp"
#include "ext.hpp"
#include "query-enum.hpp"

// cdr
#include "cdr/converter.hpp"
#include "cdr/ext.hpp"

// luapp
#include "luapp/ext-userdata.hpp"
#include "luapp/ext-usertype.hpp"
#include "luapp/state.hpp"

// traverse
#include "traverse/ext.hpp"
#include "traverse/type-ext.hpp"

// base
#include "base/adl-tag.hpp"
#include "base/cc-specific.hpp"
#include "base/maybe.hpp"

// C++ standard library
#include <memory>
#include <set>
#include <vector>

namespace refl {

// This is a map whose keys are always reflected enums and which
// is always guaranteed to have a value for every possible value
// of the enum, though some of those values might just be default
// constructed if they are not provided. That way, the map always
// has the same size and you can use operator[] on a const map.
//
// Note: type trait specializations of this (e.g. to_str) should
// just defer to to the base class implementation, which will
// usually always be provided because it is a standard container.
//
// Iteration order is guaranteed to be in order of the reflected
// enum elements because the backing container is ordered.
template<refl::ReflectedEnum Enum, typename ValT>
struct enum_map : public std::vector<std::pair<Enum, ValT>> {
  static_assert( std::is_default_constructible_v<ValT> );
  using Base = std::vector<std::pair<Enum, ValT>>;

  static constexpr int kSize = refl::enum_count<Enum>;

  Base& as_base() { return *this; }
  Base const& as_base() const { return *this; }

  friend void to_str( enum_map const& o, std::string& out,
                      base::template tag<enum_map> ) {
    to_str( o.as_base(), out, base::template tag<Base>{} );
  }

 public:
  using key_type   = typename Base::value_type::first_type;
  using value_type = typename Base::value_type::second_type;

  // All other constructors should ultimately call this one,
  // since this is the one that ensures that all keys have a
  // value.
  enum_map( Base&& b ) : Base( kSize ) {
    // At this point the backing container has the correct size
    // and all of its values are default constructed, but we
    // still need to initialize the keys.
    for( Enum e : refl::enum_values<Enum> ) {
      auto& p = this->as_base()[static_cast<size_t>( e )];
      p.first = e;
      if constexpr( std::equality_comparable<ValT> ) {
        DCHECK( p.second == ValT{} );
      }
    }
    // Now insert any values that were provided.
    for( auto& [e, val] : b ) ( *this )[e] = std::move( val );
  }

  enum_map( Base const& b ) : enum_map( Base{ b } ) {}

  enum_map() : enum_map( Base{} ) {}

  enum_map(
      std::initializer_list<std::pair<Enum const, ValT>> il )
    : enum_map( Base( il.begin(), il.end() ) ) {}

  consteval size_t size() const { return kSize; }
  consteval int ssize() const { return kSize; }

  bool operator==( enum_map const& ) const = default;

  ValT const& operator[]( Enum i ) const { return at( i ); }

  ValT& operator[]( Enum i ) { return at( i ); }

  ValT const& at( Enum i ) const {
    size_t const idx = static_cast<size_t>( i );
    DCHECK( idx >= 0 );
    DCHECK( idx < kSize );
    return this->as_base()[idx].second;
  }

  ValT& at( Enum i ) {
    size_t const idx = static_cast<size_t>( i );
    DCHECK( idx >= 0 );
    DCHECK( idx < kSize );
    return this->as_base()[idx].second;
  }

  // Return the number of entries whose values are not equal to
  // the default-constructed value. This is the closest thing
  // that we get to a concept of a "size" with this container.
  int count_non_default_values() const {
    int count               = 0;
    static ValT const empty = {};
    for( auto const& [k, v] : *this )
      if( v != empty ) //
        ++count;
    return count;
  }

  // Make sure that the following base class methods are not
  // callable since calling them is not correct for this class;
  // enum_maps always contain every enum key, and are of a fixed
  // size, and that has the consequence that there is a very lim-
  // ited set of operations that make sense to perform on it.
  //
  // We can't just hide the base class and only expose the
  // methods we want (whitelist) because we want this type to im-
  // plicitly convert to the base.
  void contains( Enum )              = delete;
  void erase( Enum )                 = delete;
  void reserve( size_t )             = delete;
  void shrink_to_fit()               = delete;
  void clear()                       = delete;
  void insert( size_t, ValT const& ) = delete;
  void push_back( ValT const& )      = delete;
  void emplace_back( ValT const& )   = delete;
  void pop_back()                    = delete;
  void resize( size_t )              = delete;
  void find( ValT )                  = delete;
  void empty()                       = delete;

  friend cdr::value to_canonical( cdr::converter& conv,
                                  enum_map const& o,
                                  cdr::tag_t<enum_map> ) {
    cdr::table tbl;
    // Here we can use to_field to allow the converter to control
    // default field value behavior because, for this data struc-
    // ture, we know the complete set of possible keys and all of
    // their names, similar to a struct.
    for( Enum e : refl::enum_values<Enum> )
      conv.to_field(
          tbl, std::string( refl::enum_value_name( e ) ), o[e] );
    return tbl;
  }

  friend cdr::result<enum_map> from_canonical(
      cdr::converter& conv, cdr::value const& v,
      cdr::tag_t<enum_map> ) {
    UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
    std::set<std::string> used_keys;
    enum_map res;
    // Here we can use from_field to allow the converter to con-
    // trol default field value behavior because, for this data
    // structure, we know the complete set of possible keys and
    // all of their names, similar to a struct.
    for( Enum e : refl::enum_values<Enum> ) {
      UNWRAP_RETURN(
          val,
          conv.from_field<ValT>(
              tbl, std::string( refl::enum_value_name( e ) ),
              used_keys ) );
      res[e] = std::move( val );
    }
    HAS_VALUE_OR_RET(
        conv.end_field_tracking( tbl, used_keys ) );
    return res;
  }

  // Implement this (as opposed to allowing the std::range ver-
  // sion to handle it) so that we can give the enum value as the
  // key. Otherwise it would end up going to the std::pair ele-
  // ment overload which would give the key as a string field
  // named "first".
  friend void traverse( enum_map const& o, auto& fn,
                        trv::tag_t<enum_map> ) {
    for( auto const& [k, v] : o ) fn( v, k );
  }

  friend void define_usertype_for( lua::state& st,
                                   lua::tag<enum_map> ) {
    using K = Enum;
    using V = ValT;
    using U = enum_map;

    auto u = st.usertype.create<U>();

    lua::table mt = u[lua::metatable_key];
    auto const __index =
        mt["__index"].template as<lua::rfunction>();
    lua::cthread const L = st.resource();

    mt["__index"] =
        [L, __index](
            U& o, lua::any const key ) -> base::maybe<lua::any> {
      auto const st = lua::state::view( L );
      if( auto const member = __index( o, key );
          member != lua::nil )
        return member.template as<lua::any>();
      auto const maybe_key = safe_as<K>( key );
      if( !maybe_key.has_value() ) return base::nothing;
      auto& val    = o[*maybe_key];
      auto const L = __index.this_cthread();
      return lua::as<lua::any>( L, val );
    };

    mt["__newindex"] = []( U& o, K const& key, V const& val ) {
      o[key] = val;
    };
  }
};

} // namespace refl

/****************************************************************
** TypeTraverse Specializations.
*****************************************************************/
namespace trv {
TRV_TYPE_TRAVERSE( ::refl::enum_map, K, V );
}

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS_KV( refl::enum_map, owned_by_cpp ){};
} // namespace lua
