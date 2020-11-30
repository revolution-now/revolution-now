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

// base
#include "source-loc.hpp"

// C++ standard library
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

// When clang gets this C++20 feature (p0848r3) then this guard
// can be removed. When this is defined, it will ensure that the
// maybe<T> has trivial special member functions when T does.
#ifndef __clang__
#  define HAS_CONDITIONALLY_TRIVIAL_SPECIAL_MEMBERS
#endif

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
** bad_maybe_access exception
*****************************************************************/
struct bad_maybe_access : public std::exception {
  // Don't give a default value to loc because we want that to be
  // supplied by someone further up the call chain in order to
  // produce a more helpful location to the user.
  bad_maybe_access( SourceLoc loc )
    : std::exception{}, loc_{ std::move( loc ) }, error_msg_{} {
    error_msg_ = loc_.file_name();
    error_msg_ += ":" + std::to_string( loc_.line() ) + ": ";
    error_msg_ += "value() called on an inactive maybe.";
  }

  char const* what() const noexcept override {
    return error_msg_.c_str();
  }

  SourceLoc   loc_;
  std::string error_msg_;
};

/****************************************************************
** Requirements on maybe::value_type
*****************************************************************/
// Normally it wouldn't make sense to turn these into a concept,
// since they're just a random list of requiremnets, but it is
// convenient because they need to be kept in sync between the
// class declaration and the deduction guide.
template<typename T>
concept MaybeTypeRequirements = requires {
  requires(
      !std::is_same_v<std::remove_cvref_t<T>, std::in_place_t> &&
      !std::is_same_v<std::remove_cvref_t<T>, nothing_t> );
};

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
#ifdef HAS_CONDITIONALLY_TRIVIAL_SPECIAL_MEMBERS
  constexpr maybe() requires(
      std::is_trivially_default_constructible_v<T> ) = default;
#endif

  // This does not initialize the union member.
  constexpr maybe() noexcept
#ifdef HAS_CONDITIONALLY_TRIVIAL_SPECIAL_MEMBERS
      requires( !std::is_trivially_default_constructible_v<T> )
#endif
    : active_{ false } {
  }

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
#ifdef HAS_CONDITIONALLY_TRIVIAL_SPECIAL_MEMBERS
  constexpr ~maybe() noexcept
      requires( std::is_trivially_destructible_v<T> ) = default;
  constexpr ~maybe() noexcept
      requires( !std::is_trivially_destructible_v<T> ) {
    destroy_if_active();
  }
#else
  constexpr ~maybe() noexcept { destroy_if_active(); }
#endif

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
    : active_{ true } /* clang-format on */ {
    new_val( val );
  }

  constexpr maybe( T const& val ) /* clang-format off */
      noexcept( std::is_nothrow_copy_constructible_v<T> )
      requires( std::is_copy_constructible_v<T> &&
                std::is_trivially_default_constructible_v<T>)
    : active_{ true }, val_{} /* clang-format on */ {
    new_val( val );
  }

  constexpr maybe( T&& val ) /* clang-format off */
      noexcept( std::is_nothrow_move_constructible_v<T> )
      requires( std::is_move_constructible_v<T> &&
               !std::is_trivially_default_constructible_v<T> )
    : active_{ true } /* clang-format on */ {
    new_val( std::move( val ) );
  }

  constexpr maybe( T&& val ) /* clang-format off */
      noexcept( std::is_nothrow_move_constructible_v<T> )
      requires( std::is_move_constructible_v<T> &&
                std::is_trivially_default_constructible_v<T>)
    : active_{ true }, val_{} /* clang-format on */ {
    new_val( std::move( val ) );
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
    : active_{ true } { /* clang-format on */
    new_val( std::forward<U>( val ) );
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
    : active_{ true }, val_{} { /* clang-format on */
    new_val( std::forward<U>( val ) );
  }

  /**************************************************************
  ** Copy Constructors
  ***************************************************************/
  constexpr maybe( maybe<T> const& other ) /* clang-format off */
      noexcept( noexcept( this->new_val( *other ) ) )
      requires( std::is_copy_constructible_v<T> &&
               !std::is_trivially_copy_constructible_v<T> )
    : active_{ other.active_ } /* clang-format on */ {
    if( active_ ) new_val( *other );
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
    : active_{ other.has_value() } /* clang-format on */ {
    if( active_ ) new_val( *other );
  }

  /**************************************************************
  ** Move Constructors
  ***************************************************************/
  constexpr maybe( maybe<T>&& other ) /* clang-format off */
    noexcept( noexcept( this->new_val( std::move( *other ) ) ) )
    requires( std::is_move_constructible_v<T> &&
             !std::is_trivially_move_constructible_v<T> )
    : active_{ other.active_ } /* clang-format on */ {
    if( active_ ) new_val( std::move( *other ) );
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
    : active_{ other.has_value() } /* clang-format on */ {
    if( active_ ) new_val( std::move( *other ) );
  }

  /**************************************************************
  ** In-place construction
  ***************************************************************/
  template<typename... Args> /* clang-format off */
  constexpr explicit maybe( std::in_place_t, Args&&... args )
      noexcept( noexcept(
            this->new_val( std::forward<Args>( args )... )) )
      requires( std::is_constructible_v<T, Args...> )
    : active_{ true } /* clang-format on */ {
    new_val( std::forward<Args>( args )... );
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
    : active_{ true } /* clang-format on */ {
    new_val( ilist, std::forward<Args>( args )... );
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
      active_ = true;
      new_val( *rhs );
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
        ( noexcept( new_val( std::move( *rhs ) ) )&& noexcept(
            ( **this ) = std::move( *rhs ) ) ) );
    if( !rhs.has_value() ) {
      if( has_value() ) reset();
      return *this;
    }
    // rhs has a value.
    if( !has_value() ) {
      active_ = true;
      new_val( std::move( *rhs ) );
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
      active_ = true;
      new_val( *rhs );
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
      active_ = true;
      new_val( std::move( *rhs ) );
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
      active_ = true;
      new_val( std::forward<U>( rhs ) );
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
    if( !active_ && !other.active_ ) return;
    if( active_ && other.active_ ) {
      using std::swap;
      swap( **this, *other );
      return;
    }
    // One is active and the other is not.
    maybe<T>& active   = has_value() ? *this : other;
    maybe<T>& inactive = has_value() ? other : *this;
    inactive.active_   = true;
    inactive.new_val( std::move( *active ) );
    active.reset();
  }

  /**************************************************************
  ** value
  ***************************************************************/
  [[nodiscard]] constexpr T const& value(
      SourceLoc loc = SourceLoc::current() ) const& {
    if( !active_ ) throw bad_maybe_access{ loc };
    return **this;
  }
  [[nodiscard]] constexpr T& value(
      SourceLoc loc = SourceLoc::current() ) & {
    if( !active_ ) throw bad_maybe_access{ loc };
    return **this;
  }

  [[nodiscard]] constexpr T const&& value(
      SourceLoc loc = SourceLoc::current() ) const&& {
    if( !active_ ) throw bad_maybe_access{ loc };
    return std::move( **this );
  }
  [[nodiscard]] constexpr T&& value(
      SourceLoc loc = SourceLoc::current() ) && {
    if( !active_ ) throw bad_maybe_access{ loc };
    return std::move( **this );
  }

  /**************************************************************
  ** value_or
  ***************************************************************/
  template<typename U>
  // clang-format off
  [[nodiscard]] constexpr T value_or( U&& def ) const&
    noexcept( std::is_nothrow_convertible_v<U&&, T> &&
              std::is_nothrow_copy_constructible_v<T>)
    requires( std::is_convertible_v<U&&, T> &&
              std::is_copy_constructible_v<T> ) {
    // clang-format on
    return has_value()
               ? **this
               : static_cast<T>( std::forward<U>( def ) );
  }

  template<typename U>
  // clang-format off
  [[nodiscard]] constexpr T value_or( U&& def ) &&
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

  [[nodiscard]] constexpr explicit operator bool()
      const noexcept {
    return active_;
  }

  /**************************************************************
  ** Deference operators
  ***************************************************************/
  constexpr T const& operator*() const& noexcept { return val_; }
  constexpr T&       operator*() & noexcept { return val_; }

  constexpr T const&& operator*() const&& noexcept {
    return std::move( val_ );
  }
  constexpr T&& operator*() && noexcept {
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
  ** Storage
  ***************************************************************/
private:
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

  bool active_ = false;
  union {
    T val_;
  };
};

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
** Equality with maybe
*****************************************************************/
template<typename T, typename U>
[[nodiscard]] constexpr bool operator==( maybe<T> const& lhs,
                                         maybe<U> const& rhs ) //
    noexcept( noexcept( *lhs == *rhs ) ) {
  bool l = lhs.has_value();
  bool r = rhs.has_value();
  return ( l != r ) ? false : l ? ( *lhs == *rhs ) : true;
}

// Delegate to the above.
template<typename T, typename U>
[[nodiscard]] constexpr bool operator!=( maybe<T> const& lhs,
                                         maybe<U> const& rhs ) //
    noexcept( noexcept( /*don't dereference*/ lhs == rhs ) ) {
  return !( lhs == rhs );
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
[[nodiscard]] constexpr bool operator!=( maybe<T> const& lhs,
                                         nothing_t ) noexcept {
  return lhs.has_value();
}

template<typename T>
[[nodiscard]] constexpr bool operator==(
    nothing_t, maybe<T> const& rhs ) noexcept {
  return !rhs.has_value();
}

template<typename T>
[[nodiscard]] constexpr bool operator!=(
    nothing_t, maybe<T> const& rhs ) noexcept {
  return rhs.has_value();
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

template<typename T, typename U> /* clang-format off */
[[nodiscard]] constexpr bool operator!=( maybe<T> const& opt,
                           U const&        val )
    noexcept( noexcept( opt == val ) ) { /* clang-format on */
  return !( opt == val );
}

template<typename T, typename U> /* clang-format off */
[[nodiscard]] constexpr bool operator!=( U const&        val,
                           maybe<T> const& opt )
    noexcept( noexcept( val == opt ) ) { /* clang-format on */
  return !( val == opt );
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
              is_swappable_v<T> ) /* clang-format on*/ {
  lhs.swap( rhs );
}

} // namespace std