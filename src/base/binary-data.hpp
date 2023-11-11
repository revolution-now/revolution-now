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
#include "zero.hpp"

// C++ standard library.
#include <array>
#include <concepts>
#include <span>
#include <type_traits>

namespace base {

/****************************************************************
** IBinaryIO.
*****************************************************************/
// Reads or writes binary via some medium.
struct IBinaryIO {
  using Byte = unsigned char;

 protected:
  IBinaryIO()                   = default;
  IBinaryIO( IBinaryIO const& ) = default;

  virtual ~IBinaryIO() = default;

  virtual bool read_bytes( int n, Byte* dst ) = 0;

  virtual bool write_bytes( int n, Byte const* src ) = 0;

  virtual int pos() const = 0;

  virtual int size() const = 0;

 public:
  template<typename..., typename T>
  requires std::is_integral_v<T>
  bool read( T& out ) {
    static auto constexpr nbytes = sizeof( T );
    return read_bytes( nbytes, reinterpret_cast<Byte*>( &out ) );
  }

  template<typename..., typename T>
  requires std::is_integral_v<T>
  bool write( T const& in ) {
    static auto constexpr nbytes = sizeof( T );
    return write_bytes( nbytes,
                        reinterpret_cast<Byte const*>( &in ) );
  }

  template<size_t N, typename..., typename T>
  requires std::is_integral_v<T> && ( N <= sizeof( T ) )
  bool read_bytes( T& out ) {
    out = 0;
    return read_bytes( N, reinterpret_cast<Byte*>( &out ) );
  }

  template<size_t N, typename..., typename T>
  requires std::is_integral_v<T> && ( N <= sizeof( T ) )
  bool write_bytes( T const& in ) {
    return write_bytes( N,
                        reinterpret_cast<Byte const*>( &in ) );
  }

  // Note that this is not quite the same as std::feof, which
  // will only be set when attempting to go beyond the end. This
  // just tells you whether or not you are at the end, which is
  // probably more useful.
  bool eof() const;

  // Number of bytes remaining.
  int remaining() const;

  // Reads the remainder of the data into a vector.
  std::vector<Byte> read_remainder();
};

/****************************************************************
** FileReaderBinaryIO.
*****************************************************************/
// Reads or writes to a binary buffer.
struct FileBinaryIO : IBinaryIO, zero<FileBinaryIO, FILE*> {
  // This allows writing as well, but requires that the file
  // exist and won't truncate it.
  static expect<FileBinaryIO, std::string>
  open_for_rw_fail_on_nonexist( std::string const& path );

  // This allows reading as well, but will destroy the contents
  // if it exists.
  static expect<FileBinaryIO, std::string>
  open_for_rw_and_truncate( std::string const& path );

  using IBinaryIO::read_bytes;
  using IBinaryIO::write_bytes;

  int pos() const override;

  int size() const override;

 private:
  FileBinaryIO( FILE* fp ) : zero( fp ) {
    CHECK( fp != nullptr );
  };

  bool read_bytes( int n, unsigned char* dst ) override;
  bool write_bytes( int n, unsigned char const* src ) override;

  // Implement base::zero.
  friend zero<FileBinaryIO, FILE*>;
  void free_resource();
};

/****************************************************************
** MemBufferBinaryIO.
*****************************************************************/
// Reads or writes to a binary buffer.
//
// NOTE: At the time of writing this is not actually used by the
// game, but it is useful to keep it around because it allows
// testing the various methods in IBinaryIO in a way that doesn't
// involve using files, which makes it easier to check results.
// So it might be worth leaving around for that reason. That
// said, if the game doesn't ever actually use it, then probably
// best to move this into the unit test file instead of keeping
// it public.
//
struct MemBufferBinaryIO : IBinaryIO {
  MemBufferBinaryIO( std::span<unsigned char> buffer )
    : buffer_( buffer ) {}

  using IBinaryIO::read_bytes;
  using IBinaryIO::write_bytes;

  int pos() const override { return idx_; }

  int size() const override { return buffer_.size(); }

 private:
  bool read_bytes( int n, unsigned char* dst ) override;
  bool write_bytes( int n, unsigned char const* src ) override;

  std::span<unsigned char> buffer_;
  int                      idx_ = 0;
};

/****************************************************************
** Concepts.
*****************************************************************/
template<typename T>
concept ToBinary = requires( IBinaryIO& b, T const& o ) {
  { write_binary( b, o ) } -> std::same_as<bool>;
};

template<typename T>
concept FromBinary = requires( IBinaryIO& b, T& o ) {
  { read_binary( b, o ) } -> std::same_as<bool>;
};

template<typename T>
concept Binable = ToBinary<T> && FromBinary<T>;

/****************************************************************
** Instances (builtins).
*****************************************************************/
template<typename T>
requires std::is_integral_v<T>
bool read_binary( base::IBinaryIO& b, T& o ) {
  return b.read( o );
}

template<typename T>
requires std::is_integral_v<T>
bool write_binary( base::IBinaryIO& b, T const& o ) {
  return b.write( o );
}

template<typename T>
requires std::is_enum_v<T> &&
         FromBinary<std::underlying_type_t<T>>
bool read_binary( base::IBinaryIO& b, T& o ) {
  using U = std::underlying_type_t<T>;
  U res;
  if( !read_binary( b, res ) ) return false;
  o = static_cast<T>( res );
  return true;
}

template<typename T>
requires std::is_enum_v<T> && ToBinary<std::underlying_type_t<T>>
bool write_binary( base::IBinaryIO& b, T const& o ) {
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
bool read_binary( base::IBinaryIO& b, std::array<T, N>& o ) {
  for( T& elem : o )
    if( !read_binary( b, elem ) ) //
      return false;
  return true;
}

template<ToBinary T, size_t N>
bool write_binary( base::IBinaryIO&        b,
                   std::array<T, N> const& o ) {
  for( T const& elem : o )
    if( !write_binary( b, elem ) ) //
      return false;
  return true;
}

} // namespace base
