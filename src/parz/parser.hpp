/****************************************************************
**parser.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: Monadic parser type.
*
*****************************************************************/
#pragma once

// parz
#include "error.hpp"

// base
#include "base/co-compat.hpp"
#include "base/error.hpp"
#include "base/fmt.hpp"
#include "base/unique-coro.hpp"

// C++ standard library
#include <string>
#include <string_view>

/****************************************************************
** Parser
*****************************************************************/
namespace parz {

struct error;

template<typename T>
struct promise_type;

template<typename T = std::monostate>
struct parser {
  using value_type  = T;
  using result_type = result<T>;

  parser( promise_type<T>* p )
    : promise_( p ),
      h_( coro::coroutine_handle<promise_type<T>>::from_promise(
          *promise_ ) ) {}

  void resume( std::string_view buffer ) {
    promise_->in_ = buffer;
    h_.resource().resume();
  }

  bool finished() const { return promise_->o_.has_value(); }

  bool is_good() const {
    return promise_->o_.has_value() && promise_->o_->has_value();
  }

  bool is_error() const {
    return promise_->o_.has_value() && promise_->o_->has_error();
  }

  std::string_view buffer() const { return promise_->buffer(); }

  result_type& result() {
    DCHECK( finished() );
    return *promise_->o_;
  }

  result_type const& result() const {
    DCHECK( finished() );
    return *promise_->o_;
  }

  int consumed() const { return promise_->consumed_; }

  int farthest() const { return promise_->farthest_; }

  T& get() {
    DCHECK( is_good() );
    return **promise_->o_;
  }

  T const& get() const {
    DCHECK( is_good() );
    return **promise_->o_;
  }

  parz::error& error() {
    DCHECK( is_error() );
    return promise_->o_->error();
  }

  parz::error const& error() const {
    DCHECK( is_error() );
    return promise_->o_->error();
  }

  // This would be try anyway because of the unique_coro.
  parser( parser const& ) = delete;
  parser& operator=( parser const& ) = delete;

  parser( parser&& ) = default;
  parser& operator=( parser&& ) = default;

private:
  promise_type<T>*  promise_;
  base::unique_coro h_;
};

} // namespace parz

DEFINE_FORMAT_T( ( T ), (parz::parser<T>), "{}",
                 ( o.is_good() ? fmt::format( "{}", o.get() )
                   : o.is_error()
                       ? fmt::format( "{}", o.error() )
                       : std::string( "unfinished" ) ) );

namespace CORO_NS {

template<typename T, typename... Args>
struct coroutine_traits<::parz::parser<T>, Args...> {
  using promise_type = ::parz::promise_type<T>;
};

} // namespace CORO_NS