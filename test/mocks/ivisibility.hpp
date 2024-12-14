/****************************************************************
**ivisibility.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-14.
*
* Description: For dependency injection in unit tests.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "src/visibility.hpp"

// mock
#include "src/mock/mock.hpp"

// base
#include "refl/to-str.hpp"

namespace rn {

struct MockIVisibility : IVisibility {
  MockIVisibility( SSConst const& ss ) : IVisibility( ss ) {}

  MOCK_METHOD( base::maybe<e_nation>, nation, (), ( const ) );
  MOCK_METHOD( e_tile_visibility, visible, ( gfx::point ),
               ( const ) );
  using MaybeColonyRef   = maybe<Colony const&>;
  using MaybeDwellingRef = maybe<Dwelling const&>;
  MOCK_METHOD( MaybeColonyRef, colony_at, ( gfx::point ),
               ( const ) );
  MOCK_METHOD( MaybeDwellingRef, dwelling_at, ( gfx::point ),
               ( const ) );
  MOCK_METHOD( MapSquare const&, square_at, ( gfx::point ),
               ( const ) );
};

static_assert( !std::is_abstract_v<MockIVisibility> );

} // namespace rn
