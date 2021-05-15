/****************************************************************
**variant.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-02.
*
* Description: A variant that inherits from std::variant but adds
*              some additional methods.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "maybe.hpp"

// C++ standard library
#include <variant>

namespace base {

/****************************************************************
** Conversion to Enum
*****************************************************************/
template<typename V>
struct variant_to_enum;

template<typename V>
using variant_to_enum_t = typename variant_to_enum<V>::type;

/****************************************************************
** better variant.
*****************************************************************/
template<typename... Args>
class variant : public std::variant<Args...> {
  using Base = std::variant<Args...>;

public:
  /**************************************************************
  ** Referring to base class.
  ***************************************************************/
  using base_t = Base;

  Base const& as_std() const& { return *this; }
  Base&       as_std() & { return *this; }

  Base const&& as_std() const&& { return *this; }
  Base&&       as_std() && { return *this; }

  /**************************************************************
  ** Take everything from std::variant.
  ***************************************************************/
  using Base::Base;

  using Base::operator=;

  using Base::index;

  using Base::valueless_by_exception;

  using Base::emplace;

  using Base::swap;

  /**************************************************************
  ** get (no checks!)
  ***************************************************************/
  template<typename T>
  T const& get() const noexcept {
    auto* p = std::get_if<T>( this );
    assert( p );
    return *p;
  }

  template<typename T>
  T& get() noexcept {
    auto* p = std::get_if<T>( this );
    assert( p );
    return *p;
  }

  /**************************************************************
  ** get_if
  ***************************************************************/
  template<typename T>
  maybe<T const&> get_if() const noexcept {
    auto* p = std::get_if<T>( this );
    return p ? maybe<T const&>( *p ) : nothing;
  }

  template<typename T>
  maybe<T&> get_if() noexcept {
    auto* p = std::get_if<T>( this );
    return p ? maybe<T&>( *p ) : nothing;
  }

  /**************************************************************
  ** holds
  ***************************************************************/
  template<typename T>
  bool holds() const noexcept {
    auto* p = std::get_if<T>( &this->as_std() );
    return p != nullptr;
  }

  /**************************************************************
  ** enum conversion
  ***************************************************************/
  auto to_enum() const noexcept {
    using Enum = variant_to_enum_t<variant>;
    return static_cast<Enum>( index() );
  }
};

/****************************************************************
** metaprogramming facilities.
*****************************************************************/
template<typename...>
struct is_base_variant : std::false_type {};

template<typename... Args>
struct is_base_variant<variant<Args...>> : std::true_type {};

} // namespace base

namespace std {

/****************************************************************
** variant_size specialization.
*****************************************************************/
template<typename... Args>
struct variant_size<::base::variant<Args...>>
  : public variant_size<std::variant<Args...>> {};

/****************************************************************
** variant_alternative specialization.
*****************************************************************/
template<std::size_t I, typename... Types>
struct variant_alternative<I, ::base::variant<Types...>>
  : public variant_alternative<I, variant<Types...>> {};

/****************************************************************
** std::visit specializations.
*****************************************************************/
template<typename Visitor, typename... Variants>
/* clang-format off */
  requires( std::conjunction_v<::base::is_base_variant<
              std::remove_cvref_t<Variants>>...> ) //
decltype( auto ) visit( Visitor&& visitor, Variants&&... variants ) {
  /* clang-format on */
  return ::std::visit(
      std::forward<Visitor>( visitor ),
      std::forward<
          decltype( std::declval<Variants>().as_std() )>(
          variants.as_std() )... );
}

template<typename T, typename Visitor, typename... Variants>
/* clang-format off */
  requires( std::conjunction_v<::base::is_base_variant<
              std::remove_cvref_t<Variants>>...> ) //
T visit( Visitor&& visitor, Variants&&... variants ) {
  /* clang-format on */
  return ::std::visit<T>(
      std::forward<Visitor>( visitor ),
      std::forward<
          decltype( std::declval<Variants>().as_std() )>(
          variants.as_std() )... );
}

} // namespace std
