/****************************************************************
**expect.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-05.
*
* Description: A sum type that either holds a value or error.
*
*****************************************************************/
#pragma once

// base
#include "maybe.hpp"
#include "meta.hpp"
#include "source-loc.hpp"

// C++ standard library
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace base {

/****************************************************************
** Requirements on expect::value_type
*****************************************************************/
// Normally it wouldn't make sense to turn these into a concept,
// since they're just a random list of requiremnets, but it is
// convenient because they need to be kept in sync between the
// class declaration, the deduction guide, and the friend decla-
// ration.
template<typename T, typename E>
concept ExpectTypeRequirements = requires {
  requires(
      !std::is_same_v<std::remove_cvref_t<T>, std::in_place_t> &&
      !std::is_same_v<std::remove_cvref_t<E>, std::in_place_t> &&
      !std::is_same_v<std::remove_cvref_t<T>, nothing_t> &&
      !std::is_same_v<std::remove_cvref_t<E>, nothing_t> &&
      !std::is_same_v<std::remove_cvref_t<T>,
                      std::remove_cvref_t<E>> );
};

/****************************************************************
** Forward Declaration
*****************************************************************/
template<typename T, typename E>
requires ExpectTypeRequirements<T, E> class [[nodiscard]] expect;

/****************************************************************
** bad_expect_access exception
*****************************************************************/
struct bad_expect_access : public std::exception {
  // Don't give a default value to loc because we want that to be
  // supplied by someone further up the call chain in order to
  // produce a more helpful location to the user.
  bad_expect_access( SourceLoc loc )
    : std::exception{}, loc_{ std::move( loc ) }, error_msg_{} {
    error_msg_ = loc_.file_name();
    error_msg_ += ":" + std::to_string( loc_.line() ) + ": ";
    error_msg_ += "value() called on an inactive expect.";
  }

  char const* what() const noexcept override {
    return error_msg_.c_str();
  }

  SourceLoc   loc_;
  std::string error_msg_;
};

/****************************************************************
** Basic metaprogramming helpers.
*****************************************************************/
template<typename T>
constexpr bool is_expect_v = false;
template<typename T, typename E>
constexpr bool is_expect_v<expect<T, E>> = true;

/****************************************************************
** expect
*****************************************************************/
template<typename T, typename E>
requires ExpectTypeRequirements<T, E> /* clang-format off */
class [[nodiscard]] expect { /* clang-format on */
public:
  /**************************************************************
  ** Types
  ***************************************************************/
  using value_type = T;
  using error_type = E;

  /**************************************************************
  ** Default Constructor
  ***************************************************************/
  expect() = delete;

  /**************************************************************
  ** Destruction
  ***************************************************************/
private:
  constexpr void destroy_val() {
    if constexpr( !std::is_trivially_destructible_v<T> )
      val_.T::~T();
  }

  constexpr void destroy_err() {
    if constexpr( !std::is_trivially_destructible_v<E> )
      err_.E::~E();
  }

  constexpr void destroy() {
    if( good_ )
      destroy_val();
    else
      destroy_err();
  }

public:
  constexpr ~expect() noexcept { destroy(); }

  /**************************************************************
  ** Value Constructors
  ***************************************************************/
  constexpr expect( T const& val ) /* clang-format off */
      noexcept( std::is_nothrow_copy_constructible_v<T> )
      requires( std::is_copy_constructible_v<T> )
    : good_{ false } /* clang-format on */ {
    new_val( val );
    good_ = true;
  }

  constexpr expect( T&& val ) /* clang-format off */
      noexcept( std::is_nothrow_move_constructible_v<T> )
      requires( std::is_move_constructible_v<T> )
    : good_{ false } /* clang-format on */ {
    new_val( std::move( val ) );
    good_ = true;
  }

  constexpr expect( E const& err ) /* clang-format off */
      noexcept( std::is_nothrow_copy_constructible_v<E> )
      requires( std::is_copy_constructible_v<E> )
    : good_{ false } /* clang-format on */ {
    new_err( err );
    good_ = false;
  }

  constexpr expect( E&& err ) /* clang-format off */
      noexcept( std::is_nothrow_move_constructible_v<E> )
      requires( std::is_move_constructible_v<E> )
    : good_{ false } /* clang-format on */ {
    new_err( std::move( err ) );
    good_ = false;
  }

  /**************************************************************
  ** Converting Value Constructor
  ***************************************************************/
  template<typename U> /* clang-format off */
  explicit( !std::is_convertible_v<U&&, T> )
  constexpr expect( U&& val )
      noexcept( std::is_nothrow_convertible_v<U, T> )
      requires(
         std::is_constructible_v<T, U&&> &&
        !std::is_same_v<std::remove_cvref_t<U>, E> &&
        !std::is_same_v<std::remove_cvref_t<U>, T> &&
        !std::is_same_v<std::remove_cvref_t<U>, std::in_place_t> &&
        !std::is_same_v<std::remove_cvref_t<U>, expect<T,E>> )
    : good_{ false } { /* clang-format on */
    new_val( std::forward<U>( val ) );
    // Now set to true after no exception has happened.
    good_ = true;
  }

  /**************************************************************
  ** Copy Constructors
  ***************************************************************/
  constexpr expect(
      expect<T, E> const& other ) /* clang-format off */
      noexcept( noexcept( this->new_val( other.val_ ) ) &&
                noexcept( this->new_err( other.err_ ) ) )
      requires( std::is_copy_constructible_v<T> &&
                std::is_copy_constructible_v<E> )
    : good_{ false } /* clang-format on */ {
    if( other.has_value() )
      new_val( other.val_ );
    else
      new_err( other.err_ );
    // Now set after no exception has happened.
    good_ = other.good_;
  }

  /**************************************************************
  ** Converting Copy Constructors
  ***************************************************************/
  template<typename U, typename V> /* clang-format off */
  explicit( !std::is_convertible_v<U const&, T> ||
            !std::is_convertible_v<V const&, E> )
  constexpr expect( expect<U, V> const& other )
      noexcept( noexcept( this->new_val( other.val_ ) ) &&
                noexcept( this->new_err( other.err_ ) ) )
      requires( std::is_constructible_v<T, const U&>            &&
                std::is_constructible_v<E, const V&>            &&
               !std::is_constructible_v<T, expect<U,V>&>        &&
               !std::is_constructible_v<T, expect<U,V> const&>  &&
               !std::is_constructible_v<T, expect<U,V>&&>       &&
               !std::is_constructible_v<T, expect<U,V> const&&> &&
               !std::is_convertible_v<expect<U,V>&, T>          &&
               !std::is_convertible_v<expect<U,V> const&, T>    &&
               !std::is_convertible_v<expect<U,V>&&, T>         &&
               !std::is_convertible_v<expect<U,V> const&&, T>   &&
               !std::is_constructible_v<E, expect<U,V>&>        &&
               !std::is_constructible_v<E, expect<U,V> const&>  &&
               !std::is_constructible_v<E, expect<U,V>&&>       &&
               !std::is_constructible_v<E, expect<U,V> const&&> &&
               !std::is_convertible_v<expect<U,V>&, E>          &&
               !std::is_convertible_v<expect<U,V> const&, E>    &&
               !std::is_convertible_v<expect<U,V>&&, E>         &&
               !std::is_convertible_v<expect<U,V> const&&, E> )
    : good_{ false } /* clang-format on */ {
    if( other.has_value() )
      new_val( other.val_ );
    else
      new_err( other.err_ );
    // Now set after no exception has happened.
    good_ = other.good_;
  }

  /**************************************************************
  ** Move Constructors
  ***************************************************************/
  constexpr expect( expect<T, E>&& other ) /* clang-format off */
    noexcept( noexcept( this->new_val( std::move( other.val_ ) ) ) &&
              noexcept( this->new_err( std::move( other.err_ ) ) ) )
    requires( std::is_move_constructible_v<T> &&
              std::is_move_constructible_v<E> )
    : good_{ false } /* clang-format on */ {
    if( other.has_value() )
      new_val( std::move( other.val_ ) );
    else
      new_err( std::move( other.err_ ) );
    // Now set after no exception has happened.
    good_ = other.good_;
    static_assert(
        std::is_nothrow_move_constructible_v<T> ==
        noexcept( this->new_val( std::move( other.val_ ) ) ) );
    static_assert(
        std::is_nothrow_move_constructible_v<E> ==
        noexcept( this->new_err( std::move( other.err_ ) ) ) );
  }

  /**************************************************************
  ** Converting Move Constructors
  ***************************************************************/
  template<typename U, typename V> /* clang-format off */
  explicit( !std::is_convertible_v<U&&, T> ||
            !std::is_convertible_v<V&&, E> )
  constexpr expect( expect<U, V>&& other )
      noexcept(
          noexcept( this->new_val( std::move( other.val_ ) ) ) &&
          noexcept( this->new_val( std::move( other.err_ ) ) ) )
      requires( std::is_constructible_v<T, U&&>                 &&
                std::is_constructible_v<E, V&&>                 &&
               !std::is_constructible_v<T, expect<U,V>&>        &&
               !std::is_constructible_v<T, expect<U,V> const&>  &&
               !std::is_constructible_v<T, expect<U,V>&&>       &&
               !std::is_constructible_v<T, expect<U,V> const&&> &&
               !std::is_convertible_v<expect<U,V>&, T>          &&
               !std::is_convertible_v<expect<U,V> const&, T>    &&
               !std::is_convertible_v<expect<U,V>&&, T>         &&
               !std::is_convertible_v<expect<U,V> const&&, T>   &&
               !std::is_constructible_v<E, expect<U,V>&>        &&
               !std::is_constructible_v<E, expect<U,V> const&>  &&
               !std::is_constructible_v<E, expect<U,V>&&>       &&
               !std::is_constructible_v<E, expect<U,V> const&&> &&
               !std::is_convertible_v<expect<U,V>&, E>          &&
               !std::is_convertible_v<expect<U,V> const&, E>    &&
               !std::is_convertible_v<expect<U,V>&&, E>         &&
               !std::is_convertible_v<expect<U,V> const&&, E> )
    : good_{ false } /* clang-format on */ {
    if( other.has_value() )
      new_val( std::move( other.val_ ) );
    else
      new_err( std::move( other.err_ ) );
    // Now set after no exception has happened.
    good_ = other.good_;
  }

  /**************************************************************
  ** In-place construction
  ***************************************************************/
  template<typename... Args> /* clang-format off */
  constexpr explicit expect( std::in_place_t, Args&&... args )
      noexcept( noexcept(
            this->new_val( std::forward<Args>( args )... ) ) )
      requires( std::is_constructible_v<T, Args...> )
    : good_{ false } /* clang-format on */ {
    new_val( std::forward<Args>( args )... );
    // Now set after no exception has happened.
    good_ = true;
  }

  template<typename U, typename... Args> /* clang-format off */
  constexpr explicit expect( std::in_place_t,
                             std::initializer_list<U> ilist,
                             Args&&... args )
      noexcept( noexcept(
        this->new_val( ilist, std::forward<Args>( args )... )) )
      requires( std::is_constructible_v<
                    T,
                    std::initializer_list<U>&,
                    Args&&...
                > )
    : good_{ false } /* clang-format on */ {
    new_val( ilist, std::forward<Args>( args )... );
    // Now set after no exception has happened.
    good_ = true;
  }

  /**************************************************************
  ** Implicit Conversions
  ***************************************************************/
  // If T is a reference_wrapper then allow implicit conversions
  // to expect<T::value_type&>.
  template<typename U>
  constexpr operator expect<U&, E>() const noexcept
      requires( mp::is_reference_wrapper_v<T>&&
                    std::is_copy_constructible_v<E> ) {
    if( !good_ ) return err_;
    return val_.get();
  }

  // Always allow implici conversions to expect<T&, E>.
  template<typename U>
  constexpr operator expect<U const&, E>() const noexcept
      requires( !mp::is_reference_wrapper_v<T> &&
                std::is_convertible_v<T const&, U const&> &&
                std::is_copy_constructible_v<E> ) {
    if( !good_ ) return err_;
    return val_;
  }

  template<typename U>
  constexpr operator expect<U&, E>() noexcept
      requires( !mp::is_reference_wrapper_v<T> &&
                std::is_convertible_v<T&, U&> &&
                std::is_copy_constructible_v<E> ) {
    if( !good_ ) return err_;
    return val_;
  }

  /**************************************************************
  ** Copy Assignment
  ***************************************************************/
  /* clang-format off */
  constexpr expect<T, E>& operator=( expect<T, E> const& rhs ) &
    noexcept( std::is_nothrow_copy_constructible_v<T> &&
              std::is_nothrow_copy_assignable_v<T>    &&
              std::is_nothrow_copy_constructible_v<E> &&
              std::is_nothrow_copy_assignable_v<E> )
    requires( std::is_copy_constructible_v<T> &&
              std::is_copy_assignable_v<T> &&
              std::is_copy_constructible_v<E> &&
              std::is_copy_assignable_v<E> ) {
    /* clang-format on */
    if( !rhs.good_ ) {
      if( !good_ ) {
        err_ = rhs.err_;
        return *this;
      }
      // this has a value.
      destroy();
      new_err( rhs.err_ );
      good_ = false;
      return *this;
    }
    // rhs has a value.
    if( !good_ ) {
      destroy();
      new_val( rhs.val_ );
      // Now set after no exception has happened.
      good_ = true;
      return *this;
    }
    // both have values.
    val_ = rhs.val_;
    return *this;
  }

  /**************************************************************
  ** Move Assignment
  ***************************************************************/
  /* clang-format off */
  constexpr expect<T, E>& operator=( expect<T, E>&& rhs ) &
    noexcept( std::is_nothrow_move_constructible_v<T> &&
              std::is_nothrow_move_assignable_v<T>    &&
              std::is_nothrow_move_constructible_v<E> &&
              std::is_nothrow_move_assignable_v<E> )
    requires( std::is_move_constructible_v<T> &&
              std::is_move_assignable_v<T>    &&
              std::is_move_constructible_v<E> &&
              std::is_move_assignable_v<E> ) {
    // Sanity check that the `noexcept` value that the compiler
    // derives from the functions we're calling (the ones that
    // could throw) is the same as the noexcept spec that we've
    // supplied above, and which is dictated by cppreference.
    static_assert(
        (std::is_nothrow_move_constructible_v<T> &&
         std::is_nothrow_move_assignable_v<T>    &&
         std::is_nothrow_move_constructible_v<E> &&
         std::is_nothrow_move_assignable_v<E>) ==
        (noexcept( new_val( std::move( rhs.val_ ) ) ) &&
         noexcept( new_err( std::move( rhs.err_ ) ) ) &&
         noexcept( val_   = std::move( rhs.val_ ) )   &&
         noexcept( err_   = std::move( rhs.err_ ) ) ) );
    /* clang-format on */
    if( !rhs.good_ ) {
      if( !good_ ) {
        err_ = std::move( rhs.err_ );
        return *this;
      }
      // this has a value.
      destroy();
      new_err( std::move( rhs.err_ ) );
      good_ = false;
      return *this;
    }
    // rhs has a value.
    if( !good_ ) {
      destroy();
      new_val( std::move( rhs.val_ ) );
      // Now set after no exception has happened.
      good_ = true;
      return *this;
    }
    // both have values.
    val_ = std::move( rhs.val_ );
    return *this;
  }

  /**************************************************************
  ** Converting Assignment
  ***************************************************************/
  template<typename U, typename V> /* clang-format off */
  constexpr expect<T, E>& operator=( expect<U, V> const& rhs ) &
      noexcept( std::is_nothrow_constructible_v<T, U const&> &&
                std::is_nothrow_assignable_v<T&, U const&>   &&
                std::is_nothrow_constructible_v<E, V const&> &&
                std::is_nothrow_assignable_v<E&, V const&> )
      requires( std::is_constructible_v<T, U const&>            &&
                std::is_assignable_v<T&, U const&>              &&
                std::is_constructible_v<E, V const&>            &&
                std::is_assignable_v<E&, V const&>              &&
               !std::is_constructible_v<T, expect<U,V>&>        &&
               !std::is_constructible_v<T, expect<U,V> const&>  &&
               !std::is_constructible_v<T, expect<U,V>&&>       &&
               !std::is_constructible_v<T, expect<U,V> const&&> &&
               !std::is_convertible_v<expect<U,V>&, T>          &&
               !std::is_convertible_v<expect<U,V> const&, T>    &&
               !std::is_convertible_v<expect<U,V>&&, T>         &&
               !std::is_convertible_v<expect<U,V> const&&, T>   &&
               !std::is_assignable_v<T&, expect<U,V>&>          &&
               !std::is_assignable_v<T&, expect<U,V> const&>    &&
               !std::is_assignable_v<T&, expect<U,V>&&>         &&
               !std::is_assignable_v<T&, expect<U,V> const&&>   &&
               !std::is_constructible_v<E, expect<U,V>&>        &&
               !std::is_constructible_v<E, expect<U,V> const&>  &&
               !std::is_constructible_v<E, expect<U,V>&&>       &&
               !std::is_constructible_v<E, expect<U,V> const&&> &&
               !std::is_convertible_v<expect<U,V>&, E>          &&
               !std::is_convertible_v<expect<U,V> const&, E>    &&
               !std::is_convertible_v<expect<U,V>&&, E>         &&
               !std::is_convertible_v<expect<U,V> const&&, E>   &&
               !std::is_assignable_v<E&, expect<U,V>&>          &&
               !std::is_assignable_v<E&, expect<U,V> const&>    &&
               !std::is_assignable_v<E&, expect<U,V>&&>         &&
               !std::is_assignable_v<E&, expect<U,V> const&&> ) {
    /* clang-format on */
    if( !rhs.good_ ) {
      if( !good_ ) {
        err_ = rhs.err_;
        return *this;
      }
      // this has a value.
      destroy();
      new_err( rhs.err_ );
      good_ = false;
      return *this;
    }
    // rhs has a value.
    if( !good_ ) {
      destroy();
      new_val( rhs.val_ );
      // Now set after no exception has happened.
      good_ = true;
      return *this;
    }
    // both have values.
    val_ = rhs.val_;
    return *this;
  }

  template<typename U, typename V> /* clang-format off */
  constexpr expect<T, E>& operator=( expect<U, V>&& rhs ) &
      noexcept( std::is_nothrow_constructible_v<T, U> &&
                std::is_nothrow_assignable_v<T&, U>   &&
                std::is_nothrow_constructible_v<E, V> &&
                std::is_nothrow_assignable_v<E&, V> )
      requires( std::is_constructible_v<T, U>                   &&
                std::is_assignable_v<T&, U>                     &&
                std::is_constructible_v<E, V>                   &&
                std::is_assignable_v<E&, V>                     &&
               !std::is_constructible_v<T, expect<U,V>&>        &&
               !std::is_constructible_v<T, expect<U,V> const&>  &&
               !std::is_constructible_v<T, expect<U,V>&&>       &&
               !std::is_constructible_v<T, expect<U,V> const&&> &&
               !std::is_convertible_v<expect<U,V>&, T>          &&
               !std::is_convertible_v<expect<U,V> const&, T>    &&
               !std::is_convertible_v<expect<U,V>&&, T>         &&
               !std::is_convertible_v<expect<U,V> const&&, T>   &&
               !std::is_assignable_v<T&, expect<U,V>&>          &&
               !std::is_assignable_v<T&, expect<U,V> const&>    &&
               !std::is_assignable_v<T&, expect<U,V>&&>         &&
               !std::is_assignable_v<T&, expect<U,V> const&&>   &&
               !std::is_constructible_v<E, expect<U,V>&>        &&
               !std::is_constructible_v<E, expect<U,V> const&>  &&
               !std::is_constructible_v<E, expect<U,V>&&>       &&
               !std::is_constructible_v<E, expect<U,V> const&&> &&
               !std::is_convertible_v<expect<U,V>&, E>          &&
               !std::is_convertible_v<expect<U,V> const&, E>    &&
               !std::is_convertible_v<expect<U,V>&&, E>         &&
               !std::is_convertible_v<expect<U,V> const&&, E>   &&
               !std::is_assignable_v<E&, expect<U,V>&>          &&
               !std::is_assignable_v<E&, expect<U,V> const&>    &&
               !std::is_assignable_v<E&, expect<U,V>&&>         &&
               !std::is_assignable_v<E&, expect<U,V> const&&> ) {
    /* clang-format on */
    if( !rhs.good_ ) {
      if( !good_ ) {
        err_ = std::move( rhs.err_ );
        return *this;
      }
      // this has a value.
      destroy();
      new_err( std::move( rhs.err_ ) );
      good_ = false;
      return *this;
    }
    // rhs has a value.
    if( !good_ ) {
      destroy();
      new_val( std::move( rhs.val_ ) );
      // Now set after no exception has happened.
      good_ = true;
      return *this;
    }
    // both have values.
    val_ = std::move( rhs.val_ );
    return *this;
  }

  /**************************************************************
  ** Value Assignment
  ***************************************************************/
  /* clang-format off */
  constexpr expect<T, E>& operator=( T const& val ) &
      noexcept( std::is_nothrow_copy_constructible_v<T> &&
                std::is_nothrow_copy_assignable_v<T> )
      requires( std::is_copy_constructible_v<T> &&
                std::is_copy_assignable_v<T> )
  /* clang-format on */ {
    if( !good_ ) {
      destroy();
      new_val( val );
      // Now set after no exception has happened.
      good_ = true;
      return *this;
    }
    val_ = val;
    return *this;
  }

  /* clang-format off */
  constexpr expect<T, E>& operator=( E const& err ) &
      noexcept( std::is_nothrow_copy_constructible_v<E> &&
                std::is_nothrow_copy_assignable_v<E> )
      requires( std::is_copy_constructible_v<E> &&
                std::is_copy_assignable_v<E> )
  /* clang-format on */ {
    if( good_ ) {
      destroy();
      new_err( err );
      good_ = false;
      return *this;
    }
    err_ = err;
    return *this;
  }

  /* clang-format off */
  constexpr expect<T, E>& operator=( T&& val ) &
      noexcept( std::is_nothrow_move_constructible_v<T> &&
                std::is_nothrow_move_assignable_v<T> )
      requires( std::is_move_constructible_v<T> &&
                std::is_move_assignable_v<T> )
  /* clang-format on */ {
    if( !good_ ) {
      destroy();
      new_val( std::move( val ) );
      // Now set after no exception has happened.
      good_ = true;
      return *this;
    }
    val_ = std::move( val );
    return *this;
  }

  /* clang-format off */
  constexpr expect<T, E>& operator=( E&& err ) &
      noexcept( std::is_nothrow_move_constructible_v<E> &&
                std::is_nothrow_move_assignable_v<E> )
      requires( std::is_move_constructible_v<E> &&
                std::is_move_assignable_v<E> )
  /* clang-format on */ {
    if( good_ ) {
      destroy();
      new_err( std::move( err ) );
      good_ = false;
      return *this;
    }
    err_ = std::move( err );
    return *this;
  }

  /**************************************************************
  ** Converting Value Assignment
  ***************************************************************/
  template<typename U> /* clang-format off */
  constexpr expect<T, E>& operator=( U&& val ) &
      noexcept( std::is_nothrow_constructible_v<T, U> &&
                std::is_nothrow_assignable_v<T&, U> )
      requires( std::is_constructible_v<T, U>          &&
                std::is_assignable_v<T&, U>            &&
               !std::is_constructible_v<E, U>          &&
               !std::is_assignable_v<E&, U>            &&
               !std::is_same_v<std::remove_cvref_t<T>,
                               std::remove_cvref_t<U>> &&
               !std::is_same_v<std::remove_cvref_t<E>,
                               std::remove_cvref_t<U>> )
  /* clang-format on */ {
    if( !good_ ) {
      destroy();
      new_val( std::forward<U>( val ) );
      // Now set after no exception has happened.
      good_ = true;
      return *this;
    }
    // both have values.
    val_ = std::forward<U>( val );
    return *this;
  }

  template<typename U> /* clang-format off */
  constexpr expect<T, E>& operator=( U&& err ) &
      noexcept( std::is_nothrow_constructible_v<E, U> &&
                std::is_nothrow_assignable_v<E&, U> )
      requires( std::is_constructible_v<E, U>          &&
                std::is_assignable_v<E&, U>            &&
               !std::is_constructible_v<T, U>          &&
               !std::is_assignable_v<T&, U>            &&
               !std::is_same_v<std::remove_cvref_t<T>,
                               std::remove_cvref_t<U>> &&
               !std::is_same_v<std::remove_cvref_t<E>,
                               std::remove_cvref_t<U>> )
  /* clang-format on */ {
    if( good_ ) {
      destroy();
      new_err( std::forward<U>( err ) );
      // Now set after no exception has happened.
      good_ = false;
      return *this;
    }
    // both have values.
    err_ = std::forward<U>( err );
    return *this;
  }

  /**************************************************************
  ** Swap
  ***************************************************************/
  constexpr void swap(
      expect<T, E>& other ) /* clang-format off */
    noexcept( std::is_nothrow_move_constructible_v<T> &&
              std::is_nothrow_swappable_v<T>          &&
              std::is_nothrow_move_constructible_v<E> &&
              std::is_nothrow_swappable_v<E> )
    requires( std::is_move_constructible_v<T> &&
              std::is_swappable_v<T>          &&
              std::is_move_constructible_v<E> &&
              std::is_swappable_v<E> ) { /* clang-format on */
    if( !good_ && !other.good_ ) {
      using std::swap;
      swap( err_, other.err_ );
      return;
    }
    if( good_ && other.good_ ) {
      using std::swap;
      swap( val_, other.val_ );
      return;
    }
    // One is good and the other is not.
    expect<T, E>& good     = good_ ? *this : other;
    expect<T, E>& not_good = good_ ? other : *this;
    not_good.new_val( std::move( good.val_ ) );
    good.new_err( std::move( not_good.err_ ) );
    good.destroy_val();
    not_good.destroy_err();
    // Now set after no exception has happened.
    not_good.good_ = true;
    good.good_     = false;
  }

  /**************************************************************
  ** value
  ***************************************************************/
  [[nodiscard]] constexpr T const& value(
      SourceLoc loc = SourceLoc::current() ) const& {
    if( !good_ ) throw bad_expect_access{ loc };
    return **this;
  }
  [[nodiscard]] constexpr T& value(
      SourceLoc loc = SourceLoc::current() ) & {
    if( !good_ ) throw bad_expect_access{ loc };
    return **this;
  }

  [[nodiscard]] constexpr T const&& value(
      SourceLoc loc = SourceLoc::current() ) const&& {
    if( !good_ ) throw bad_expect_access{ loc };
    return std::move( **this );
  }
  [[nodiscard]] constexpr T&& value(
      SourceLoc loc = SourceLoc::current() ) && {
    if( !good_ ) throw bad_expect_access{ loc };
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
  ** has_value/bool
  ***************************************************************/
  [[nodiscard]] constexpr bool has_value() const noexcept {
    return good_;
  }

  [[nodiscard]] constexpr bool has_error() const noexcept {
    return !good_;
  }

  [[nodiscard]] constexpr explicit operator bool() const noexcept
      // If we have a expect<T, E> where either T or E is a bool
      // then don't allow conversion to bool, that's probably not
      // a good thing. Instead one should call either the
      // has_value() member for just checking whether there is a
      // value, or call the is_value_truish() method to return
      // true iff there is a value and it's true.
      requires( !std::is_same_v<std::remove_cvref_t<T>, bool> &&
                !std::is_same_v<std::remove_cvref_t<E>, bool> ) {
    return good_;
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
    destroy();
    new_val( std::forward<Args>( args )... );
    good_ = true;
    return **this;
  }

  template<typename U, typename... Args> /* clang-format off */
  T& emplace( std::initializer_list<U> ilist, Args&&... args )
    noexcept( std::is_nothrow_constructible_v<T,
                  std::initializer_list<U>, Args...> )
    requires( std::is_constructible_v<T,
                  std::initializer_list<U>, Args...> ) {
                                         /* clang-format on */
    destroy();
    new_val( ilist, std::forward<Args>( args )... );
    good_ = true;
    return **this;
  }

  /**************************************************************
  ** Mapping to Bool
  ***************************************************************/
  // Returns true only if the expect is good and contains a value
  // that, when converted to bool, yields true.
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
  template<typename Alt> /* clang-format off */
  expect<maybe<Alt&>, E> get_if() noexcept
    requires(
        requires { std::get_if<Alt>(std::declval<T*>()); } ) {
    /* clang-format on */
    if( !has_value() ) return err_;
    auto* p = std::get_if<Alt>( &val_ );
    if( p == nullptr ) return nothing;
    return maybe<Alt&>( *p );
  }

  template<typename Alt> /* clang-format off */
  expect<maybe<Alt const&>, E> get_if() const noexcept
    requires(
        requires { std::get_if<Alt>(std::declval<T*>()); } ) {
    /* clang-format on */
    if( !has_value() ) return err_;
    auto* p = std::get_if<Alt>( &val_ );
    if( p == nullptr ) return nothing;
    return maybe<Alt const&>( *p );
  }

  /**************************************************************
  ** Monadic Interface: member
  ***************************************************************/
  template<typename Func>
  auto member( Func&& func ) const& /* clang-format off */
    -> expect<std::invoke_result_t<Func, T const&>, E>
    requires( std::is_invocable_v<Func, T const&> &&
              std::is_copy_constructible_v<E> &&
              std::is_member_object_pointer_v<Func> &&
             !is_expect_v<std::remove_cvref_t<
               std::invoke_result_t<Func,T const&>>> ) {
    /* clang-format on */
    using res_t =
        expect<std::invoke_result_t<Func, T const&>, E>;
    if( !good_ ) return err_;
    return std::invoke( std::forward<Func>( func ), **this );
  }

  template<typename Func>
  auto member( Func&& func ) & /* clang-format off */
    -> expect<std::invoke_result_t<Func, T&>, E>
    requires( std::is_invocable_v<Func, T&> &&
              std::is_copy_constructible_v<E> &&
              std::is_member_object_pointer_v<Func> &&
             !is_expect_v<std::remove_cvref_t<
               std::invoke_result_t<Func,T&>>> ) {
    /* clang-format on */
    using res_t = expect<std::invoke_result_t<Func, T&>, E>;
    if( !good_ ) return err_;
    return std::invoke( std::forward<Func>( func ), **this );
  }

  /**************************************************************
  ** Monadic Interface: maybe_member
  ***************************************************************/
  template<typename Func>
  auto maybe_member( Func&& func ) const& /* clang-format off */
    -> expect<maybe<typename std::remove_cvref_t<
                        std::invoke_result_t<Func, T const&>
                      >::value_type const&>, E>
    requires( std::is_member_object_pointer_v<Func> &&
              std::is_copy_constructible_v<E> &&
              is_maybe_v<std::remove_cvref_t<
                std::invoke_result_t<Func, T const&>>> ) {
    /* clang-format on */
    using res_t =
        expect<maybe<typename std::remove_reference_t<
                   std::invoke_result_t<Func, T const&>>::
                         value_type const&>,
               E>;
    if( !has_value() ) return err_;
    decltype( auto ) maybe_field =
        std::invoke( std::forward<Func>( func ), **this );
    if( !maybe_field ) return res_t::value_type( nothing );
    return res_t::value_type( std::move( *maybe_field ) );
  }

  template<typename Func>
  auto maybe_member( Func&& func ) & /* clang-format off */
    -> expect<maybe<typename std::remove_cvref_t<
                        std::invoke_result_t<Func, T&>
                      >::value_type&>, E>
    requires( std::is_member_object_pointer_v<Func> &&
              std::is_copy_constructible_v<E> &&
              is_maybe_v<std::remove_cvref_t<
                std::invoke_result_t<Func, T&>>> ) {
    /* clang-format on */
    using res_t =
        expect<maybe<typename std::remove_reference_t<
                   std::invoke_result_t<Func, T&>>::value_type&>,
               E>;
    if( !has_value() ) return err_;
    decltype( auto ) maybe_field =
        std::invoke( std::forward<Func>( func ), **this );
    if( !maybe_field ) return res_t::value_type( nothing );
    return res_t::value_type( std::move( *maybe_field ) );
  }

  /**************************************************************
  ** Monadic Interface: fmap
  ***************************************************************/
  template<typename Func>
  auto fmap( Func&& func ) const& /* clang-format off */
    -> expect<std::invoke_result_t<Func, T const&>, E>
    requires( std::is_invocable_v<Func, T const&> &&
              std::is_copy_constructible_v<E> &&
             !std::is_member_object_pointer_v<Func> &&
             !is_expect_v<std::remove_cvref_t<
               std::invoke_result_t<Func,T const&>>> ) {
    /* clang-format on */
    if( !good_ ) return err_;
    return std::invoke( std::forward<Func>( func ), **this );
  }

  template<typename Func>
  auto fmap( Func&& func ) & /* clang-format off */
    -> expect<std::invoke_result_t<Func, T&>, E>
    requires( std::is_invocable_v<Func, T&> &&
              std::is_copy_constructible_v<E> &&
             !std::is_member_object_pointer_v<Func> &&
             !is_expect_v<std::remove_cvref_t<
               std::invoke_result_t<Func, T&>>> ) {
    /* clang-format on */
    if( !good_ ) return err_;
    return std::invoke( std::forward<Func>( func ), **this );
  }

  template<typename Func> /* clang-format off */
  auto fmap( Func&& func ) &&
       -> expect<std::invoke_result_t<Func, T>, E>
       requires( std::is_invocable_v<Func, T> &&
                 std::is_copy_constructible_v<E> &&
                !std::is_member_object_pointer_v<Func> &&
                !is_expect_v<std::remove_cvref_t<
                  std::invoke_result_t<Func, T>>> ) {
    /* clang-format on */
    if( !good_ ) return err_;
    return std::invoke( std::forward<Func>( func ),
                        std::move( **this ) );
  }

  /**************************************************************
  ** Monadic Interface: bind
  ***************************************************************/
  template<typename Func>
  auto bind( Func&& func ) const& /* clang-format off */
    -> std::remove_cvref_t<std::invoke_result_t<Func, T const&>>
    requires( !std::is_member_object_pointer_v<Func> &&
               std::is_copy_constructible_v<E> &&
               std::is_same_v<typename std::remove_cvref_t<
                 std::invoke_result_t<Func,T const&>>::error_type,
                 E> &&
               is_expect_v<std::remove_cvref_t<
                 std::invoke_result_t<Func,T const&>>> ) {
    /* clang-format on */
    if( !good_ ) return err_;
    return std::invoke( std::forward<Func>( func ), **this );
  }

  template<typename Func> /* clang-format off */
  auto bind( Func&& func ) &&
    -> std::remove_cvref_t<std::invoke_result_t<Func, T>>
    requires( !std::is_member_object_pointer_v<Func> &&
               std::is_copy_constructible_v<E> &&
               std::is_same_v<typename std::remove_cvref_t<
                 std::invoke_result_t<Func, T>>::error_type,
                 E> &&
               is_expect_v<std::remove_cvref_t<
                 std::invoke_result_t<Func, T>>> ) {
    /* clang-format on */
    if( !good_ ) return err_;
    return std::invoke( std::forward<Func>( func ),
                        std::move( **this ) );
  }

  /**************************************************************
  ** Storage
  ***************************************************************/
private:
  // Allows expect<T> to access private members of expect<U>.
  template<typename U, typename V>
  requires ExpectTypeRequirements<U, V> friend class expect;

  // This is so that we can have a common API with the expect-ref
  // type for assigning, since we need to delete the expect-ref
  // type's assignment operators (we can't just make them private
  // otherwise the is_invocable type traits of the expect-ref
  // type get messed up.
  template<typename... Args>
  void assign( Args&&... args ) {
    (void)( this->operator=( std::forward<Args>( args )... ) );
  }

  template<typename... Vs> /* clang-format off */
  constexpr void new_val( Vs&&... v )
    noexcept(
      (std::is_trivially_constructible_v<
            T, decltype( std::forward<Vs>( v ) )...> &&
       std::is_trivially_move_assignable_v<T>) ||
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

  template<typename... Vs> /* clang-format off */
  constexpr void new_err( Vs&&... v )
    noexcept(
      (std::is_trivially_constructible_v<
            E, decltype( std::forward<Vs>( v ) )...> &&
       std::is_trivially_move_assignable_v<E>) ||
      noexcept( new( &this->err_ ) E( std::forward<Vs>( v )... ) )
    ) { /* clang-format on */
    if constexpr( std::is_trivially_constructible_v<
                      E, decltype( std::forward<Vs>( v ) )...> &&
                  std::is_trivially_move_assignable_v<E> ) {
      err_ = E( std::forward<Vs>( v )... );
    } else {
      new( &err_ ) E( std::forward<Vs>( v )... );
    }
  }

  bool good_ = false;
  union {
    T val_;
    E err_;
  };
};

/****************************************************************
** expect-of-reference
*****************************************************************/
template<typename T, typename E>
requires ExpectTypeRequirements<T, E> /* clang-format off */
class [[nodiscard]] expect<T&, E> { /* clang-format on */
public:
  /**************************************************************
  ** Types
  ***************************************************************/
  using value_type = T&;
  using error_type = E;

  /**************************************************************
  ** Default Constructor
  ***************************************************************/
  expect() = delete;

  /**************************************************************
  ** Destruction
  ***************************************************************/
private:
  constexpr void destroy_if_err() {
    if( !p_ ) {
      if constexpr( !std::is_trivially_destructible_v<E> )
        err_.E::~E();
    }
  }

public:
  constexpr ~expect() noexcept { destroy_if_err(); }

  /**************************************************************
  ** Value Constructors
  ***************************************************************/
  constexpr expect( T& ref ) noexcept : p_( &ref ) {}

  // We don't support holding rvalue refs in expect<>, so there
  // isn't much point it having this.
  expect( T&& ) = delete;

  constexpr expect( E const& err ) /* clang-format off */
      noexcept( std::is_nothrow_copy_constructible_v<E> )
      requires( std::is_copy_constructible_v<E> )
    : p_{ nullptr } /* clang-format on */ {
    new_err( err );
  }

  constexpr expect( E&& err ) /* clang-format off */
      noexcept( std::is_nothrow_move_constructible_v<E> )
      requires( std::is_move_constructible_v<E> )
    : p_{ false } /* clang-format on */ {
    new_err( std::move( err ) );
  }

  /**************************************************************
  ** Converting Value Constructor
  ***************************************************************/
  template<typename U = T> /* clang-format off */
  constexpr expect( U& ref ) noexcept
    requires(
       std::is_convertible_v<U&, T&> &&
      !std::is_same_v<std::remove_cvref_t<U>, E> &&
      !std::is_same_v<std::remove_cvref_t<U>, std::in_place_t> &&
      !is_expect_v<std::remove_cvref_t<U>> ) {
    /* clang-format on */
    p_ = static_cast<T*>( &ref );
  }

  /**************************************************************
  ** Copy Constructor
  ***************************************************************/
  /* clang-format off */
  constexpr expect( expect<T&, E> const& other )
      noexcept( noexcept( this->new_err( other.err_ ) ) )
      requires( std::is_copy_constructible_v<E> )
    : p_{ nullptr } /* clang-format on */ {
    if( other.has_value() )
      p_ = other.p_;
    else
      new_err( other.err_ );
  }

  /**************************************************************
  ** Converting Copy Constructors
  ***************************************************************/
  template<typename U, typename V> /* clang-format off */
  explicit( !std::is_convertible_v<V const&, E> )
  constexpr expect( expect<U&, V> const& other )
      noexcept( noexcept( this->new_err( other.err_ ) ) )
      requires( std::is_copy_constructible_v<E>                  &&
                std::is_convertible_v<U&, T&>                    &&
                std::is_constructible_v<E, const V&>             &&
               !std::is_convertible_v<expect<U&,V>&, T&>         &&
               !std::is_convertible_v<expect<U&,V> const&, T&>   &&
               !std::is_convertible_v<expect<U&,V>&&, T&>        &&
               !std::is_convertible_v<expect<U&,V> const&&, T&>  &&
               !std::is_constructible_v<E, expect<U&,V>&>        &&
               !std::is_constructible_v<E, expect<U&,V> const&>  &&
               !std::is_constructible_v<E, expect<U&,V>&&>       &&
               !std::is_constructible_v<E, expect<U&,V> const&&> &&
               !std::is_convertible_v<expect<U&,V>&, E>          &&
               !std::is_convertible_v<expect<U&,V> const&, E>    &&
               !std::is_convertible_v<expect<U&,V>&&, E>         &&
               !std::is_convertible_v<expect<U&,V> const&&, E> )
    : p_{ nullptr } /* clang-format on */ {
    if( other.has_value() )
      p_ = other.p_;
    else
      new_err( other.err_ );
  }

  /**************************************************************
  ** Move Constructors
  ***************************************************************/
  constexpr expect(
      expect<T&, E>&& other ) /* clang-format off */
    noexcept( noexcept( this->new_err( std::move( other.err_ ) ) ) )
    requires( std::is_move_constructible_v<E> )
    : p_{ nullptr } /* clang-format on */ {
    if( other.has_value() )
      p_ = other.p_;
    else
      new_err( std::move( other.err_ ) );
    static_assert(
        std::is_nothrow_move_constructible_v<E> ==
        noexcept( this->new_err( std::move( other.err_ ) ) ) );
  }

  /**************************************************************
  ** Converting Move Constructors
  ***************************************************************/
  template<typename U, typename V> /* clang-format off */
  explicit( !std::is_convertible_v<V&&, E> )
  constexpr expect( expect<U&, V>&& other )
      noexcept(
          noexcept( this->new_val( std::move( other.err_ ) ) ) )
      requires( std::is_convertible_v<T&, U&>                    &&
                std::is_constructible_v<E, V&&>                  &&
               !std::is_convertible_v<expect<U&,V>&, T&>         &&
               !std::is_convertible_v<expect<U&,V> const&, T&>   &&
               !std::is_convertible_v<expect<U&,V>&&, T&>        &&
               !std::is_convertible_v<expect<U&,V> const&&, T&>  &&
               !std::is_constructible_v<E, expect<U&,V>&>        &&
               !std::is_constructible_v<E, expect<U&,V> const&>  &&
               !std::is_constructible_v<E, expect<U&,V>&&>       &&
               !std::is_constructible_v<E, expect<U&,V> const&&> &&
               !std::is_convertible_v<expect<U&,V>&, E>          &&
               !std::is_convertible_v<expect<U&,V> const&, E>    &&
               !std::is_convertible_v<expect<U&,V>&&, E>         &&
               !std::is_convertible_v<expect<U&,V> const&&, E> )
    : p_{ nullptr } /* clang-format on */ {
    if( other.has_value() )
      p_ = other.p_;
    else
      new_err( std::move( other.err_ ) );
  }

  /**************************************************************
  ** Assignment Operators.
  ***************************************************************/
  expect<T&, E> operator=( expect<T&, E> const& ) = delete;
  expect<T&, E> operator=( expect<T&, E>&& ) = delete;

  /**************************************************************
  ** value
  ***************************************************************/
  [[nodiscard]] constexpr T& value(
      SourceLoc loc = SourceLoc::current() ) const {
    if( !has_value() ) throw bad_expect_access{ loc };
    return **this;
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

  /**************************************************************
  ** has_value/bool
  ***************************************************************/
  [[nodiscard]] constexpr bool has_value() const noexcept {
    return p_ != nullptr;
  }

  [[nodiscard]] constexpr explicit operator bool() const noexcept
      // If we have a expect<T, E> where either T or E is a bool
      // then don't allow conversion to bool, that's probably not
      // a good thing. Instead one should call either the
      // has_value() member for just checking whether there is a
      // value, or call the is_value_truish() method to return
      // true iff there is a value and it's true.
      requires( !std::is_same_v<std::remove_cvref_t<T>, bool> &&
                !std::is_same_v<std::remove_cvref_t<E>, bool> ) {
    return p_ != nullptr;
    ;
  }

  /**************************************************************
  ** Deference operators
  ***************************************************************/
  constexpr T& operator*() const noexcept { return *p_; }

  constexpr std::remove_reference_t<T>* operator->()
      const noexcept {
    return &**this;
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
  auto get_if() const noexcept
    -> expect<maybe<mp::const_if_t<Alt, std::is_const_v<T>>&>, E>
    requires(
        requires { std::get_if<Alt>(std::declval<T*>()); } ) {
    /* clang-format on */
    if( !has_value() ) return err_;
    auto* ptr = std::get_if<Alt>( p_ );
    if( ptr == nullptr ) return nothing;
    return maybe<mp::const_if_t<Alt, std::is_const_v<T>>&>(
        *ptr );
  }

  /**************************************************************
  ** Monadic Interface: member
  ***************************************************************/
  template<typename Func>
  auto member( Func&& func ) const& /* clang-format off */
    -> expect<std::invoke_result_t<Func, T&>, E>
    requires( std::is_invocable_v<Func, T&> &&
              std::is_copy_constructible_v<E> &&
              std::is_member_object_pointer_v<Func> &&
             !is_expect_v<std::remove_cvref_t<
               std::invoke_result_t<Func,T&>>> ) {
    /* clang-format on */
    if( !has_value() ) return err_;
    return std::invoke( std::forward<Func>( func ), **this );
  }

  /**************************************************************
  ** Monadic Interface: maybe_member
  ***************************************************************/
  template<typename Func>
  auto maybe_member( Func&& func ) const /* clang-format off */
    -> expect<maybe<typename mp::const_if_t<
                        typename std::remove_cvref_t<
                          std::invoke_result_t<Func, T&>
                        >::value_type,
                        std::is_const_v<T>
                      >&>,
                      E
                    >
    requires( std::is_member_object_pointer_v<Func> &&
              std::is_copy_constructible_v<E> &&
              is_maybe_v<std::remove_cvref_t<
                std::invoke_result_t<Func,T&>>> ) {
    using res_t = expect<maybe<typename mp::const_if_t<
                                  typename std::remove_cvref_t<
                                    std::invoke_result_t<Func, T&>
                                  >::value_type,
                                  std::is_const_v<T>
                                >&>,
                                E
                              >;
    /* clang-format on */
    if( !has_value() ) return err_;
    decltype( auto ) maybe_field =
        std::invoke( std::forward<Func>( func ), **this );
    if( !maybe_field ) return res_t::value_type( nothing );
    return res_t::value_type( std::move( *maybe_field ) );
  }

  /**************************************************************
  ** Monadic Interface: fmap
  ***************************************************************/
  template<typename Func>
  auto fmap( Func&& func ) const /* clang-format off */
    -> expect<std::invoke_result_t<Func, T&>, E>
    requires( std::is_invocable_v<Func, T&> &&
              std::is_copy_constructible_v<E> &&
             !std::is_member_object_pointer_v<Func> &&
             !is_expect_v<std::remove_cvref_t<
                std::invoke_result_t<Func,T&>>> ) {
    /* clang-format on */
    if( !has_value() ) return err_;
    return std::invoke( std::forward<Func>( func ), **this );
  }

  /**************************************************************
  ** Monadic Interface: bind
  ***************************************************************/
  template<typename Func>
  auto bind( Func&& func ) const /* clang-format off */
    -> std::remove_cvref_t<std::invoke_result_t<Func, T&>>
    requires( !std::is_member_object_pointer_v<Func> &&
               std::is_copy_constructible_v<E> &&
               std::is_same_v<typename std::remove_cvref_t<
                 std::invoke_result_t<Func,T&>>::error_type,
                 E> &&
              is_expect_v<std::remove_cvref_t<
                std::invoke_result_t<Func,T&>>> ) {
    /* clang-format on */
    if( !has_value() ) return err_;
    return std::invoke( std::forward<Func>( func ), **this );
  }

private:
  // This allows expect<T, E> to access private members of
  // expect<U, V>.
  template<typename U, typename V>
  requires ExpectTypeRequirements<U, V> friend class expect;

  /**************************************************************
  ** Construction of new error.
  ***************************************************************/
  template<typename... Vs> /* clang-format off */
  constexpr void new_err( Vs&&... v )
    noexcept(
      (std::is_trivially_constructible_v<
            T, decltype( std::forward<Vs>( v ) )...> &&
       std::is_trivially_move_assignable_v<T>) ||
      noexcept( new( &this->err_ ) T( std::forward<Vs>( v )... ) )
    ) { /* clang-format on */
    if constexpr( std::is_trivially_constructible_v<
                      T, decltype( std::forward<Vs>( v ) )...> &&
                  std::is_trivially_move_assignable_v<T> ) {
      err_ = T( std::forward<Vs>( v )... );
    } else {
      new( &err_ ) T( std::forward<Vs>( v )... );
    }
  }

  /**************************************************************
  ** Storage
  ***************************************************************/
  T* p_ = nullptr;
  union {
    E err_;
  };
};

} // namespace base