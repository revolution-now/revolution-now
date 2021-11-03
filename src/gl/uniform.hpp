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

#define DECL_UNIFORM_NAME_TAG( prog_type, shader_name )    \
  struct shader_name##_##Uniform_Tag {                     \
    constexpr static std::string_view name = #shader_name; \
  };                                                       \
  inline constexpr auto u_##shader_name =                  \
      prog_type::make_checked_uniform<                     \
          shader_name##_##Uniform_Tag>();

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
  // Use the safe-num types as parameters so that there are no
  // ambiguities/uncertainties due to implicit conversions.
  void set( base::safe::floating<float> val ) const;
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
template<typename T>
struct Uniform : UniformNonTyped {
  using type = T;

  Uniform( ObjId pgrm_id, std::string_view name )
    : UniformNonTyped( pgrm_id, name ) {}

  void set( T const& val ) {
    if( cache_ == val ) return;
    cache_ = val;
    this->UniformNonTyped::set( val );
  }

private:
  base::maybe<T> cache_;
};

} // namespace gl
