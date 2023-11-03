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
#include "maybe.hpp"

// C++ standard library.
#include <array>
#include <cstdint>
#include <type_traits>

namespace base {

namespace detail {
template<size_t N>
consteval auto uint_for_bits() {
  if constexpr( N <= 0 )
    return static_cast<void*>( nullptr ); // bad.
  // bool is not necessarily the smallest integral type that can
  // hold one bit, but it is probably a better API.
  else if constexpr( N <= 1 )
    return static_cast<bool*>( nullptr );
  else if constexpr( N <= 8 )
    return static_cast<uint8_t*>( nullptr );
  else if constexpr( N <= 16 )
    return static_cast<uint16_t*>( nullptr );
  else if constexpr( N <= 32 )
    return static_cast<uint32_t*>( nullptr );
  else if constexpr( N <= 64 )
    return static_cast<uint64_t*>( nullptr );
}
} // namespace detail

template<size_t N>
using uint_for_bits_t = std::remove_reference_t<
    decltype( *detail::uint_for_bits<N>() )>;

/****************************************************************
** BinaryData.
*****************************************************************/
struct BinaryData {
  bool good() const;

 private:
  maybe<uint8_t> buffer_;
};

/****************************************************************
** BinaryReader.
*****************************************************************/
struct BinaryReader : BinaryData {
  template<size_t N, typename Ret = uint_for_bits_t<N>>
  requires( sizeof( Ret ) <= sizeof( uint64_t ) &&
            sizeof( Ret ) >= ( N + 8 ) / 8 )
  [[nodiscard]] Ret read_n_bits() {
    if( !good() ) return 0;
    uint64_t const ret = read_n_bits( N );
    return static_cast<Ret>( ret );
  }

  template<typename T>
  T read_one() {
    T out = {};
    read_n_bytes( sizeof( T ), out );
  }

  template<typename T>
  bool read( T& out ) {
    if( !good() ) return false;
    // TODO
  }

 private:
  [[nodiscard]] uint64_t read_n_bits( int nbits );
};

/****************************************************************
** BinaryWriter.
*****************************************************************/
struct BinaryWriter : BinaryData {
  void write_bit( bool b );

  template<typename T>
  requires( (std::is_unsigned_v<T> ||
             std::is_unsigned_v<std::underlying_type_t<T>>) &&
            sizeof( T ) <= sizeof( uint64_t ) )
  void write_bits( int nbits, T n ) {
    auto input = static_cast<uint64_t>( n );
    for( int i = 0; i < nbits; ++i ) {
      write_bit( input & 1 );
      input >>= 1;
    }
  }

  template<typename T>
  bool write( T const& out ) {
    if( !good() ) return false;
    // TODO
  }
};

template<typename T>
requires std::is_integral_v<T>
bool read_binary( base::BinaryReader& b, T& o ) {
  // TODO
  return false;
}

template<typename T>
requires std::is_enum_v<T>
bool read_binary( base::BinaryReader& b, T& o ) {
  return read_binary(
      b, static_cast<std::underlying_type_t<T>&>( o ) );
}

template<typename T, size_t N>
bool read_binary( base::BinaryReader& b, std::array<T, N>& o ) {
  // TODO
  return false;
}

template<typename T>
requires std::is_integral_v<T>
bool write_binary( base::BinaryWriter& b, T const& o ) {
  // TODO
  return false;
}

template<typename T>
requires std::is_enum_v<T>
bool write_binary( base::BinaryWriter& b, T const& o ) {
  return write_binary(
      b, static_cast<std::underlying_type_t<T> const&>( o ) );
}

template<typename T, size_t N>
bool write_binary( base::BinaryWriter&     b,
                   std::array<T, N> const& o ) {
  // TODO
  return false;
}

} // namespace base
