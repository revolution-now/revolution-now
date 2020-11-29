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

#include <stdexcept>
#include <type_traits>
#include <utility>

namespace base {

// The class template std::optional manages an optional contained
// value, i.e. a value that may or may not be present.
//
// A common use case for optional is the return value of a func-
// tion that may fail. As opposed to other approaches, such as
// std::pair<T,bool>, optional handles expensive-to-construct ob-
// jects well and is more readable, as the intent is expressed
// explicitly.
//
// Any instance of optional<T> at any given point in time either
// contains a value or does not contain a value.
//
// If an optional<T> contains a value, the value is guaranteed to
// be allocated as part of the optional object footprint, i.e. no
// dynamic memory allocation ever takes place. Thus, an optional
// object models an object, not a pointer, even though opera-
// tor*() and operator->() are defined.
//
// When an object of type optional<T> is contextually converted
// to bool, the conversion returns true if the object contains a
// value and false if it does not contain a value.
//
// The optional object contains a value in the following condi-
// tions:
//
// - The object is initialized with/assigned from a value of type
//   T or another optional that contains a value.
//
// The object does not contain a value in the following condi-
// tions:
//
// - The object is default-initialized.
// - The object is initialized with/assigned from a value of type
//   std::nullopt_t or an optional object that does not contain a
//   value.
// - The member function reset() is called.
//
// There are no optional references; a program is ill-formed if
// it instantiates an optional with a reference type. Alterna-
// tively, an optional of a std::reference_wrapper of type T may
// be used to hold a reference. In addition, a program is
// ill-formed if it instantiates an optional with the (possibly
// cv-qualified) tag types std::nullopt_t or std::in_place_t.
template<typename T>
class maybe {
public:
  /**************************************************************
  ** Destruction
  ***************************************************************/
private:
  void destroy() {
    if( active_ ) val_.~T();
  }

public:
  ~maybe() { destroy(); }

  /**************************************************************
  ** Default Constructor
  ***************************************************************/
  maybe() {}

  /**************************************************************
  ** Value Constructors
  ***************************************************************/
  maybe( T const& val ) noexcept(
      std::is_nothrow_copy_constructible_v<T> )
    : active_{ true } {
    new( &val_ ) T( val );
  }

  maybe( T&& val ) noexcept(
      std::is_nothrow_move_constructible_v<T> )
    : active_{ true } {
    new( &val_ ) T( std::move( val ) );
  }

  /**************************************************************
  ** Converting Value Constructor
  ***************************************************************/
  template<typename U, typename = std::enable_if_t<
                           std::is_convertible_v<U, T>, void> >
  maybe( U&& val ) noexcept(
      std::is_nothrow_convertible_v<U, T> )
    : active_{ true } {
    new( &val_ ) T( std::forward<U>( val ) );
  }

  /**************************************************************
  ** Copy Constructors
  ***************************************************************/
  maybe( maybe<T> const& other ) noexcept(
      std::is_nothrow_copy_constructible_v<T> ) {
    active_ = other.active_;
    if( active_ ) new( &val_ ) T( other.val_ );
  }

  /**************************************************************
  ** Move Constructors
  ***************************************************************/
  maybe( maybe<T>&& other ) noexcept(
      std::is_nothrow_move_constructible_v<T> ) {
    active_ = other.active_;
    if( active_ ) new( &val_ ) T( std::move( other.val_ ) );
    other.reset();
  }

  /**************************************************************
  ** Copy Assignment
  ***************************************************************/
  maybe<T>& operator=( maybe<T> const& rhs ) & {
    destroy();
    active_ = rhs.active_;
    if( rhs.active_ ) new( &val_ ) T( rhs.val_ );
    return *this;
  }

  /**************************************************************
  ** Move Assignment
  ***************************************************************/
  maybe<T>& operator=( maybe<T>&& rhs ) & {
    destroy();
    active_ = rhs.active_;
    if( rhs.active_ ) new( &val_ ) T( std::move( rhs.val_ ) );
    rhs.reset();
    return *this;
  }

  /**************************************************************
  ** Converting Assignment
  ***************************************************************/
  // template<typename U, typename = std::enable_if_t<
  //                          std::is_convertible_v<U, T>,
  //                          void> >
  // maybe<T>&
  // operator=( maybe<U>&& rhs ) & noexcept(
  //     std::is_nothrow_convertible_v<U, T> ) {
  //   destroy();
  //   active_ = rhs.active_;
  //   if( rhs.active_ )
  //     new( &val_ ) T( std::forward<maybe<U>>( rhs ).val_ );
  //   rhs.reset();
  //   return *this;
  // }

  /**************************************************************
  ** Converting Value Assignment
  ***************************************************************/
  template<typename U, typename = std::enable_if_t<
                           std::is_convertible_v<U, T>, void> >
  maybe<T>& operator=( U&& rhs ) & noexcept(
      std::is_nothrow_convertible_v<U, T> ) {
    destroy();
    active_ = true;
    if( active_ ) new( &val_ ) T( std::forward<U>( rhs ) );
    return *this;
  }

  /**************************************************************
  **
  ***************************************************************/
  void reset() {
    destroy();
    active_ = false;
  }

  bool has_value() const noexcept { return active_; }

  T&       operator*() noexcept { return val_; }
  T const& operator*() const noexcept { return val_; }

  T*       operator->() noexcept { return &val_; }
  T const* operator->() const noexcept { return &val_; }

  T& value() {
    if( !active_ )
      throw std::runtime_error(
          "value() called on inactive-maybe." );
    return val_;
  }

  T const& value() const {
    if( !active_ )
      throw std::runtime_error(
          "value() called on inactive-maybe." );
    return val_;
  }

private:
  bool active_ = false;
  union {
    T val_;
  };
};

} // namespace base