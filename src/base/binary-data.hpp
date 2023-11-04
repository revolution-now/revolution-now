/****************************************************************
**binary-data.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-02.
*
* Description: For ease of working with binary buffers.
*
*****************************************************************/
#pragma once

// base
#include "error.hpp"
#include "expect.hpp"
#include "maybe.hpp"
#include "valid.hpp"

// C++ standard library.
#include <array>
#include <concepts>
#include <span>
#include <type_traits>
#include <vector>

namespace base {

/****************************************************************
** BinaryBuffer.
*****************************************************************/
struct BinaryBuffer {
  BinaryBuffer( std::vector<unsigned char>&& buffer )
    : buffer_( std::move( buffer ) ) {}

  // The bytes will be initialized to zero.
  BinaryBuffer( int size )
    : BinaryBuffer( std::vector<unsigned char>( size ) ) {}

  static base::expect<BinaryBuffer, std::string> from_file(
      std::string const& path );

  operator std::span<unsigned char>() { return buffer_; }

  valid_or<std::string> write_file( std::string const& path,
                                    int                n );

  valid_or<std::string> write_file( std::string const& path ) {
    return write_file( path, buffer_.size() );
  }

  int size() const { return buffer_.size(); }

  auto const* data() const { return buffer_.data(); }

  bool operator==( BinaryBuffer const& ) const = default;

 private:
  std::vector<unsigned char> buffer_;
};

/****************************************************************
** BinaryData.
*****************************************************************/
// Reads or writes to a binary buffer.
struct BinaryData {
  BinaryData( std::span<unsigned char> buffer )
    : buffer_( buffer ) {}

  bool eof() const;

  bool good( int nbytes ) const;

  int pos() const { return idx_; }

  int size() const { return buffer_.size(); }

  template<typename..., typename T>
  requires std::is_integral_v<T>
  bool read( T& out ) {
    static auto constexpr nbytes = sizeof( T );
    return read_bytes(
        nbytes, reinterpret_cast<unsigned char*>( &out ) );
  }

  template<typename..., typename T>
  requires std::is_integral_v<T>
  bool write( T const& in ) {
    static auto constexpr nbytes = sizeof( T );
    return write_bytes(
        nbytes, reinterpret_cast<unsigned char const*>( &in ) );
  }

  template<size_t N, typename..., typename T>
  requires std::is_integral_v<T> && ( N <= sizeof( T ) )
  bool read_bytes( T& out ) {
    out = 0;
    return read_bytes(
        N, reinterpret_cast<unsigned char*>( &out ) );
  }

  template<size_t N, typename..., typename T>
  requires std::is_integral_v<T> && ( N <= sizeof( T ) )
  bool write_bytes( T const& in ) {
    return write_bytes(
        N, reinterpret_cast<unsigned char const*>( &in ) );
  }

 private:
  bool read_bytes( int n, unsigned char* dst );
  bool write_bytes( int n, unsigned char const* src );

  std::span<unsigned char> buffer_;
  int                      idx_ = 0;
};

/****************************************************************
** Concepts.
*****************************************************************/
template<typename T>
concept ToBinary = requires( BinaryData& b, T const& o ) {
  { write_binary( b, o ) } -> std::same_as<bool>;
};

template<typename T>
concept FromBinary = requires( BinaryData& b, T& o ) {
  { read_binary( b, o ) } -> std::same_as<bool>;
};

template<typename T>
concept Binable = ToBinary<T> && FromBinary<T>;

/****************************************************************
** Instances (builtins).
*****************************************************************/
template<typename T>
requires std::is_integral_v<T>
bool read_binary( base::BinaryData& b, T& o ) {
  return b.read( o );
}

template<typename T>
requires std::is_integral_v<T>
bool write_binary( base::BinaryData& b, T const& o ) {
  return b.write( o );
}

template<typename T>
requires std::is_enum_v<T> &&
         FromBinary<std::underlying_type_t<T>>
bool read_binary( base::BinaryData& b, T& o ) {
  using U = std::underlying_type_t<T>;
  U res;
  if( !read_binary( b, res ) ) return false;
  o = static_cast<T>( res );
  return true;
}

template<typename T>
requires std::is_enum_v<T> && ToBinary<std::underlying_type_t<T>>
bool write_binary( base::BinaryData& b, T const& o ) {
  using U = std::underlying_type_t<T>;
  // Need to convert to U by value because a U const& can't bind
  // to a T const&, which is strange, since U const& is the un-
  // derlying type.
  return write_binary( b, static_cast<U>( o ) );
}

/****************************************************************
** Instances (std).
*****************************************************************/
template<FromBinary T, size_t N>
bool read_binary( base::BinaryData& b, std::array<T, N>& o ) {
  for( T& elem : o )
    if( !read_binary( b, elem ) ) //
      return false;
  return true;
}

template<ToBinary T, size_t N>
bool write_binary( base::BinaryData&       b,
                   std::array<T, N> const& o ) {
  for( T const& elem : o )
    if( !write_binary( b, elem ) ) //
      return false;
  return true;
}

} // namespace base
