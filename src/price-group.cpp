/****************************************************************
**price-group.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-16.
*
* Description: Implementation of the OG's market model governing
*              the movement of prices of rum, cigars, cloth, and
*              coats.
*
*****************************************************************/
#include "price-group.hpp"

// Revolution Now
#include "irand.hpp"
#include "logger.hpp"
#include "ts.hpp"

// config
#include "config/market.rds.hpp"

// luapp
#include "luapp/as.hpp"
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

int sum_values(
    refl::enum_map<e_processed_good, int> const& m ) {
  int res = 0;
  for( auto const& [k, v] : m ) res = res + v;
  return res;
}

} // namespace

void linker_dont_discard_module_price_group();
void linker_dont_discard_module_price_group() {}

/****************************************************************
** ProcessedGoodsPriceGroup
*****************************************************************/
void to_str( ProcessedGoodsPriceGroup const& o, string& out,
             base::ADL_t ) {
  out += fmt::format(
      "ProcessedGoodsPriceGroup{{config={},intrinsic_volumes={},"
      "traded_volumes={}}}",
      o.config_, o.intrinsic_volumes_, o.traded_volumes_ );
}

ProcessedGoodsPriceGroup::ProcessedGoodsPriceGroup(
    ProcessedGoodsPriceGroupConfig const& config )
  : config_( config ),
    intrinsic_volumes_( config.starting_intrinsic_volumes ),
    traded_volumes_( config.starting_traded_volumes ) {}

// The sign of `quantity` should represent the change in net
// volume in europe.
void ProcessedGoodsPriceGroup::transaction(
    e_processed_good good, int quantity ) {
  traded_volumes_[good] = traded_volumes_[good] + quantity;
  if( traded_volumes_[good] >= 0 ) {
    // This is one of the benefits that the Dutch get.
    if( !config_.dutch ) evolve();
  }
}

void ProcessedGoodsPriceGroup::buy( e_processed_good good,
                                    int              quantity ) {
  transaction( good, -quantity );
}

void ProcessedGoodsPriceGroup::sell( e_processed_good good,
                                     int quantity ) {
  transaction( good, quantity );
}

// This is the function that takes the two volumes (intrinsic
// volume and traded volume) and from them derives the equilib-
// rium prices (the prices toward which the actual player-visible
// prices will tend toward). The basic idea is is that the equi-
// librium prices are proportional to the inverses of the total
// volumes. The OG appears to do this:
//
//   1. Get the total volume for each good, ignoring negative
//      traded volumes, as usual.
//   2. Normalize them so that their average is one, which will
//      allow scaling them to our desired target value.
//   3. Take the inverse of that normalized volume, which is pro-
//      portional to the price, then scale it up to the target
//      value (which is fixed at 12).
//
// This process ensures that a couple of things:
//
//   1. The price of a good will generally go down when the vol-
//      umes go up, which makes sense because this is a basic
//      supply/demand mechanic that one would expect. This is
//      achieved by letting the prices be proportional to the in-
//      verse of the volumes.
//   2. But no matter what is bought/sold, the average price of
//      the four goods will (approximately) remain constant; if
//      one falls, the others will rise. This is achieved by nor-
//      malizing the volumes and then scaling them up to the
//      target value. This implements what was likely the design-
//      er's goal of encouraging the production and sale of all
//      four goods (or at least the more the better) by allowing
//      the prices to remain stable no matter how much is sold,
//      so long as multiple of the goods (the more, the better)
//      are sold. If the player only sells one, its price will
//      quickly drop and the others will rise. If the player
//      sells all four of them in alternation, none of the prices
//      will drop, no matter how much is sold. This way, if the
//      player can always rely on earning a lot of gold so long
//      as they are producing all four goods, and the prices will
//      never drop.
//
// It seems like, ideally, step 3 (taking the inverse) would be
// done before step 2 (normalizing), since presumably we want the
// average *price* of the goods to remain approximately constant
// and not their inverses (volumes) per se. But, that is what the
// original game appears to do.
//
// There are some additional technicalities, such as ignoring
// negative values in some cases, and rounding/nan behavior, that
// may not have reasons per se, but they are just what the orig-
// inal game appears to do.
//
// This function appears to reproduces the OG's numbers *exact-
// ly*, despite the complex behaviors of the prices, and is thus
// quite astonishing.
refl::enum_map<e_processed_good, int>
ProcessedGoodsPriceGroup::equilibrium_prices() {
  refl::enum_map<e_processed_good, int> res;
  refl::enum_map<e_processed_good, int> total_volumes;
  for( e_processed_good good :
       refl::enum_values<e_processed_good> )
    total_volumes[good] = intrinsic_volumes_[good] +
                          std::max( traded_volumes_[good], 0 );
  double const avg_total_volume =
      double( sum_values( total_volumes ) ) /
      refl::enum_count<e_processed_good>;
  for( e_processed_good good :
       refl::enum_values<e_processed_good> ) {
    // When all the total volumes are equal then this will be 1,
    // and then the below will yield the target price.
    double const normalized_volume =
        std::max( total_volumes[good], 0 ) / avg_total_volume;
    double floating_res =
        config_.target_price / normalized_volume;
    // nan can happen if both the numerator and denominator in
    // the above are both zero, which is not expected to happen
    // in normal game play, but just in case let's handle it.
    if( isnan( floating_res ) ) {
      floating_res = 1;
      lg.warn( "nan encountered in price group model." );
    }
    if( isinf( floating_res ) ) {
      // This can happen if the normalized_volume is zero.
      res[good] = config_.max;
      continue;
    }
    // The original game seems to use floor here and not round,
    // and it makes a difference.
    res[good] = static_cast<int>( floor( floating_res ) );
    res[good] = clamp( res[good], config_.min, config_.max );
  }
  return res;
}

// This is the function that will evolve the intrinsic volumes.
// It is done at the start of each turn, and also when buying
// selling a good in the harbor (unless the player is dutch).
//
// This function appears to reproduces the OG's numbers *exact-
// ly*, despite the complex behaviors of the prices, and is thus
// quite astonishing.
void ProcessedGoodsPriceGroup::evolve_intrinsic_volume(
    e_processed_good good ) {
  double r = intrinsic_volumes_[good];
  // The original game basically seems to ignore the traded
  // volume in all cases if it is negative (meaning that more of
  // the good has been bought than sold).
  int const vol = std::max( traded_volumes_[good], 0 );

  // The OG evolves the volumes each turn by multiplying the
  // total volume (intrinsic + traded) by .99. That said, it
  // never modifies the traded volumes, and so the evolution of
  // total volume is only reflected in the intrinsic volume.
  // Hence why we add in the traded volume, then multiply by .99,
  // then remove it again.
  //
  // Although the OG likely wanted to scale r down by 1%, it
  // likely uses a fixed point representation with 8 decimal
  // bits. In that representation, .9921875 is the closest one
  // can get to .99.
  //
  // The addition of .5 is needed to make the numbers match the
  // empirical data exactly. Not sure if the OG explicitly does
  // this or if it some kind of artifact of the way it does
  // floating point math. One possibility is that adding this
  // bias will prevent the intrinsic volumes from drifting all
  // the way to zero, which would not be good because this model
  // does not behave well when they hit zero.
  r = ( ( r + vol + .5 ) * .9921875 ) - vol;

  // The original game represents the intrinsic volumes as inte-
  // gers, and so to simulate that we need to round the intrinsic
  // volume after every evolution step.
  intrinsic_volumes_[good] = lround( r );
}

void ProcessedGoodsPriceGroup::evolve() {
  for( e_processed_good good :
       refl::enum_values<e_processed_good> )
    evolve_intrinsic_volume( good );
}

/****************************************************************
** Public API
*****************************************************************/
int generate_random_intrinsic_volume( TS& ts, int center,
                                      int window ) {
  int const bottom = center - window / 2;
  return static_cast<int>( floor(
      ts.rand.between_doubles( 0.0, 1.0 ) * window + bottom ) );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  {
    using U = ::rn::ProcessedGoodsPriceGroup;
    auto u  = st.usertype.create<U>();

    lua::table pg_tbl =
        lua::table::create_or_get( st["price_group"] );

    lua::table pg_constructor = lua::table::create_or_get(
        pg_tbl["ProcessedGoodsPriceGroup"] );

    pg_constructor["new_with_random_volumes"] =
        []( lua::table t ) {
          lua::state  st = lua::state::view( t.this_cthread() );
          auto const& model_params =
              config_market.processed_goods_model;
          ProcessedGoodsPriceGroupConfig config{
              .dutch                      = false,
              .starting_intrinsic_volumes = {},
              .starting_traded_volumes    = {},
              .min          = model_params.bid_price_min + 1,
              .max          = model_params.bid_price_max + 1,
              .target_price = model_params.target_price };
          CHECK( lua::safe_as<TS&>( st["TS"] ) );
          TS& ts = st["TS"].as<TS&>();
          for( e_processed_good good :
               refl::enum_values<e_processed_good> )
            config.starting_intrinsic_volumes[good] =
                generate_random_intrinsic_volume(
                    ts, model_params.random_init_center,
                    model_params.random_init_window );
          return ProcessedGoodsPriceGroup( config );
        };

    pg_constructor["new_with_zero_volumes"] = [] {
      auto const& model_params =
          config_market.processed_goods_model;
      ProcessedGoodsPriceGroupConfig config{
          .dutch                      = false,
          .starting_intrinsic_volumes = {},
          .starting_traded_volumes    = {},
          .min          = model_params.bid_price_min + 1,
          .max          = model_params.bid_price_max + 1,
          .target_price = model_params.target_price };
      return ProcessedGoodsPriceGroup( config );
    };

    u["equilibrium_prices"] =
        [&]( ProcessedGoodsPriceGroup& group ) {
          lua::table res       = st.table.create();
          auto       eq_prices = group.equilibrium_prices();
          res["rum"]    = eq_prices[e_processed_good::rum];
          res["cigars"] = eq_prices[e_processed_good::cigars];
          res["cloth"]  = eq_prices[e_processed_good::cloth];
          res["coats"]  = eq_prices[e_processed_good::coats];
          return res;
        };

    u["intrinsic_volume"] = []( ProcessedGoodsPriceGroup& group,
                                e_processed_good good ) -> int {
      return group.intrinsic_volumes()[good];
    };

    u["traded_volumes"] = []( ProcessedGoodsPriceGroup& group,
                              e_processed_good good ) -> int {
      return group.traded_volumes()[good];
    };
  };
};

} // namespace
} // namespace rn
