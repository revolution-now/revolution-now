/****************************************************************
**uniform.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-02.
*
* Description: OpenGL uniform representation.
*
*****************************************************************/
#pragma once

// gl
#include "attribs.hpp"
#include "types.hpp"

// base
#include "base/maybe.hpp"
#include "base/safe-num.hpp"
#include "base/valid.hpp"

// C++ standard library
#include <string_view>

namespace gl {

/****************************************************************
** UniformSpec
*****************************************************************/
template<typename T>
struct UniformSpec {
  constexpr UniformSpec( std::string_view name_ )
    : name( name_ ) {}
  using type = T;
  std::string_view name;
};

/****************************************************************
** UniformNonTyped
*****************************************************************/
struct UniformNonTyped {
  UniformNonTyped( ObjId pgrm_id, std::string_view name );

 protected:
  using set_valid_t = base::valid_or<std::string>;
  // Use the safe-num types as parameters so that there are no
  // ambiguities/uncertainties due to implicit conversions.
  set_valid_t set( base::safe::floating<float> val ) const;
  set_valid_t set( base::safe::integer<long> val ) const;
  set_valid_t set( base::safe::boolean val ) const;
  set_valid_t set( vec2 val ) const;

 private:
  ObjId pgrm_id_;
  int   location_;
};

/****************************************************************
** Uniform
*****************************************************************/
template<typename T>
struct Uniform : UniformNonTyped {
  using type = T;

  explicit Uniform( ObjId pgrm_id, std::string_view name )
    : UniformNonTyped( pgrm_id, name ) {}

  void set( T const& val ) { CHECK_HAS_VALUE( try_set( val ) ); }

  Uniform& operator=( T const& val ) {
    set( val );
    return *this;
  }

  // Normally you should just use `set` above. This version is
  // used when constructing the uniforms to check that they can
  // be set with a given type (which effectively verifies that
  // they have the same type in the shader code as in the C++
  // code).
  base::valid_or<std::string> try_set( T const& val ) {
    if( cache_ == val ) return base::valid;
    auto res = this->UniformNonTyped::set( val );
    if( res.valid() )
      cache_ = val;
    else
      cache_.reset(); // just in case.
    return res;
  }

 private:
  base::maybe<T> cache_;
};

} // namespace gl
