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

// C++ standard library
#include <string_view>

namespace gl {

/****************************************************************
** UniformSpec
*****************************************************************/
template<typename GlType>
struct UniformSpec {
  constexpr UniformSpec( std::string_view name_ )
    : name( name_ ) {}
  std::string_view name;
};

/****************************************************************
** UniformNonTyped
*****************************************************************/
struct UniformNonTyped {
  UniformNonTyped( ObjId pgrm_id, std::string_view name );

  // Use the safe-num types as parameters so that there are no
  // ambiguities/uncertainties due to implicit conversions.
  //
  // NOTE: must call glUseProgram before calling these.
  void set( base::safe::floating<float> val ) const;
  void set( base::safe::integer<int> val ) const;
  void set( base::safe::integer<long> val ) const;
  void set( base::safe::boolean val ) const;
  void set( vec2 val ) const;

private:
  ObjId pgrm_id_;
  int   location_;
};

/****************************************************************
** Uniform
*****************************************************************/
template<typename GlType>
struct Uniform : UniformNonTyped {
  constexpr Uniform( std::string_view name ) : name_( name ) {}

private:
  std::string_view    name_;
  base::maybe<GlType> cache_;
};

} // namespace gl
