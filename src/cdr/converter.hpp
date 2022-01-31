/****************************************************************
**converter.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-29.
*
* Description: Helper for calling {to,from}_canonical.
*
*****************************************************************/
#pragma once

// cdr
#include "error.hpp"
#include "ext.hpp"

// base
#include "base/cc-specific.hpp"

namespace cdr {

/****************************************************************
** converter
*****************************************************************/
// This is a helper object that is used to do Cdr conversions to
// and from C++ types. Instead of calling to_canonical or
// from_canonical directly, you must use a converter object (even
// inside the implementations of to_canonical and from_canonical
// extensions) because it does two things for us: 1) It is used
// to keep track of the back trace when propagating errors gener-
// ated from from_canonical, and 2) it holds conversion options
// used to customize the behaviors of the conversions.
//
// Note that when initiating a conversion at the top level you
// should use the functions:
//
//   run_conversion_from_canonical
//   run_conversion_to_canonical
//
// Further below instead of manually creating a converter.
//
struct converter {
  struct options {
    // TODO: add options here for how to deal with:
    //
    //   1. missing fields.
    //   2. extra unrecognized fields.
    //   3. fields that hold their default value.
  };

  converter() = default;

  converter( options const& opts ) : options_( opts ) {}

  template<typename... Args>
  error err( std::string_view fmt_str, Args&&... args ) & {
    frames_on_error_ = frames_;
    return error( fmt_str, std::forward<Args>( args )... );
  }

  template<FromCanonical T>
  result<std::remove_const_t<T>> from( value const& v ) {
    // The function called below should be found via ADL.
    auto res =
        from_canonical( *this, v, tag<std::remove_const_t<T>> );
    if( res.has_value() ) frames_on_error_.clear();
    return res;
  }

  template<ToCanonical T>
  value to( T const& o ) {
    (void)options_;
    // The function called below should be found via ADL.
    return to_canonical( *this, o, tag<std::remove_const_t<T>> );
  }

  // If there was an error then this will have the frames.
  std::vector<std::string> const& error_stack() const;

 private:
  struct scoped_frame {
    explicit scoped_frame( converter* owner, std::string name )
      : owner_( owner ) {
      owner_->frames_.push_back( std::move( name ) );
    }

    ~scoped_frame() noexcept { owner_->frames_.pop_back(); }

   private:
    scoped_frame( scoped_frame const& ) = delete;
    scoped_frame( scoped_frame&& )      = delete;

    converter* owner_;
  };

 public:
  scoped_frame frame( std::string name ) {
    return scoped_frame( this, std::move( name ) );
  }

  template<typename Arg1, typename... Args>
  scoped_frame frame( std::string_view fmt_str, Arg1&& arg1,
                      Args&&... args ) & {
    return frame( fmt::format( fmt::runtime( fmt_str ),
                               std::forward<Arg1>( arg1 ),
                               std::forward<Args>( args )... ) );
  }

  template<typename T>
  scoped_frame frame( tag_t<T> ) {
    return frame( base::demangled_typename<T>() );
  }

 private:
  options                  options_ = {};
  std::vector<std::string> frames_  = {};
  // These are the frames as they were on the most recent call to
  // generate an error. This gets reset when a call to
  // from_canonical is made that succeeds.
  std::vector<std::string> frames_on_error_ = {};
};

// If the converter contains an error then this will format the
// back trace into something readable. If the converter does not
// contain an error, it will return something unspecified (don't
// call it in that case).
std::string dump_error_stack(
    std::vector<std::string> const& frames );

/****************************************************************
** Conversion Orchestration.
*****************************************************************/
// This is only to be called at the very top level of a conver-
// sion. If there is an error, it will get the backtrace, format
// it into a string, then put it into a new error object.
template<typename T>
result<T> run_conversion_from_canonical(
    value const& v, converter::options opts = {} ) {
  converter conv( std::move( opts ) );
  result<T> res = conv.from<T>( v );
  if( res.has_value() ) return res;
  // We have an error.
  std::string err =
      fmt::format( "message: {}\n", res.error().what() ) +
      dump_error_stack( conv.error_stack() );
  return conv.err( std::move( err ) );
}

template<typename T>
value run_conversion_to_canonical(
    T const& o, converter::options opts = {} ) {
  converter conv( std::move( opts ) );
  return conv.to( o );
}

} // namespace cdr
