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
#include "base/valid.hpp"

// C++ standard library
#include <unordered_set>

namespace cdr {

/****************************************************************
** converter
*****************************************************************/
// This is a helper object that is used to do Cdr conversions to
// and from C++ types. It is created once at the start of a con-
// version operation and it is threaded down through all of the
// nested fields, allowing it to gather information as it goes,
// which is useful for error checking.
//
// Instead of calling to_canonical or from_canonical directly,
// you must use a converter object (even inside the implementa-
// tions of to_canonical and from_canonical extensions) because
// it does two things for us: 1) It is used to keep track of the
// backtrace when propagating errors generated from
// from_canonical, and 2) it holds conversion options used to
// customize the behaviors of the conversions.
//
// Note that when initiating a conversion at the top level you
// should use these functions:
//
//   run_conversion_from_canonical
//   run_conversion_to_canonical
//
// further below instead of manually creating a converter. Fur-
// thermore, when implementing from_canonical and to_canonical,
// you should avoid using the bare `from` and `to` by default;
// instead you should prefer the specialized ones, such as e.g.
// from_field or to_field and only fall back to the `from` and
// `to` when you have a reason (usually this will only apply to
// "special" types such as containers that aren't simple lists or
// records).
//
struct converter {
  struct options {
    // Options for to_canonical.
    bool write_fields_with_default_value = true;

    // Options for from_canonical.
    bool allow_unrecognized_fields        = false;
    bool default_construct_missing_fields = false;
  };

  converter() = default;

  converter( options const& opts ) : options_( opts ) {}

  template<typename... Args>
  error err( std::string_view fmt_str, Args&&... args ) & {
    frames_on_error_ = frames_;
    return error( fmt_str, std::forward<Args>( args )... );
  }

  template<FromCanonical T>
  result<std::remove_const_t<T>> from_index( list const& lst,
                                             int         idx ) {
    auto _ = frame( "index {}", idx );
    return from<T>( lst[idx] );
  }

  template<FromCanonical T>
  result<std::remove_const_t<T>> from_field(
      table const& tbl, std::string const& key ) {
    auto _ = frame( "value for key '{}'", key );
    used_keys_.insert( key );
    base::maybe<value const&> val = tbl[key];
    if( !val.has_value() ) {
      static_assert( std::is_default_constructible_v<
                     std::remove_const_t<T>> );
      if( options_.default_construct_missing_fields )
        return T{};
      else
        return error( "key '{}' not found in table.", key );
    }
    return from<T>( *val );
  }

  // This one should only really be called by a top-level conver-
  // sion that is initiating the entire operation (which includes
  // unit tests, which will have to call this). Otherwise, you
  // should pick one of the more specific ones above that are
  // adapted to the particular type of data type (table or list)
  // that you are going into.
  template<FromCanonical T>
  result<std::remove_const_t<T>> from( value const& v ) {
    auto _ = frame( base::demangled_typename<T>() );
    // The function called below should be found via ADL.
    auto res =
        from_canonical( *this, v, tag<std::remove_const_t<T>> );
    if( res.has_value() ) frames_on_error_.clear();
    return res;
  }

  // Ensure that the type T is in the `value` variant.
  template<typename T>
  result<T const&> ensure_type( value const& v ) {
    if( !v.holds<T>() )
      return err( "expected type {}, instead found type {}.",
                  type_name( value{ T{} } ), type_name( v ) );
    return v.get<T>();
  }

  // Ensure that the given list has the given size.
  base::valid_or<error> ensure_list_size( list const& lst,
                                          int expected_size );

  // Call this just before you start calling from_field on the
  // fields of a record object. Don't forget to call
  // end_field_tracking when finished.
  void start_field_tracking();

  // Call this and check the result when finished calling
  // from_field on the fields of a record object.
  base::valid_or<error> end_field_tracking( table const& tbl );

  template<ToCanonical T>
  void to_field( table& tbl, std::string const& key,
                 T const& o ) {
    if( !options_.write_fields_with_default_value && o == T{} )
      return;
    tbl[key] = to( o );
  }

  // Prefer to call to_field when dealing with named fields of
  // records.
  template<ToCanonical T>
  value to( T const& o ) {
    (void)options_;
    // The function called below should be found via ADL.
    return to_canonical( *this, o, tag<std::remove_const_t<T>> );
  }

  // If there was an error then this will have the frames.
  std::vector<std::string> const& error_stack() const;

  // If the converter contains an error then this will format the
  // back trace into something readable. If the converter does
  // not contain an error, it will return something unspecified
  // (don't call it in that case).
  std::string dump_error_stack() const;

  // Takes an error object and returns a new one containing not
  // only the original error, but also a formatted backtrace.
  error from_canonical_readable_error( error const& err ) const;

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

  options options_ = {};

  // Backtrace frames that are accumulated during the conversion
  // process.
  std::vector<std::string> frames_ = {};

  // These are the frames as they were on the most recent call to
  // generate an error. This gets reset when a call to
  // from_canonical is made that succeeds.
  std::vector<std::string> frames_on_error_ = {};

  // This is used in "field tracking mode"; in this mode, one
  // first calls start_field_tracking, then calls `from_field`
  // repeatedly, and each field requested will be recorded here.
  // Then one calls end_field_tracking, which checks whether
  // there are any extra unused keys in the table and returns an
  // error (if that is enabled in the options).
  std::unordered_set<std::string> used_keys_ = {};
};

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
  return conv.from_canonical_readable_error( res.error() );
}

template<typename T>
value run_conversion_to_canonical(
    T const& o, converter::options opts = {} ) {
  converter conv( std::move( opts ) );
  return conv.to( o );
}

/****************************************************************
** Testing Helpers.
*****************************************************************/
namespace testing {

// This will add the backtrace into the error message upon fail-
// ure, so that the unit testing framework will automatically
// show it if a test fails.
template<FromCanonical T>
result<T> conv_from_bt( value const& v ) {
  static converter conv;
  auto             res = conv.from<T>( v );
  if( !res.has_value() )
    return conv.from_canonical_readable_error( res.error() );
  return res;
};

} // namespace testing

} // namespace cdr
