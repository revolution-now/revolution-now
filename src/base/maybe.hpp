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

namespace base {

/****************************************************************
** nothing_t
*****************************************************************/
struct nothing_t {
  constexpr bool operator==( nothing_t const& ) const = default;
};

inline constexpr nothing_t nothing;

/****************************************************************
** bad_maybe_access exception
*****************************************************************/
struct bad_maybe_access : public std::exception {
  bad_maybe_access( SourceLoc loc = SourceLoc{} )
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
class maybe { /* clang-format on */
public:
  /**************************************************************
  ** Types
  ***************************************************************/
  using value_type = T;

  /**************************************************************
  ** Default Constructor
  ***************************************************************/
  // This does not initialize the union member.
  constexpr maybe() noexcept : active_{ false } {}

  constexpr maybe( nothing_t ) noexcept : maybe() {}

  /**************************************************************
  ** Destruction
  ***************************************************************/
private:
  void destroy() {
    if constexpr( !std::is_trivially_destructible_v<T> ) {
      if( active_ ) val().~T();
      // Don't set active_ to false here. That is the purpose of
      // this function. If you want to destroy+deactivate then
      // call the public reset() function.
    }
  }

public:
  // FIXME: If T is trivially destructible then this destructor
  // should also be trivial, making optional<T> also trivially
  // destructible. This is not simple to do currently; it re-
  // quires tricky conditional base classes. But the following
  // proposal, if accepted, would make it easy:
  //
  //   http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0848r3.html
  //
  ~maybe() { destroy(); }

  /**************************************************************
  ** Value Constructors
  ***************************************************************/
  maybe( T const& val ) /* clang-format off */
      noexcept( std::is_nothrow_copy_constructible_v<T> )
      requires( std::is_copy_constructible_v<T> )
    : active_{ true } /* clang-format on */ {
    new_val( val );
  }

  maybe( T&& val ) /* clang-format off */
      noexcept( std::is_nothrow_move_constructible_v<T> )
      requires( std::is_move_constructible_v<T> )
    : active_{ true } /* clang-format on */ {
    new_val( std::move( val ) );
  }

  /**************************************************************
  ** Converting Value Constructor
  ***************************************************************/
  template<
      typename U,
      typename = std::enable_if_t<
          std::is_constructible_v<T, U&&> &&
          !std::is_same_v<std::remove_cvref_t<U>,
                          std::in_place_t> &&
          !std::is_same_v<std::remove_cvref_t<U>, maybe<T>>>>
  explicit( !std::is_convertible_v<U&&, T> ) //
      maybe( U&& val ) noexcept(
          std::is_nothrow_convertible_v<U, T> )
    : active_{ true } {
    new_val( std::forward<U>( val ) );
  }

  /**************************************************************
  ** Copy Constructors
  ***************************************************************/
  constexpr maybe( maybe<T> const& other ) noexcept(
      std::is_nothrow_copy_constructible_v<T> ) {
    active_ = other.active_;
    if( active_ ) new_val( *other );
  }

  /**************************************************************
  ** Move Constructors
  ***************************************************************/
  maybe( maybe<T>&& other ) noexcept(
      std::is_nothrow_move_constructible_v<T> )
    : active_{ false } {
    this->swap( other );
  }

  /**************************************************************
  ** In-place construction
  ***************************************************************/
  template<typename... Args> /* clang-format off */
  constexpr explicit maybe( std::in_place_t, Args&&... args )
      requires( std::is_constructible_v<T, Args...> )
    : active_{ true } /* clang-format on */ {
    new_val( std::forward<Args>( args )... );
  }

  template<typename U, typename... Args> /* clang-format off */
  constexpr explicit maybe( std::in_place_t,
                            std::initializer_list<U> ilist,
                            Args&&... args )
      requires( std::is_constructible_v<
                    T,
                    std::initializer_list<U>&,
                    Args...
                > )
    : active_{ true } /* clang-format on */ {
    new_val( ilist, std::forward<Args>( args )... );
  }

  /**************************************************************
  ** Copy Assignment
  ***************************************************************/
  /* clang-format off */
  constexpr maybe<T>& operator=( maybe<T> const& rhs ) &
      noexcept( noexcept( new_val( *rhs ) ) ) {
    /* clang-format on */
    destroy();
    active_ = rhs.active_;
    if( rhs.active_ ) new_val( *rhs );
    return *this;
  }

  /**************************************************************
  ** Move Assignment
  ***************************************************************/
  constexpr maybe<T>& operator=( maybe<T>&& rhs ) & noexcept(
      noexcept( this->swap( rhs ) ) ) {
    reset();
    this->swap( rhs );
    return *this;
  }

  /**************************************************************
  ** Converting Assignment
  ***************************************************************/
  template<typename U, typename = std::enable_if_t<
                           std::is_convertible_v<U&&, T>, void>
           // TODO: a lot more conditions need to be added here.
           >
  maybe<T>& operator=( maybe<U>&& rhs ) & noexcept(
      std::is_nothrow_convertible_v<U&&, T> ) {
    destroy();
    active_ = rhs.active_;
    if( active_ ) new_val( std::move( *rhs ) );
    // Note: this reference goes against what cppreference says,
    // since it says that a moved-from optional still has a
    // value. But for the `maybe` type we will deviate from that
    // because it just doesn't sound right.
    rhs.reset();
    return *this;
  }

  template<typename U,
           typename = std::enable_if_t<
               std::is_convertible_v<U const&, T>, void>
           // TODO: a lot more conditions need to be added here.
           >
  maybe<T>& operator=( maybe<U> const& rhs ) & noexcept(
      std::is_nothrow_convertible_v<U const&, T> ) {
    destroy();
    active_ = rhs.has_value();
    if( active_ ) new_val( *rhs );
    return *this;
  }

  /**************************************************************
  ** Converting Value Assignment
  ***************************************************************/
  template<typename U, typename = std::enable_if_t<
                           std::is_convertible_v<U, T>>>
  maybe<T>& operator=( U&& rhs ) & noexcept(
      std::is_nothrow_convertible_v<U, T> ) {
    destroy();
    active_ = true;
    if( active_ ) new_val( std::forward<U>( rhs ) );
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
  void swap( maybe<T>& other ) noexcept(
      std::is_nothrow_move_constructible_v<T>&&
          std::is_nothrow_swappable_v<
              T> ) requires( std::is_move_constructible_v<T>&&
                                 std::is_swappable_v<T> ) {
    if( !active_ && !other.active_ ) return;
    if( active_ && other.active_ ) {
      using std::swap;
      swap( **this, *other );
      return;
    }
    // From cppreference: if only one of *this and other contains
    // a value (let's call this object in and the other un), the
    // contained value of un is direct-initialized from std::-
    // move(*in), followed by destruction of the contained value
    // of in as if by in->T::~T(). After this call, in does not
    // contain a value; un contains a value.
    maybe<T>& in = active_ ? *this : other;
    maybe<T>& un = active_ ? other : *this;
    new_val( std::move( *in ) );
    in.reset();
    un.active_ = true;
  }

  /**************************************************************
  ** value
  ***************************************************************/
  T& value( SourceLoc loc = SourceLoc{} ) {
    if( !active_ ) throw bad_maybe_access{ loc };
    return **this;
  }

  T const& value( SourceLoc loc = SourceLoc{} ) const {
    if( !active_ ) throw bad_maybe_access{ loc };
    return **this;
  }

  /**************************************************************
  ** value_or
  ***************************************************************/
  template<typename U>
  // clang-format off
  requires (std::is_convertible_v<U&&, T> &&
            std::is_copy_constructible_v<T>)
  constexpr T value_or( U&& def ) const& {
    // clang-format on
    return has_value()
               ? **this
               : static_cast<T>( std::forward<U>( def ) );
  }

  template<typename U>
  // clang-format off
  requires (std::is_convertible_v<U&&, T> &&
            std::is_move_constructible_v<T>)
  constexpr T value_or( U&& def ) && {
    // clang-format on
    return has_value()
               ? std::move( **this )
               : static_cast<T>( std::forward<U>( def ) );
  }

  /**************************************************************
  ** reset
  ***************************************************************/
  void reset() noexcept {
    destroy();
    active_ = false;
  }

  /**************************************************************
  ** has_value/bool
  ***************************************************************/
  bool has_value() const noexcept { return active_; }

  explicit operator bool() const noexcept { return active_; }

  /**************************************************************
  ** Deference operators
  ***************************************************************/
  T&       operator*() & noexcept { return val(); }
  T const& operator*() const& noexcept { return val(); }

  T&& operator*() && noexcept { return std::move( val() ); }

  T*       operator->() & noexcept { return &**this; }
  T const* operator->() const& noexcept { return &**this; }

  /**************************************************************
  ** emplace
  ***************************************************************/
  template<typename... Args>
  auto emplace( Args&&... args ) noexcept(
      std::is_nothrow_constructible_v<T, Args...> )
      -> std::enable_if_t<std::is_constructible_v<T, Args...>,
                          T&> {
    destroy();
    active_ = true;
    new_val( std::forward<Args>( args )... );
    return **this;
  }

  // clang-format off
  template<typename U, typename... Args>
  auto emplace( std::initializer_list<U> ilist, Args&&... args )
      noexcept( std::is_nothrow_constructible_v<T,
                  std::initializer_list<U>, Args...> )
      -> std::enable_if_t<
           std::is_constructible_v<
               T, std::initializer_list<U>, Args...>,
           T&>
  // clang-format on
  {
    destroy();
    active_ = true;
    new_val( ilist, std::forward<Args>( args )... );
    return **this;
  }

  /**************************************************************
  ** Storage
  ***************************************************************/
private:
  T const& val() const noexcept { return val_; }
  T&       val() noexcept { return val_; }

  template<typename... Vs>
  void new_val( Vs&&... v ) noexcept(
      noexcept( new( &val() ) T( std::forward<Vs>( v )... ) ) ) {
    new( &val() ) T( std::forward<Vs>( v )... );
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
constexpr maybe<std::decay_t<T>> make_maybe( T&& val ) {
  return maybe<std::decay_t<T>>( std::forward<T>( val ) );
}

template<typename T, typename... Args>
constexpr maybe<T> make_maybe( Args&&... args ) {
  return maybe<T>( std::in_place,
                   std::forward<Args>( args )... );
}

template<typename T, typename U, typename... Args>
constexpr maybe<T> make_optional( std::initializer_list<U> ilist,
                                  Args&&... args ) {
  return maybe<T>( std::in_place, ilist,
                   std::forward<Args>( args )... );
}

/****************************************************************
** Equality
*****************************************************************/
template<typename T, typename U,
         typename = std::void_t<decltype( std::declval<T>() ==
                                          std::declval<U>() )>>
bool operator==( maybe<T> const& lhs, maybe<U> const& rhs ) //
    noexcept( noexcept( *lhs == *rhs ) ) {
  bool l = lhs.has_value();
  bool r = rhs.has_value();
  return ( l != r ) ? false : l ? ( *lhs == *rhs ) : true;
}

template<
    typename T, typename U,
    typename = std::void_t<decltype( std::declval<maybe<T>>() ==
                                     std::declval<maybe<U>>() )>>
bool operator!=( maybe<T> const& lhs, maybe<U> const& rhs ) //
    noexcept( noexcept( lhs == rhs ) ) {
  return !( lhs == rhs );
}

template<typename T>
bool operator==( maybe<T> const& lhs, nothing_t ) noexcept {
  return !lhs.has_value();
}

template<typename T>
bool operator!=( maybe<T> const& lhs, nothing_t ) noexcept {
  return lhs.has_value();
}

template<typename T>
bool operator==( nothing_t, maybe<T> const& rhs ) noexcept {
  return !rhs.has_value();
}

template<typename T>
bool operator!=( nothing_t, maybe<T> const& rhs ) noexcept {
  return rhs.has_value();
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
auto swap( ::base::maybe<T>& lhs,
           ::base::maybe<T>& rhs )
       noexcept( noexcept( lhs.swap( rhs ) ) )
    -> enable_if_t<is_move_constructible_v<T> &&
                   is_swappable_v<T>> /* clang-format on*/ {
  lhs.swap( rhs );
}

} // namespace std