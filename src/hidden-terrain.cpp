/****************************************************************
**hidden-terrain.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-03-10.
*
* Description: Things related to the "Show Hidden Terrain"
*              feature.
*
*****************************************************************/
#include "hidden-terrain.hpp"

// Revolution Now
#include "anim-builder.hpp"
#include "irand.hpp"
#include "map-square.hpp"
#include "visibility.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// gfx
#include "gfx/iter.hpp"

using namespace std;

namespace rn {

namespace {

using ::std::chrono::milliseconds;

/****************************************************************
** ContentHider.
*****************************************************************/
struct ContentHider {
  ContentHider()          = default;
  virtual ~ContentHider() = default;

  ContentHider( ContentHider const& ) = delete;
  ContentHider( ContentHider&& )      = default;

  virtual void hide( AnimationBuilder& builder,
                     int stage ) const = 0;

  virtual void hide_final( AnimationBuilder& builder ) const = 0;

  virtual int num_stages() const = 0;

  virtual milliseconds delay() const = 0;
};

/****************************************************************
** ContentHiderImpl.
*****************************************************************/
template<typename T, typename Derived>
struct ContentHiderImpl : public ContentHider {
  ContentHiderImpl( SSConst const& ss, IVisibility const& viz,
                    vector<Coord> const& shuffled_tiles,
                    int target_chunks, T const& accum )
    : ss_( ss ),
      viz_( viz ),
      shuffled_tiles_( shuffled_tiles ),
      target_chunks_( std::min( target_chunks,
                                int( shuffled_tiles.size() ) ) ),
      accum_( accum ) {}

  using accum_t = T;

  Derived& as_derived() {
    return static_cast<Derived&>( *this );
  }

  Derived const& as_derived() const {
    return static_cast<Derived const&>( *this );
  }

  static Derived create( SSConst const& ss,
                         IVisibility const& viz,
                         vector<Coord> const& shuffled_tiles,
                         int target_chunks, T const& accum ) {
    Derived hider( ss, viz, shuffled_tiles, target_chunks,
                   accum );
    hider.compute_stages();
    return hider;
  }

  int num_stages() const override { return stages_.size(); }

  T const& accum() const { return accum_; }

  void hide( AnimationBuilder& builder,
             int stage ) const override {
    CHECK_LT( stage, int( stages_.size() ) );
    for( auto const& e : stages_[stage] )
      as_derived().hide_one( builder, e );
  }

  void hide_final( AnimationBuilder& builder ) const override {
    if( !stages_.empty() ) hide( builder, stages_.size() - 1 );
  }

  milliseconds delay() const override { return Derived::kDelay; }

  template<typename U>
  friend struct LandviewModMixin;

 protected:
  void compute_stages() {
    auto const [num_chunks, chunk_size] = [&] {
      int num_chunks = target_chunks_;
      if( num_chunks == 0 ) return pair{ 0, 0 };
      CHECK_GT( target_chunks_, 0 );
      int chunk_size = shuffled_tiles_.size() / target_chunks_;
      int const residual =
          shuffled_tiles_.size() - ( chunk_size * num_chunks );
      if( residual > 0 )
        num_chunks += std::max( residual / chunk_size, 1 );
      return pair{ num_chunks, chunk_size };
    }();

    for( int chunk = 0; chunk < num_chunks; ++chunk ) {
      int const chunk_start = chunk * chunk_size;
      CHECK_LT( chunk_start, int( shuffled_tiles_.size() ) );
      int const chunk_end =
          std::min( ( chunk + 1 ) * chunk_size,
                    int( shuffled_tiles_.size() ) );
      bool changed = false;
      for( int i = chunk_start; i < chunk_end; ++i ) {
        Coord const tile = shuffled_tiles_[i];
        if( viz_.visible( tile ) == e_tile_visibility::hidden )
          continue;
        changed = as_derived().on_tile( tile ) || changed;
      }
      if( !changed ) continue;
      stages_.push_back( accum_ );
    }
    erase_if( stages_,
              []( auto& elem ) { return elem.empty(); } );
  }

 protected:
  SSConst const& ss_;
  IVisibility const& viz_;
  vector<Coord> const shuffled_tiles_;
  int const target_chunks_;
  T accum_;
  vector<T> stages_;
};

/****************************************************************
** LandviewModMixin.
*****************************************************************/
template<typename Derived>
struct LandviewModMixin {
  Derived const& as_derived() const {
    return static_cast<Derived const&>( *this );
  }

  Derived& as_derived() {
    return static_cast<Derived&>( *this );
  }

  void hide_one( AnimationBuilder& builder,
                 auto const& elem ) const {
    auto const& [tile, square] = elem;
    builder.landview_replace_set_tile( tile, square );
  }

  MapSquare const& current_square( Coord tile ) const {
    auto& accum = as_derived().accum_;
    if( auto it = accum.find( tile ); it != accum.end() )
      return it->second;
    return as_derived().viz_.square_at( tile );
  }

  MapSquare& moddable_square( Coord tile ) {
    auto& accum = as_derived().accum_;
    if( auto it = accum.find( tile ); it != accum.end() )
      return it->second;
    accum[tile] = as_derived().viz_.square_at( tile );
    return accum[tile];
  }
};

/****************************************************************
** UnitsHider.
*****************************************************************/
struct UnitsHider final
  : public ContentHiderImpl<vector<GenericUnitId>, UnitsHider> {
  using Base =
      ContentHiderImpl<vector<GenericUnitId>, UnitsHider>;
  using Base::Base;

  inline static milliseconds const kDelay{ 30 };

  [[nodiscard]] bool on_tile( Coord tile ) {
    unordered_set<GenericUnitId> const& units =
        ss_.units.from_coord( tile );
    if( units.empty() ) return false;
    vector<GenericUnitId> units_ordered( units.begin(),
                                         units.end() );
    sort( units_ordered.begin(), units_ordered.end() );
    accum_.insert( accum_.end(), units_ordered.begin(),
                   units_ordered.end() );
    return true;
  }

  void hide_one( AnimationBuilder& builder,
                 GenericUnitId const& id ) const {
    builder.hide_unit( id );
  }
};

/****************************************************************
** DwellingHider.
*****************************************************************/
struct DwellingsHider final
  : public ContentHiderImpl<vector<Coord>, DwellingsHider> {
  using Base = ContentHiderImpl<vector<Coord>, DwellingsHider>;
  using Base::Base;

  inline static milliseconds const kDelay{ 20 };

  [[nodiscard]] bool on_tile( Coord tile ) {
    if( !viz_.dwelling_at( tile ).has_value() ) return false;
    accum_.push_back( tile );
    return true;
  }

  void hide_one( AnimationBuilder& builder,
                 Coord const& tile ) const {
    builder.hide_dwelling( tile );
  }
};

/****************************************************************
** ColoniesHider.
*****************************************************************/
struct ColoniesHider final
  : public ContentHiderImpl<vector<Coord>, ColoniesHider> {
  using Base = ContentHiderImpl<vector<Coord>, ColoniesHider>;
  using Base::Base;

  inline static milliseconds const kDelay{ 20 };

  [[nodiscard]] bool on_tile( Coord tile ) {
    if( !viz_.colony_at( tile ).has_value() ) return false;
    accum_.push_back( tile );
    return true;
  }

  void hide_one( AnimationBuilder& builder,
                 Coord const& tile ) const {
    builder.hide_colony( tile );
  }
};

/****************************************************************
** ImprovementsHider.
*****************************************************************/
struct ImprovementsHider final
  : public ContentHiderImpl<map<Coord, MapSquare>,
                            ImprovementsHider>,
    public LandviewModMixin<ImprovementsHider> {
  using Base =
      ContentHiderImpl<map<Coord, MapSquare>, ImprovementsHider>;
  using Base::Base;

  inline static milliseconds const kDelay{ 80 };

  [[nodiscard]] bool on_tile( Coord tile ) {
    MapSquare const& square = current_square( tile );
    if( !square.road && !square.irrigation ) return false;
    auto& new_square      = moddable_square( tile );
    new_square.road       = false;
    new_square.irrigation = false;
    return true;
  }
};

/****************************************************************
** ResourcesHider.
*****************************************************************/
struct ResourcesHider final
  : public ContentHiderImpl<map<Coord, MapSquare>,
                            ResourcesHider>,
    public LandviewModMixin<ResourcesHider> {
  using Base =
      ContentHiderImpl<map<Coord, MapSquare>, ResourcesHider>;
  using Base::Base;

  inline static milliseconds const kDelay{ 40 };

  [[nodiscard]] bool on_tile( Coord tile ) {
    MapSquare const& square = current_square( tile );
    // Note in the below we try to avoid adding an element to
    // the map unless we actually need to make a change.
    if( square.ground_resource.has_value() ||
        square.forest_resource.has_value() ||
        square.lost_city_rumor ) {
      auto& new_square = moddable_square( tile );
      if( new_square.ground_resource !=
          e_natural_resource::fish )
        new_square.ground_resource = nothing;
      new_square.forest_resource = nothing;
      new_square.lost_city_rumor = false;
      return true;
    }
    return false;
  }
};

/****************************************************************
** ForestHider.
*****************************************************************/
struct ForestHider final
  : public ContentHiderImpl<map<Coord, MapSquare>, ForestHider>,
    public LandviewModMixin<ForestHider> {
  using Base =
      ContentHiderImpl<map<Coord, MapSquare>, ForestHider>;
  using Base::Base;

  inline static milliseconds const kDelay{ 80 };

  [[nodiscard]] bool on_tile( Coord tile ) {
    MapSquare const& square = current_square( tile );
    if( square.overlay == e_land_overlay::forest ) {
      auto& new_square   = moddable_square( tile );
      new_square.overlay = nothing;
      return true;
    }
    return false;
  }
};

/****************************************************************
** HiderBuilder.
*****************************************************************/
struct HiderBuilder {
  HiderBuilder( SSConst const& ss, IVisibility const& viz,
                AnimationBuilder& builder,
                vector<Coord> const& shuffled_tiles,
                int target_chunks )
    : ss_( ss ),
      viz_( viz ),
      builder_( builder ),
      shuffled_tiles_( shuffled_tiles ),
      target_chunks_( target_chunks ) {}

  template<typename T, typename U>
  auto hider( unique_ptr<U> const& accum_from ) {
    CHECK( accum_from != nullptr );
    auto res = make_unique<T>(
        T::create( ss_, viz_, shuffled_tiles_, target_chunks_,
                   accum_from->accum() ) );
    build_hider( *res );
    return res;
  };

  template<typename T>
  auto hider() {
    auto res = make_unique<T>( T::create(
        ss_, viz_, shuffled_tiles_, target_chunks_, {} ) );
    build_hider( *res );
    return res;
  };

  void hide_previous() {
    for( ContentHider const* previous_hider : hiders_ )
      previous_hider->hide_final( builder_ );
  };

 private:
  void build_hider( ContentHider const& hider ) {
    for( int i = 0; i < hider.num_stages(); ++i ) {
      builder_.new_phase();
      hide_previous();
      hider.hide( builder_, i );
      builder_.delay( hider.delay() );
    }
    hiders_.push_back( &hider );
  };

  SSConst const& ss_;
  IVisibility const& viz_;
  AnimationBuilder& builder_;
  vector<Coord> shuffled_tiles_;
  int const target_chunks_;
  vector<ContentHider const*> hiders_;
};

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
HiddenTerrainAnimationSequence anim_seq_for_hidden_terrain(
    SSConst const& ss, IVisibility const& viz, IRand& rand ) {
  HiddenTerrainAnimationSequence res;
  AnimationBuilder builder;

  // When our chunk size is 10 that means that with each chunk we
  // add roughly 1/10th of the squares. However, because when re-
  // drawing a square we need to redraw its 8 surrounding
  // squares, this means that even on the very first phase when
  // we are modifying only 1/10th of the squares, we end up
  // roughly redrawing all of the land squares anyway, at least
  // when there is full forest coverage, which there usually will
  // be in the default map generation mode. This matters because
  // it means that even the first phase/chunk of the animation
  // will take as long to draw in the landscape_anim buffer as
  // the subsequent chunks where we are redrawing more tiles,
  // thus there isn't really any benefit to redrawing them incre-
  // mentally; it is fine to completely redraw the landscape_anim
  // buffer with each phase/chunk of the animation.
  static int const kTargetChunks = 10;

  vector<Coord> const shuffled_coords = [&] {
    vector<Coord> res;
    res.reserve( viz.rect_tiles().area() );
    for( gfx::point const p :
         gfx::rect_iterator( viz.rect_tiles() ) ) {
      Coord const tile = Coord::from_gfx( p ); // FIXME
      if( viz.visible( tile ) != e_tile_visibility::hidden )
        res.push_back( tile );
    }
    rand.shuffle( res );
    return res;
  }();

  HiderBuilder h( ss, viz, builder, shuffled_coords,
                  kTargetChunks );

  // 1. Hide.
  auto units        = h.hider<UnitsHider>();
  auto dwellings    = h.hider<DwellingsHider>();
  auto colonies     = h.hider<ColoniesHider>();
  auto improvements = h.hider<ImprovementsHider>();
  auto resources    = h.hider<ResourcesHider>( improvements );
  auto forest       = h.hider<ForestHider>( resources );
  res.hide          = std::move( builder ).result();

  // 2. Hold.
  builder.clear();
  h.hide_previous();
  res.hold = std::move( builder ).result();

  // 3. Spin up.
  res.show = res.hide;
  reverse( res.show.sequence.begin(), res.show.sequence.end() );

  return res;
}

} // namespace rn
