/****************************************************************
**bindable.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-26.
*
* Description: RAII type for handling binding/unbinding of
*              objects.
*
*****************************************************************/
#pragma once

// gl
#include "types.hpp"

// base
#include "base/error.hpp"
#include "base/macros.hpp"

namespace gl {

/****************************************************************
** bindable
*****************************************************************/
template<typename Derived>
struct bindable {
private:
  struct [[nodiscard]] binder {
    binder( Derived const& derived )
      : derived_( derived ), prev_( Derived::current_bound() ) {
      Derived::bind_obj_id( derived_.bindable_obj_id() );
    }

    ~binder() {
      CHECK_EQ( Derived::current_bound(),
                derived_.bindable_obj_id() );
      Derived::bind_obj_id( prev_ );
      CHECK_EQ( Derived::current_bound(), prev_ );
    }

    NO_COPY_NO_MOVE( binder );

  private:
    Derived const& derived_;
    ObjId          prev_;
  };

public:
  binder bind() const { return binder( derived() ); }

  ObjId id() const { return derived().bindable_obj_id(); }

protected:
  bindable() = default;

private:
  Derived const& derived() const {
    return static_cast<Derived const&>( *this );
  }
};

} // namespace gl
