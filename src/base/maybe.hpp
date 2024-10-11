/****************************************************************
**maybe.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-28.
*
* Description: An alternative to std::optional, used in the RN
*              code base.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "attributes.hpp"
#include "auto-field.hpp"
#include "error.hpp"
#include "fmt.hpp"
#include "meta.hpp"
#include "to-str.hpp"

// base-util
#include "base-util/mp.hpp"

// C++ standard library
#include <functional>
#include <source_location>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace base {

/****************************************************************
** nothing_t
*****************************************************************/
struct nothing_t {
  // Constructor must be explicit so that we can do `m = {}` to
  // disengage a `maybe` object without ambiguity.
  constexpr explicit nothing_t( int ) {}
};

inline constexpr nothing_t nothing( 0 );

/****************************************************************
** Requirements on maybe::value_type
*****************************************************************/
// Normally it wouldn't make sense to turn these into a concept,
// since they're just a random list of requiremnets, but it is
// convenient because they need to be kept in sync between the
// class declaration, the deduction guide, and the friend decla-
// ration.
template<typename T>
concept MaybeTypeRequirements = requires {
  requires(
      !std::is_same_v<std::remove_cvref_t<T>, std::in_place_t> &&
      !std::is_same_v<std::remove_cvref_t<T>, nothing_t> );
};

/****************************************************************
** Forward Declaration
*****************************************************************/
template<typename T>
requires MaybeTypeRequirements<T>
class [[nodiscard]] maybe;

/****************************************************************
** Deduction Guide
*****************************************************************/
// From cppreference: One deduction guide is provided to account
// for the edge cases missed by the implicit deduction guides, in
// particular, non-copyable arguments and array to pointer con-
// version.
template<class T>
requires MaybeTypeRequirements<T> /* clang-format off */
maybe( T ) -> maybe<T>; /* clang-format on */

/****************************************************************
** bad_maybe_access exception
*****************************************************************/
struct bad_maybe_access : public std::exception {
  // Don't give a default value to loc because we want that to be
  // supplied by someone further up the call chain in order to
  // produce a more helpful location to the user.
  bad_maybe_access( std::source_location loc )
    : std::exception{}, loc_{ std::move( loc ) }, error_msg_{} {
    error_msg_ = loc_.file_name();
    error_msg_ += ":";
    error_msg_ += std::to_string( loc_.line() );
    error_msg_ += ": ";
    error_msg_ += "value() called on an inactive maybe.";
  }

  // This is to suppress clang's -Wweak-vtables, which warns that
  // without any out-of-line-functions the vtable would have to
  // be emitted in every translation unit, which we don't want.
  virtual void dummy_key_function() const final;

  char const* what() const noexcept override {
    return error_msg_.c_str();
  }

  std::source_location loc_;
  std::string          error_msg_;
};

/****************************************************************
** Basic metaprogramming helpers.
*****************************************************************/
template<typename T>
constexpr bool is_maybe_v = false;
template<typename T>
constexpr bool is_maybe_v<maybe<T>> = true;

template<typename T>
constexpr bool is_nothing_v = false;
template<>
inline constexpr bool is_nothing_v<nothing_t> = true;

/****************************************************************
** maybe
*****************************************************************/
template<typename T>
requires MaybeTypeRequirements<T> /* clang-format off */
class [[nodiscard]] maybe { /* clang-format on */
 public:
  /**************************************************************
  ** Types
  ***************************************************************/
  using value_type = T;

  /**************************************************************
  ** Default Constructor
  ***************************************************************/
  constexpr maybe()
  requires( std::is_trivially_default_constructible_v<T> )
  = default;

  // This does not initialize the union member.
  constexpr maybe() noexcept
  requires( !std::is_trivially_default_constructible_v<T> )
    : active_{ false } {}

  constexpr maybe( nothing_t ) noexcept : maybe() {}

  /**************************************************************
  ** Destruction
  ***************************************************************/

 private:
  constexpr void destroy_if_active() {
    if constexpr( !std::is_trivially_destructible_v<T> ) {
      if( active_ ) ( **this ).T::~T();
      // Don't set active_ to false here. That is the purpose of
      // this function. If you want to destroy+deactivate then
      // call the public reset() function.
    }
  }

 public:
  constexpr ~maybe() noexcept
  requires( std::is_trivially_destructible_v<T> )
  = default;
  constexpr ~maybe() noexcept
  requires( !std::is_trivially_destructible_v<T> )
  {
    destroy_if_active();
  }

  /**************************************************************
  ** Value Constructors
  ***************************************************************/
  // For each one there are two copies, one for trivially default
  // constructible and one not. This is to support constexpr. It
  // seems that, in order for a non primitive type to be initial-
  // ized with a value in a constexpr constext (i.e., by as-
  // signing to it and not using placement new) it must already
  // have been initialized, otherwise you cannot call its assign-
  // ment operator. So these variations will ensure that the
  // trivially default constructible ones will get initialized
  // before calling new_val on them.  Not sure if this is the
  // right way to go about this...
  constexpr maybe( T const& val ) /* clang-format off */
      noexcept( std::is_nothrow_copy_constructible_v<T> )
      requires( std::is_copy_constructible_v<T> &&
               !std::is_trivially_default_constructible_v<T>)
    : active_{ false } /* clang-format on */ {
    new_val( val );
    // Now set to true after no exception has happened.
    active_ = true;
  }

  constexpr maybe( T const& val ) /* clang-format off */
      noexcept( std::is_nothrow_copy_constructible_v<T> )
      requires( std::is_copy_constructible_v<T> &&
                std::is_trivially_default_constructible_v<T>)
    : val_{}, active_{ false } {
    new_val( val );
    // Now set to true after no exception has happened.
    active_ = true;
  }

  constexpr maybe( T&& val ) /* clang-format off */
      noexcept( std::is_nothrow_move_constructible_v<T> )
      requires( std::is_move_constructible_v<T> &&
               !std::is_trivially_default_constructible_v<T> )
    : active_{ false } /* clang-format on */ {
    new_val( std::move( val ) );
    // Now set to true after no exception has happened.
    active_ = true;
  }

  constexpr maybe( T&& val ) /* clang-format off */
      noexcept( std::is_nothrow_move_constructible_v<T> )
      requires( std::is_move_constructible_v<T> &&
                std::is_trivially_default_constructible_v<T>)
    : val_{}, active_{ false } {
    new_val( std::move( val ) );
    // Now set to true after no exception has happened.
    active_ = true;
  }

  /**************************************************************
  ** Converting Value Constructor
  ***************************************************************/
  template<typename U = T> /* clang-format off */
  explicit( !std::is_convertible_v<U&&, T> )
  constexpr maybe( U&& val )
      noexcept( std::is_nothrow_convertible_v<U, T> )
      requires(
         std::is_constructible_v<T, U&&> &&
        !std::is_same_v<std::remove_cvref_t<U>, std::in_place_t> &&
        !std::is_same_v<std::remove_cvref_t<U>, maybe<T>> &&
        !std::is_trivially_default_constructible_v<T> )
    : active_{ false } { /* clang-format on */
    new_val( std::forward<U>( val ) );
    // Now set to true after no exception has happened.
    active_ = true;
  }

  template<typename U = T> /* clang-format off */
  explicit( !std::is_convertible_v<U&&, T> )
  constexpr maybe( U&& val )
      noexcept( std::is_nothrow_convertible_v<U, T> )
      requires(
         std::is_constructible_v<T, U&&> &&
        !std::is_same_v<std::remove_cvref_t<U>, std::in_place_t> &&
        !std::is_same_v<std::remove_cvref_t<U>, maybe<T>> &&
         std::is_trivially_default_constructible_v<T> )
    : val_{}, active_{ false } {
    new_val( std::forward<U>( val ) );
    // Now set to true after no exception has happened.
    active_ = true;
  }

  /**************************************************************
  ** Copy Constructors
  ***************************************************************/
  constexpr maybe( maybe<T> const& other ) /* clang-format off */
      noexcept( noexcept( this->new_val( *other ) ) )
      requires( std::is_copy_constructible_v<T> &&
               !std::is_trivially_copy_constructible_v<T> )
    : active_{ false } /* clang-format on */ {
    if( other.has_value() ) new_val( *other );
    // Now set after no exception has happened.
    active_ = other.has_value();
  }

  constexpr maybe( maybe<T> const& other ) /* clang-format off */
      noexcept( std::is_nothrow_copy_constructible_v<T> )
      requires( std::is_trivially_copy_constructible_v<T> )
    = default; /* clang-format on */

  /**************************************************************
  ** Converting Copy Constructors
  ***************************************************************/
  template<typename U> /* clang-format off */
  explicit( !std::is_convertible_v<U const&, T> )
  constexpr maybe( maybe<U> const& other )
      noexcept( noexcept( this->new_val( *other ) ) )
      requires( std::is_constructible_v<T, const U&>         &&
               !std::is_constructible_v<T, maybe<U>&>        &&
               !std::is_constructible_v<T, maybe<U> const&>  &&
               !std::is_constructible_v<T, maybe<U>&&>       &&
               !std::is_constructible_v<T, maybe<U> const&&> &&
               !std::is_convertible_v<maybe<U>&, T>          &&
               !std::is_convertible_v<maybe<U> const&, T>    &&
               !std::is_convertible_v<maybe<U>&&, T>         &&
               !std::is_convertible_v<maybe<U> const&&, T> )
    : active_{ false } /* clang-format on */ {
    if( other.has_value() ) new_val( *other );
    // Now set after no exception has happened.
    active_ = other.has_value();
  }

  /**************************************************************
  ** Move Constructors
  ***************************************************************/
  constexpr maybe( maybe<T>&& other ) /* clang-format off */
    noexcept( noexcept( this->new_val( std::move( *other ) ) ) )
    requires( std::is_move_constructible_v<T> &&
             !std::is_trivially_move_constructible_v<T> )
    : active_{ false } /* clang-format on */ {
    if( other.has_value() ) new_val( std::move( *other ) );
    // Now set after no exception has happened.
    active_ = other.has_value();
    // cppreference says that the noexcept spec for this function
    // should be is_nothrow_move_constructible_v<T>, so as a
    // sanity check, compare it with what we have.
    static_assert(
        std::is_nothrow_move_constructible_v<T> ==
        noexcept( this->new_val( std::move( *other ) ) ) );
  }

  constexpr maybe( maybe<T>&& other ) /* clang-format off */
      requires( std::is_trivially_move_constructible_v<T> )
    = default; /* clang-format on */

  /**************************************************************
  ** Converting Move Constructors
  ***************************************************************/
  template<typename U> /* clang-format off */
  explicit( !std::is_convertible_v<U&&, T> )
  constexpr maybe( maybe<U>&& other )
      noexcept( noexcept(
            this->new_val( std::move( *other ) ) ) )
      requires( std::is_constructible_v<T, U&&>              &&
               !std::is_constructible_v<T, maybe<U>&>        &&
               !std::is_constructible_v<T, maybe<U> const&>  &&
               !std::is_constructible_v<T, maybe<U>&&>       &&
               !std::is_constructible_v<T, maybe<U> const&&> &&
               !std::is_convertible_v<maybe<U>&, T>          &&
               !std::is_convertible_v<maybe<U> const&, T>    &&
               !std::is_convertible_v<maybe<U>&&, T>         &&
               !std::is_convertible_v<maybe<U> const&&, T> )
    : active_{ false } /* clang-format on */ {
    if( other.has_value() ) new_val( std::move( *other ) );
    // Now set after no exception has happened.
    active_ = other.has_value();
  }

  /**************************************************************
  ** In-place construction
  ***************************************************************/
  template<typename... Args> /* clang-format off */
  constexpr explicit maybe( std::in_place_t, Args&&... args )
      noexcept( noexcept(
            this->new_val( std::forward<Args>( args )... )) )
      requires( std::is_constructible_v<T, Args...> )
    : active_{ false } /* clang-format on */ {
    new_val( std::forward<Args>( args )... );
    // Now set after no exception has happened.
    active_ = true;
  }

  template<typename U, typename... Args> /* clang-format off */
  constexpr explicit maybe( std::in_place_t,
                            std::initializer_list<U> ilist,
                            Args&&... args )
      noexcept( noexcept(
        this->new_val( ilist, std::forward<Args>( args )... )) )
      requires( std::is_constructible_v<
                    T,
                    std::initializer_list<U>&,
                    Args&&...
                > )
    : active_{ false } /* clang-format on */ {
    new_val( ilist, std::forward<Args>( args )... );
    // Now set after no exception has happened.
    active_ = true;
  }

  /**************************************************************
  ** Implicit Conversions
  ***************************************************************/
  // If T is a reference_wrapper then allow implicit conversions
  // to maybe<T::value_type&>.
  template<typename U>
  constexpr operator maybe<U&>() const noexcept
      ATTR_LIFETIMEBOUND
  requires( mp::is_reference_wrapper_v<T> )
  {
    if( !has_value() ) return nothing;
    return ( **this ).get();
  }

  // Always allow implici conversions to maybe<T&>.
  template<typename U>
  constexpr operator maybe<U const&>() const noexcept
      ATTR_LIFETIMEBOUND
  requires( !mp::is_reference_wrapper_v<T> &&
            std::is_convertible_v<T const&, U const&> )
  {
    if( !has_value() ) return nothing;
    return **this;
  }

  template<typename U>
  constexpr operator maybe<U&>() noexcept ATTR_LIFETIMEBOUND
  requires( !mp::is_reference_wrapper_v<T> &&
            std::is_convertible_v<T&, U&> )
  {
    if( !has_value() ) return nothing;
    return **this;
  }

  /**************************************************************
  ** Copy Assignment
  ***************************************************************/
  /* clang-format off */
  constexpr maybe<T>& operator=( maybe<T> const& rhs ) &
    noexcept( std::is_nothrow_copy_constructible_v<T> &&
              std::is_nothrow_copy_assignable_v<T> )
    requires( std::is_copy_constructible_v<T> &&
              std::is_copy_assignable_v<T> &&
             !(
                std::is_trivially_copy_constructible_v<T> &&
                std::is_trivially_copy_assignable_v<T> &&
                std::is_trivially_destructible_v<T>
              )) {
    /* clang-format on */
    if( !rhs.has_value() ) {
      if( has_value() ) reset();
      return *this;
    }
    // rhs has a value.
    if( !has_value() ) {
      new_val( *rhs );
      // Now set after no exception has happened.
      active_ = true;
      return *this;
    }
    // both have values.
    ( **this ) = *rhs;
    return *this;
  }

  /* clang-format off */
  constexpr maybe<T>& operator=( maybe<T> const& rhs ) &
    requires( std::is_trivially_copy_constructible_v<T> &&
              std::is_trivially_copy_assignable_v<T> &&
              std::is_trivially_destructible_v<T> )
    = default; /* clang-format on */

  /**************************************************************
  ** Move Assignment
  ***************************************************************/
  /* clang-format off */
  constexpr maybe<T>& operator=( maybe<T>&& rhs ) &
    noexcept( std::is_nothrow_move_constructible_v<T> &&
              std::is_nothrow_move_assignable_v<T> )
    requires( std::is_move_constructible_v<T> &&
              std::is_move_assignable_v<T> &&
             !(
                std::is_trivially_move_constructible_v<T> &&
                std::is_trivially_move_assignable_v<T> &&
                std::is_trivially_destructible_v<T>
              )) {
    /* clang-format on */
    // Sanity check that the `noexcept` value that the compiler
    // derives from the functions we're calling (the ones that
    // coudl throw) is the same as the noexcept spec that we've
    // supplied above, and which is dictated by cppreference.
    static_assert(
        (std::is_nothrow_move_constructible_v<T> &&
         std::is_nothrow_move_assignable_v<T>) ==
        ( noexcept( new_val( std::move( *rhs ) ) ) &&
          noexcept( ( **this ) = std::move( *rhs ) ) ) );
    if( !rhs.has_value() ) {
      if( has_value() ) reset();
      return *this;
    }
    // rhs has a value.
    if( !has_value() ) {
      new_val( std::move( *rhs ) );
      // Now set after no exception has happened.
      active_ = true;
      return *this;
    }
    // both have values.
    ( **this ) = std::move( *rhs );
    return *this;
  }

  /* clang-format off */
  constexpr maybe<T>& operator=( maybe<T>&& rhs ) &
    noexcept( std::is_nothrow_move_assignable_v<T> )
    requires( std::is_trivially_move_constructible_v<T> &&
              std::is_trivially_move_assignable_v<T> &&
              std::is_trivially_destructible_v<T> )
    = default; /* clang-format on */

  /**************************************************************
  ** Converting Assignment
  ***************************************************************/
  template<typename U = T> /* clang-format off */
  constexpr maybe<T>& operator=( maybe<U> const& rhs ) &
      noexcept( std::is_nothrow_constructible_v<T, U const&> &&
                std::is_nothrow_assignable_v<T&, U const&> )
      requires( std::is_constructible_v<T, U const&>         &&
                std::is_assignable_v<T&, U const&>           &&
               !std::is_constructible_v<T, maybe<U>&>        &&
               !std::is_constructible_v<T, maybe<U> const&>  &&
               !std::is_constructible_v<T, maybe<U>&&>       &&
               !std::is_constructible_v<T, maybe<U> const&&> &&
               !std::is_convertible_v<maybe<U>&, T>          &&
               !std::is_convertible_v<maybe<U> const&, T>    &&
               !std::is_convertible_v<maybe<U>&&, T>         &&
               !std::is_convertible_v<maybe<U> const&&, T>   &&
               !std::is_assignable_v<T&, maybe<U>&>          &&
               !std::is_assignable_v<T&, maybe<U> const&>    &&
               !std::is_assignable_v<T&, maybe<U>&&>         &&
               !std::is_assignable_v<T&, maybe<U> const&&> ) {
    /* clang-format on */
    if( !rhs.has_value() ) {
      if( has_value() ) reset();
      return *this;
    }
    // rhs has a value.
    if( !has_value() ) {
      new_val( *rhs );
      // Now set after no exception has happened.
      active_ = true;
      return *this;
    }
    // both have values.
    ( **this ) = *rhs;
    return *this;
  }

  template<typename U = T> /* clang-format off */
  constexpr maybe<T>& operator=( maybe<U>&& rhs ) &
      noexcept( std::is_nothrow_constructible_v<T, U> &&
                std::is_nothrow_assignable_v<T&, U> )
      requires( std::is_constructible_v<T, U>                &&
                std::is_assignable_v<T&, U>                  &&
               !std::is_constructible_v<T, maybe<U>&>        &&
               !std::is_constructible_v<T, maybe<U> const&>  &&
               !std::is_constructible_v<T, maybe<U>&&>       &&
               !std::is_constructible_v<T, maybe<U> const&&> &&
               !std::is_convertible_v<maybe<U>&, T>          &&
               !std::is_convertible_v<maybe<U> const&, T>    &&
               !std::is_convertible_v<maybe<U>&&, T>         &&
               !std::is_convertible_v<maybe<U> const&&, T>   &&
               !std::is_assignable_v<T&, maybe<U>&>          &&
               !std::is_assignable_v<T&, maybe<U> const&>    &&
               !std::is_assignable_v<T&, maybe<U>&&>         &&
               !std::is_assignable_v<T&, maybe<U> const&&> ) {
    /* clang-format on */
    if( !rhs.has_value() ) {
      if( has_value() ) reset();
      return *this;
    }
    // rhs has a value.
    if( !has_value() ) {
      new_val( std::move( *rhs ) );
      // Now set after no exception has happened.
      active_ = true;
      return *this;
    }
    // both have values.
    ( **this ) = std::move( *rhs );
    return *this;
  }

  /**************************************************************
  ** Converting Value Assignment
  ***************************************************************/
  template<typename U = T> /* clang-format off */
  constexpr maybe<T>& operator=( U&& rhs ) &
      noexcept( std::is_nothrow_constructible_v<T, U> &&
                std::is_nothrow_assignable_v<T&, U> )
      requires( std::is_constructible_v<T, U> &&
                std::is_assignable_v<T&, U> &&
                (!std::is_scalar_v<T> ||
                 !std::is_same_v<std::decay_t<U>, T>) )
  /* clang-format on */ {
    if( !has_value() ) {
      new_val( std::forward<U>( rhs ) );
      // Now set after no exception has happened.
      active_ = true;
      return *this;
    }
    // both have values.
    ( **this ) = std::forward<U>( rhs );
    return *this;
  }

  /**************************************************************
  ** nothing assignment
  ***************************************************************/
  maybe<T>& operator=( nothing_t ) & noexcept {
    reset();
    return *this;
  }

  /**************************************************************
  ** Swap
  ***************************************************************/
  constexpr void swap( maybe<T>& other ) /* clang-format off */
    noexcept( std::is_nothrow_move_constructible_v<T> &&
              std::is_nothrow_swappable_v<T> )
    requires( std::is_move_constructible_v<T> &&
              std::is_swappable_v<T> ) { /* clang-format on */
    if( !active_ && !other.has_value() ) return;
    if( active_ && other.has_value() ) {
      using std::swap;
      swap( **this, *other );
      return;
    }
    // One is active and the other is not.
    maybe<T>& active   = has_value() ? *this : other;
    maybe<T>& inactive = has_value() ? other : *this;
    inactive.new_val( std::move( *active ) );
    // Now set after no exception has happened.
    inactive.active_ = true;
    active.reset();
  }

  /**************************************************************
  ** value
  ***************************************************************/
  [[nodiscard]] constexpr T const& value(
      std::source_location loc =
          std::source_location::current() ) const& {
    if( !active_ ) throw bad_maybe_access{ loc };
    return **this;
  }
  [[nodiscard]] constexpr T& value(
      std::source_location loc =
          std::source_location::current() ) & {
    if( !active_ ) throw bad_maybe_access{ loc };
    return **this;
  }

  [[nodiscard]] constexpr T const&& value(
      std::source_location loc =
          std::source_location::current() ) const&& {
    if( !active_ ) throw bad_maybe_access{ loc };
    return std::move( **this );
  }
  [[nodiscard]] constexpr T&& value(
      std::source_location loc =
          std::source_location::current() ) && {
    if( !active_ ) throw bad_maybe_access{ loc };
    return std::move( **this );
  }

  // Just to give a uniform interface with expect<>.
  constexpr nothing_t error(
      std::source_location loc =
          std::source_location::current() ) const {
    if( active_ ) throw bad_maybe_access{ loc };
    return nothing;
  }

  /**************************************************************
  ** value_or
  ***************************************************************/

 private:
  // C++ does not support returning C array types by value from
  // functions, so we need to check for that here otherwise the
  // value_or methods below will cause hard compile errors.
  using return_type_for_arrays =
      std::conditional_t<std::is_array_v<T>, void, T>;

 public:
  template<typename U = T>
  // clang-format off
  [[nodiscard]] return_type_for_arrays constexpr
  value_or( U&& def ) const&
    noexcept( std::is_nothrow_convertible_v<U&&, T> &&
              std::is_nothrow_copy_constructible_v<T>)
    requires( std::is_convertible_v<U&&, T> &&
              std::is_copy_constructible_v<T> ) {
    // clang-format on
    return has_value()
               ? **this
               : static_cast<T>( std::forward<U>( def ) );
  }

  template<typename U = T>
  // clang-format off
  [[nodiscard]] return_type_for_arrays constexpr
  value_or( U&& def ) &&
    noexcept( std::is_nothrow_convertible_v<U&&, T> &&
              std::is_nothrow_move_constructible_v<T>)
    requires( std::is_convertible_v<U&&, T> &&
              std::is_move_constructible_v<T> ) {
    // clang-format on
    return has_value()
               ? std::move( **this )
               : static_cast<T>( std::forward<U>( def ) );
  }

  /**************************************************************
  ** reset
  ***************************************************************/
  void reset() noexcept {
    if( active_ ) {
      destroy_if_active();
      active_ = false;
    }
  }

  /**************************************************************
  ** has_value/bool
  ***************************************************************/
  [[nodiscard]] constexpr bool has_value() const noexcept {
    return active_;
  }

  [[nodiscard]] constexpr explicit operator bool() const noexcept
      // If we have a maybe<bool> don't allow conversion to bool,
      // that's probably not a good thing. Instead one should
      // call either the has_value() member for just checking
      // whether there is a value, or call the is_value_truish()
      // method to return true iff there is a value and it's
      // true.
  requires( !std::is_same_v<std::remove_cvref_t<T>, bool> )
  {
    return active_;
  }

  /**************************************************************
  ** Deference operators
  ***************************************************************/
  constexpr T const& operator*() const& noexcept {
    DCHECK( has_value() );
    return val_;
  }
  constexpr T& operator*() & noexcept {
    DCHECK( has_value() );
    return val_;
  }

  constexpr T const&& operator*() const&& noexcept {
    DCHECK( has_value() );
    return std::move( val_ );
  }
  constexpr T&& operator*() && noexcept {
    DCHECK( has_value() );
    return std::move( val_ );
  }

  constexpr T const* operator->() const noexcept {
    return &**this;
  }
  constexpr T* operator->() noexcept { return &**this; }

  /**************************************************************
  ** emplace
  ***************************************************************/
  template<typename... Args> /* clang-format off */
  T& emplace( Args&&... args )
    noexcept( std::is_nothrow_constructible_v<T, Args...> )
    requires( std::is_constructible_v<T, Args...> ) {
    /* clang-format on */
    reset();
    new_val( std::forward<Args>( args )... );
    // Set active_ to true after the construction in order to
    // have exception safety, which says that if an exception is
    // thrown during construction, the object should not have a
    // value.
    active_ = true;
    return **this;
  }

  template<typename U, typename... Args> /* clang-format off */
  T& emplace( std::initializer_list<U> ilist, Args&&... args )
    noexcept( std::is_nothrow_constructible_v<T,
                  std::initializer_list<U>, Args...> )
    requires( std::is_constructible_v<T,
                  std::initializer_list<U>, Args...> ) {
                                         /* clang-format on */
    reset();
    new_val( ilist, std::forward<Args>( args )... );
    // Set active_ to true after the construction in order to
    // have exception safety, which says that if an exception is
    // thrown during construction, the object should not have a
    // value.
    active_ = true;
    return **this;
  }

  /**************************************************************
  ** Mapping to Bool
  ***************************************************************/
  // Returns true only if the maybe is active and contains a
  // value that, when converted to bool, yields true.
  bool is_value_truish() const /* clang-format off */
      noexcept( std::is_nothrow_convertible_v<T, bool> )
      requires( std::is_convertible_v<T, bool> ) {
                               /* clang-format on */
    return has_value() && static_cast<bool>( **this );
  }

  /**************************************************************
  ** Monadic Interface: get_if
  ***************************************************************/
  // This is when the value type is a std::variant.
  template<typename Alt>
  maybe<Alt&> get_if() noexcept ATTR_LIFETIMEBOUND
  requires( requires {
    std::get_if<Alt>( std::declval<T*>() );
  } )
  {
    if( !has_value() ) return nothing;
    auto* p = std::get_if<Alt>( &( **this ) );
    if( p == nullptr ) return nothing;
    return *p;
  }

  template<typename Alt>
  maybe<Alt const&> get_if() const noexcept ATTR_LIFETIMEBOUND
  requires( requires {
    std::get_if<Alt>( std::declval<T*>() );
  } )
  {
    if( !has_value() ) return nothing;
    auto* p = std::get_if<Alt>( &( **this ) );
    if( p == nullptr ) return nothing;
    return *p;
  }

  /**************************************************************
  ** Monadic Interface: inner_if
  ***************************************************************/
  // This is when the value type is a std::variant.
  template<typename Alt>
  auto inner_if() noexcept ATTR_LIFETIMEBOUND
  requires requires { std::get_if<Alt>( std::declval<T*>() ); }
  {
    return bind( []( auto& val ) {
      return val.template inner_if<Alt>();
    } );
  }

  template<typename Alt>
  auto inner_if() const noexcept ATTR_LIFETIMEBOUND
  requires requires { std::get_if<Alt>( std::declval<T*>() ); }
  {
    return bind( []( auto& val ) {
      return val.template inner_if<Alt>();
    } );
  }

  /**************************************************************
  ** Monadic Interface: member
  ***************************************************************/
  template<typename Func>
  auto member( Func&& func ) const& /* clang-format off */
    -> maybe<std::invoke_result_t<Func, T const&>>
    requires( std::is_invocable_v<Func, T const&> &&
              std::is_member_object_pointer_v<Func> &&
             !is_maybe_v<std::remove_cvref_t<
               std::invoke_result_t<Func,T const&>>> ) {
    /* clang-format on */
    using res_t = maybe<std::invoke_result_t<Func, T const&>>;
    res_t res;
    if( has_value() )
      res.assign(
          std::invoke( std::forward<Func>( func ), **this ) );
    return res;
  }

  // To prevent dangling reference bugs.
  template<typename Func>
  auto member( Func&& func ) && = delete;

  template<typename Func>
  auto member( Func&& func ) & /* clang-format off */
    -> maybe<std::invoke_result_t<Func, T&>>
    requires( std::is_invocable_v<Func, T&> &&
              std::is_member_object_pointer_v<Func> &&
             !is_maybe_v<std::remove_cvref_t<
               std::invoke_result_t<Func,T&>>> ) {
    /* clang-format on */
    using res_t = maybe<std::invoke_result_t<Func, T&>>;
    res_t res;
    if( has_value() )
      res.assign(
          std::invoke( std::forward<Func>( func ), **this ) );
    return res;
  }

  /**************************************************************
  ** Monadic Interface: maybe_member
  ***************************************************************/
  template<typename Func>
  auto maybe_member( Func&& func ) const& /* clang-format off */
    -> maybe<typename std::remove_cvref_t<
                        std::invoke_result_t<Func, T const&>
                      >::value_type const&>
    requires( std::is_member_object_pointer_v<Func> &&
              is_maybe_v<std::remove_cvref_t<
                std::invoke_result_t<Func, T const&>>> ) {
    /* clang-format on */
    using res_t = maybe<
        typename std::remove_reference_t<std::invoke_result_t<
            Func, T const&>>::value_type const&>;
    res_t res;
    if( has_value() ) {
      decltype( auto ) maybe_field =
          std::invoke( std::forward<Func>( func ), **this );
      if( maybe_field.has_value() ) res.assign( *maybe_field );
    }
    return res;
  }

  // To prevent dangling reference bugs.
  template<typename Func>
  auto maybe_member( Func&& func ) && = delete;

  template<typename Func>
  auto maybe_member( Func&& func ) & /* clang-format off */
    -> maybe<typename std::remove_cvref_t<
                        std::invoke_result_t<Func, T&>
                      >::value_type&>
    requires( std::is_member_object_pointer_v<Func> &&
              is_maybe_v<std::remove_cvref_t<
                std::invoke_result_t<Func, T&>>> ) {
    /* clang-format on */
    using res_t = maybe<typename std::remove_reference_t<
        std::invoke_result_t<Func, T&>>::value_type&>;
    res_t res;
    if( has_value() ) {
      decltype( auto ) maybe_field =
          std::invoke( std::forward<Func>( func ), **this );
      if( maybe_field.has_value() ) res.assign( *maybe_field );
    }
    return res;
  }

  /**************************************************************
  ** Monadic Interface: fmap
  ***************************************************************/
  template<typename Func>
  auto fmap( Func&& func ) const& /* clang-format off */
    -> maybe<std::invoke_result_t<Func, T const&>>
    requires( std::is_invocable_v<Func, T const&> &&
             !std::is_member_object_pointer_v<Func> &&
             !is_maybe_v<std::remove_cvref_t<
               std::invoke_result_t<Func,T const&>>> ) {
    /* clang-format on */
    using res_t = maybe<std::invoke_result_t<Func, T const&>>;
    res_t res;
    if( has_value() )
      res.assign(
          std::invoke( std::forward<Func>( func ), **this ) );
    return res;
  }

  template<typename Func>
  auto fmap( Func&& func ) & /* clang-format off */
    -> maybe<std::invoke_result_t<Func, T&>>
    requires( std::is_invocable_v<Func, T&> &&
             !std::is_member_object_pointer_v<Func> &&
             !is_maybe_v<std::remove_cvref_t<
               std::invoke_result_t<Func,T&>>> ) {
    /* clang-format on */
    using res_t = maybe<std::invoke_result_t<Func, T&>>;
    res_t res;
    if( has_value() )
      res.assign(
          std::invoke( std::forward<Func>( func ), **this ) );
    return res;
  }

  template<typename Func> /* clang-format off */
  auto fmap( Func&& func ) &&
       -> maybe<std::invoke_result_t<Func, T>>
       requires( std::is_invocable_v<Func, T> &&
                !std::is_member_object_pointer_v<Func> &&
                !is_maybe_v<std::remove_cvref_t<
                  std::invoke_result_t<Func, T>>> ) {
    /* clang-format on */
    using res_t = maybe<std::invoke_result_t<Func, T>>;
    res_t res;
    if( has_value() )
      res.assign( std::invoke( std::forward<Func>( func ),
                               std::move( **this ) ) );
    return res;
  }

  /**************************************************************
  ** Monadic Interface: bind
  ***************************************************************/
  template<typename Func>
  auto bind( Func&& func ) const& /* clang-format off */
    -> std::remove_cvref_t<std::invoke_result_t<Func, T const&>>
    requires( !std::is_member_object_pointer_v<Func> &&
               is_maybe_v<std::remove_cvref_t<
                 std::invoke_result_t<Func,T const&>>> ) {
    /* clang-format on */
    using res_t = std::remove_cvref_t<
        std::invoke_result_t<Func, T const&>>;
    res_t res;
    if( has_value() )
      res.assign(
          std::invoke( std::forward<Func>( func ), **this ) );
    return res;
  }

  template<typename Func>
  auto bind( Func&& func ) & /* clang-format off */
    -> std::remove_cvref_t<std::invoke_result_t<Func, T&>>
    requires( !std::is_member_object_pointer_v<Func> &&
               is_maybe_v<std::remove_cvref_t<
                 std::invoke_result_t<Func,T&>>> ) {
    /* clang-format on */
    using res_t =
        std::remove_cvref_t<std::invoke_result_t<Func, T&>>;
    res_t res;
    if( has_value() )
      res.assign(
          std::invoke( std::forward<Func>( func ), **this ) );
    return res;
  }

  template<typename Func> /* clang-format off */
  auto bind( Func&& func ) &&
    -> std::remove_cvref_t<std::invoke_result_t<Func, T>>
    requires( !std::is_member_object_pointer_v<Func> &&
               is_maybe_v<std::remove_cvref_t<
                 std::invoke_result_t<Func, T>>> ) {
    /* clang-format on */
    using res_t =
        std::remove_cvref_t<std::invoke_result_t<Func, T>>;
    res_t res;
    if( has_value() )
      res.assign( std::invoke( std::forward<Func>( func ),
                               std::move( **this ) ) );
    return res;
  }

  /**************************************************************
  ** Storage
  ***************************************************************/

 private:
  // This allows maybe<T> to access private members of maybe<U>.
  template<typename U>
  requires MaybeTypeRequirements<U>
  friend class maybe;

  // This is so that we can have a common API with the maybe-ref
  // type for assigning, since we need to delete the maybe-ref
  // type's assignment operators (we can't just make them private
  // otherwise the is_invocable type traits of the maybe-ref type
  // get messed up.
  template<typename... Args>
  void assign( Args&&... args ) {
    (void)( this->operator=( std::forward<Args>( args )... ) );
  }

  template<typename... Vs> /* clang-format off */
  constexpr void new_val( Vs&&... v )
    noexcept(
      (std::is_trivially_constructible_v<
            T, decltype( std::forward<Vs>( v ) )...> &&
       std::is_trivially_move_assignable_v<T>)
      ||
      noexcept( new( &this->val_ ) T( std::forward<Vs>( v )... ) )
    ) { /* clang-format on */
    if constexpr( std::is_trivially_constructible_v<
                      T, decltype( std::forward<Vs>( v ) )...> &&
                  std::is_trivially_move_assignable_v<T> ) {
      val_ = T( std::forward<Vs>( v )... );
    } else {
      new( &val_ ) T( std::forward<Vs>( v )... );
    }
  }

  union {
    T val_;
  };
  bool active_ = false;
};

/****************************************************************
** maybe-of-reference
*****************************************************************/
template<typename T>
requires MaybeTypeRequirements<T> /* clang-format off */
class [[nodiscard]] maybe<T&> { /* clang-format on */
 public:
  /**************************************************************
  ** Types
  ***************************************************************/
  using value_type = T&;

  /**************************************************************
  ** Default Constructor
  ***************************************************************/
  maybe() = default;
  constexpr maybe( nothing_t ) noexcept {}

  /**************************************************************
  ** Destruction
  ***************************************************************/
  ~maybe() = default;

  /**************************************************************
  ** Value Constructors
  ***************************************************************/
  constexpr maybe( T& ref ) noexcept { p_ = &ref; }

  // We don't want to form a non-const rvalue reference to a tem-
  // porary, mirroring the behavior of normal references.
  constexpr maybe( T&& ref ATTR_LIFETIMEBOUND ) noexcept
  requires( std::is_const_v<T> )
  {
    p_ = &ref;
  }

  /**************************************************************
  ** Copy Constructor
  ***************************************************************/
  maybe( maybe<T&> const& ) = default;

  /**************************************************************
  ** Converting Copy Constructors
  ***************************************************************/
  template<typename U> /* clang-format off */
  constexpr maybe( maybe<U&> const& other ) noexcept
      requires( std::is_convertible_v<U&, T&> &&
               !std::is_convertible_v<maybe<U&>&, T&> ) {
                       /* clang-format on */
    p_ = static_cast<T*>( other.p_ );
  }

  /**************************************************************
  ** Assignment Operators.
  ***************************************************************/
  maybe<T&> operator=( maybe<T&> const& ) = delete;
  maybe<T&> operator=( maybe<T&>&& )      = delete;

  /**************************************************************
  ** value
  ***************************************************************/
  [[nodiscard]] constexpr T& value(
      std::source_location loc =
          std::source_location::current() ) const {
    if( !has_value() ) throw bad_maybe_access{ loc };
    return **this;
  }

  // Just to give a uniform interface with expect<>.
  constexpr nothing_t error(
      std::source_location loc =
          std::source_location::current() ) const {
    if( has_value() ) throw bad_maybe_access{ loc };
    return nothing;
  }

  /**************************************************************
  ** value_or
  ***************************************************************/
  template<typename U>
  // clang-format off
  [[nodiscard]] constexpr T& value_or( U& def ) const noexcept
    requires( std::is_convertible_v<U&, T&> ) {
    // clang-format on
    return has_value() ? **this : static_cast<T&>( def );
  }

  template<typename U>
  // clang-format off
  [[nodiscard]] constexpr T value_or( U&& def ) const noexcept
    requires( std::is_convertible_v<U&, T&> &&
             !std::is_lvalue_reference_v<
                 decltype( std::forward<U>(def) )> ) {
    // clang-format on
    return has_value() ? **this : static_cast<T&>( def );
  }

  /**************************************************************
  ** has_value/bool
  ***************************************************************/
  [[nodiscard]] constexpr bool has_value() const noexcept {
    return p_ != nullptr;
  }

  [[nodiscard]] constexpr explicit operator bool() const noexcept
      // If we have a maybe<bool&> don't allow conversion to
      // bool, that's probably not a good thing. Instead one
      // should call either the has_value() member for just
      // checking whether there is a value, or call the
      // is_value_truish() method to return true iff there is a
      // value and it's true.
  requires( !std::is_same_v<std::remove_cvref_t<T>, bool> )
  {
    return p_ != nullptr;
  }

  /**************************************************************
  ** Deference operators
  ***************************************************************/
  constexpr T& operator*() const noexcept { return *p_; }

  constexpr T* operator->() const noexcept { return &**this; }

  /**************************************************************
  ** Conversion to Value
  ***************************************************************/
  maybe<std::remove_const_t<T>> to_value() const {
    return *this;
  }

  /**************************************************************
  ** Mapping to Bool
  ***************************************************************/
  // Returns true only if the maybe is active and refers to a
  // value that, when converted to bool, yields true.
  bool is_value_truish() const noexcept /* clang-format off */
      requires( std::is_convertible_v<T, bool> ) {
                                        /* clang-format on */
    return has_value() && static_cast<bool>( **this );
  }

  /**************************************************************
  ** Monadic Interface: get_if
  ***************************************************************/
  // This is when the value type is a std::variant.
  template<typename Alt> /* clang-format off */
  auto get_if() noexcept
    -> maybe<mp::const_if_t<Alt, std::is_const_v<T>>&>
    requires(
        requires { std::get_if<Alt>(std::declval<T*>()); } ) {
    /* clang-format on */
    if( !has_value() ) return nothing;
    auto* p = std::get_if<Alt>( &( **this ) );
    if( p == nullptr ) return nothing;
    return *p;
  }

  template<typename Alt> /* clang-format off */
  auto get_if() const noexcept
    -> maybe<mp::const_if_t<Alt, std::is_const_v<T>>&>
    requires(
        requires { std::get_if<Alt>(std::declval<T*>()); } ) {
    /* clang-format on */
    if( !has_value() ) return nothing;
    auto* p = std::get_if<Alt>( &( **this ) );
    if( p == nullptr ) return nothing;
    return *p;
  }

  /**************************************************************
  ** Monadic Interface: inner_if
  ***************************************************************/
  // This is when the value type is a std::variant.
  template<typename Alt>
  auto inner_if() noexcept
  requires requires { std::get_if<Alt>( std::declval<T*>() ); }
  {
    return bind( []( auto& val ) {
      return val.template inner_if<Alt>();
    } );
  }

  template<typename Alt>
  auto inner_if() const noexcept
  requires requires { std::get_if<Alt>( std::declval<T*>() ); }
  {
    return bind( []( auto& val ) {
      return val.template inner_if<Alt>();
    } );
  }

  /**************************************************************
  ** Monadic Interface: member
  ***************************************************************/
  template<typename Func>
  auto member( Func&& func ) const& /* clang-format off */
    -> maybe<std::invoke_result_t<Func, T&>>
    requires( std::is_invocable_v<Func, T&> &&
              std::is_member_object_pointer_v<Func> &&
             !is_maybe_v<std::remove_cvref_t<
               std::invoke_result_t<Func,T&>>> ) {
    /* clang-format on */
    using res_t = maybe<std::invoke_result_t<Func, T&>>;
    res_t res;
    if( has_value() )
      res.assign(
          std::invoke( std::forward<Func>( func ), **this ) );
    return res;
  }

  /**************************************************************
  ** Monadic Interface: maybe_member
  ***************************************************************/
  template<typename Func>
  auto maybe_member( Func&& func ) const /* clang-format off */
    -> maybe<typename mp::const_if_t<
                        typename std::remove_cvref_t<
                          std::invoke_result_t<Func, T&>
                        >::value_type,
                        std::is_const_v<T>
                      >&
                    >
    requires( std::is_member_object_pointer_v<Func> &&
              is_maybe_v<std::remove_cvref_t<
                std::invoke_result_t<Func,T&>>> ) {
    using res_t = maybe<typename mp::const_if_t<
                                  typename std::remove_cvref_t<
                                    std::invoke_result_t<Func, T&>
                                  >::value_type,
                                  std::is_const_v<T>
                                >&
                              >;
    /* clang-format on */
    res_t res;
    if( has_value() ) {
      decltype( auto ) maybe_field =
          std::invoke( std::forward<Func>( func ), **this );
      if( maybe_field.has_value() ) res.assign( *maybe_field );
    }
    return res;
  }

  /**************************************************************
  ** Monadic Interface: fmap
  ***************************************************************/
  template<typename Func>
  auto fmap( Func&& func ) const /* clang-format off */
    -> maybe<std::invoke_result_t<Func, T&>>
    requires( std::is_invocable_v<Func, T&> &&
             !std::is_member_object_pointer_v<Func> &&
             !is_maybe_v<std::remove_cvref_t<
                std::invoke_result_t<Func,T&>>> ) {
    /* clang-format on */
    using res_t = maybe<std::invoke_result_t<Func, T&>>;
    res_t res;
    if( has_value() )
      res.assign(
          std::invoke( std::forward<Func>( func ), **this ) );
    return res;
  }

  /**************************************************************
  ** Monadic Interface: bind
  ***************************************************************/
  template<typename Func>
  auto bind( Func&& func ) const /* clang-format off */
    -> std::remove_cvref_t<std::invoke_result_t<Func, T&>>
    requires( !std::is_member_object_pointer_v<Func> &&
              is_maybe_v<std::remove_cvref_t<
                std::invoke_result_t<Func,T&>>> ) {
    /* clang-format on */
    using res_t =
        std::remove_cvref_t<std::invoke_result_t<Func, T&>>;
    res_t res;
    if( has_value() )
      res.assign(
          std::invoke( std::forward<Func>( func ), **this ) );
    return res;
  }

 private:
  // This allows maybe<T> to access private members of maybe<U>.
  template<typename U>
  requires MaybeTypeRequirements<U>
  friend class maybe;

  /**************************************************************
  ** Converting Value Assignment Operator
  ***************************************************************/
  // We don't want to expose this, because the maybe-ref type is
  // supposed to be immutable, but we need this as a helper, e.g.
  // it is used in the fmap/bind functions.
  template<typename U = T> /* clang-format off */
  constexpr void assign( U& ref ) & noexcept
    requires(
       std::is_convertible_v<U&, T&> &&
      !std::is_same_v<std::remove_cvref_t<U>, std::in_place_t> &&
      !is_maybe_v<std::remove_cvref_t<U>> ) {
    /* clang-format on */
    p_ = static_cast<T*>( &ref );
  }

  /**************************************************************
  ** Converting Assignment Operators
  ***************************************************************/
  // We don't want to expose this, because the maybe-ref type is
  // supposed to be immutable, but we need this as a helper, e.g.
  // it is used in the fmap/bind functions.
  template<typename U> /* clang-format off */
  constexpr void assign( maybe<U&> const& other ) &
      noexcept
      requires( std::is_convertible_v<U&, T&> &&
               !std::is_convertible_v<maybe<U&>&, T&> ) {
                       /* clang-format on */
    p_ = static_cast<T*>( other.p_ );
  }

  template<typename U> /* clang-format off */
  constexpr void assign( maybe<U&>&& other ) &
      noexcept
      requires( std::is_convertible_v<U&, T&> &&
               !std::is_convertible_v<maybe<U&>&, T&> ) {
                       /* clang-format on */
    p_ = static_cast<T*>( other.p_ );
  }

  /**************************************************************
  ** Storage
  ***************************************************************/
  T* p_ = nullptr;
};

/****************************************************************
** make_maybe
*****************************************************************/
template<typename T>
[[nodiscard]] constexpr maybe<std::decay_t<T>> make_maybe(
    T&& val ) {
  return maybe<std::decay_t<T>>( std::forward<T>( val ) );
}

template<typename T, typename... Args>
[[nodiscard]] constexpr maybe<T> make_maybe( Args&&... args ) {
  return maybe<T>( std::in_place,
                   std::forward<Args>( args )... );
}

/****************************************************************
** just
*****************************************************************/
// Will infer T.
template<typename T>
maybe<std::decay_t<T>> just( T&& arg )
requires( !is_maybe_v<std::remove_cvref_t<T>> &&
          !is_nothing_v<std::remove_cvref_t<T>> )
{
  return maybe<std::decay_t<T>>( std::forward<T>( arg ) );
}

// Requires specifying T explicitly.
template<typename T, typename... Args>
maybe<T> just( std::in_place_t, Args&&... args ) {
  return maybe<T>( std::in_place,
                   std::forward<Args>( args )... );
}

template<typename T>
maybe<T&> just_ref( T& arg )
requires( !is_maybe_v<std::remove_cvref_t<T>> &&
          !is_nothing_v<std::remove_cvref_t<T>> )
{
  return maybe<T&>( arg );
}

/****************************************************************
** Equality with maybe
*****************************************************************/
template<typename T, typename U>
requires std::equality_comparable_with<T, U>
[[nodiscard]] constexpr bool operator==( maybe<T> const& lhs,
                                         maybe<U> const& rhs ) //
    noexcept( noexcept( *lhs == *rhs ) ) {
  bool l = lhs.has_value();
  bool r = rhs.has_value();
  return ( l != r ) ? false : l ? ( *lhs == *rhs ) : true;
}

/****************************************************************
** Equality with nothing
*****************************************************************/
template<typename T>
[[nodiscard]] constexpr bool operator==( maybe<T> const& lhs,
                                         nothing_t ) noexcept {
  return !lhs.has_value();
}

template<typename T>
[[nodiscard]] constexpr bool operator==(
    nothing_t, maybe<T> const& rhs ) noexcept {
  return !rhs.has_value();
}

/****************************************************************
** Equality with value
*****************************************************************/
template<typename T, typename U> /* clang-format off */
[[nodiscard]] constexpr bool operator==( maybe<T> const& opt,
                                         U const&        val )
    noexcept( noexcept( *opt == val ) ) { /* clang-format on */
  if( !opt.has_value() ) return false;
  return ( *opt == val );
}

template<typename T, typename U> /* clang-format off */
[[nodiscard]] constexpr bool operator==( U const&        val,
                                         maybe<T> const& opt )
    noexcept( noexcept( val == *opt ) ) { /* clang-format on */
  if( !opt.has_value() ) return false;
  return ( val == *opt );
}

/****************************************************************
** to_str
*****************************************************************/
template<Show T>
void to_str( maybe<T> const& m, std::string& out,
             tag<maybe<T>> ) {
  if( m.has_value() )
    to_str( *m, out );
  else
    out += "nothing";
}

inline void to_str( nothing_t const&, std::string& out,
                    tag<nothing_t> ) {
  out += "nothing";
}

} // namespace base

/****************************************************************
** std::swap
*****************************************************************/
namespace std {

// As usual, if this version is not selected to be part of the
// overload set then a generic std::swap will be used; either way
// std::swap should work so long as std::swap<T> would work.
template<typename T> /* clang-format off */
void swap( ::base::maybe<T>& lhs, ::base::maybe<T>& rhs )
    noexcept( noexcept( lhs.swap( rhs ) ) )
    requires( is_move_constructible_v<T> &&
              is_swappable_v<T> )
/* clang-format on */ {
  lhs.swap( rhs );
}

template<typename T>
struct hash<::base::maybe<T>> {
  auto operator()( ::base::maybe<T> const& m ) const noexcept {
    if( !m ) return hash<bool>{}( false );
    return hash<T>{}( *m );
  }
};

} // namespace std

/****************************************************************
** {fmt}
*****************************************************************/
// {fmt} formatter for formatting maybes whose contained
// type is formattable.
template<typename T> /* clang-format off */
requires( bool( ::fmt::has_formatter<
        std::remove_cvref_t<T>, ::fmt::format_context>() ) )
struct fmt::
    formatter<base::maybe<T>> : fmt::formatter<std::string> {
  /* clang-format on */
  using formatter_base = fmt::formatter<std::string>;
  template<typename FormatContext>
  auto format( base::maybe<T> const& o,
               FormatContext&        ctx ) const {
    static const std::string nothing_str( "nothing" );
    return formatter_base::format(
        o.has_value() ? fmt::format( "{}", *o ) : nothing_str,
        ctx );
  }
};

// {fmt} formatter for formatting nothing_t.
template<>
struct fmt::formatter<base::nothing_t>
  : fmt::formatter<std::string> {
  using formatter_base = fmt::formatter<std::string>;
  template<typename FormatContext>
  auto format( base::nothing_t const&,
               FormatContext& ctx ) const {
    static const std::string nothing_str( "nothing" );
    return formatter_base::format( nothing_str, ctx );
  }
};