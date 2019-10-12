/****************************************************************
**serial.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-28.
*
* Description: Serialization.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "enum.hpp"
#include "errors.hpp"

// base-util
#include "base-util/macros.hpp"
#include "base-util/pp.hpp"

// Flatbuffers
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/minireflect.h"

// C++ standard library
#include <cstddef>

namespace rn::serial {

/****************************************************************
** Binary Blobs
*****************************************************************/
class ByteBuffer {
  using byte_t = uint8_t;

public:
  ByteBuffer( byte_t* buf, int size )
    : size_( size ), buf_( buf ) {}

  static expect<ByteBuffer> read( fs::path const& file );

  int size() const { return size_; }

  byte_t const* get() const { return buf_.get(); }
  byte_t*       get() { return buf_.get(); }

private:
  int                       size_{0};
  std::unique_ptr<byte_t[]> buf_{};
};

class BinaryBlob {
public:
  BinaryBlob( uint8_t* buf, int size, int offset )
    : buf_( buf, size ), offset_( offset ) {}

  BinaryBlob( ByteBuffer&& buf )
    : buf_( std::move( buf ) ), offset_( 0 ) {}

  static expect<BinaryBlob> read( fs::path const& path );

  expect<> write( fs::path const& path ) const;

  // Must move argument.
  static BinaryBlob from_builder(
      flatbuffers::FlatBufferBuilder builder );

  static expect<BinaryBlob> from_json(
      fs::path const&  schema_file_name,
      fs::path const&  json_file_path,
      std::string_view root_type );

  template<typename FB>
  std::string to_json() const {
    flatbuffers::ToStringVisitor tostring_visitor( //
        /*delimiter=*/"\n",                        //
        /*quotes=*/true,                           //
        /*indent=*/"  ",                           //
        /*vdelimited=*/true                        //
    );
    flatbuffers::IterateFlatBuffer(
        get(), FB::MiniReflectTypeTable(), &tostring_visitor );
    return tostring_visitor.s;
  }

  uint8_t const* get() const { return buf_.get() + offset_; }
  uint8_t*       get() { return buf_.get() + offset_; }

  int size() const { return buf_.size() - offset_; }

private:
  ByteBuffer buf_;
  int        offset_;
};

/****************************************************************
** Testing
*****************************************************************/
void test_serial();

} // namespace rn::serial
