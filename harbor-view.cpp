namespace rn {
namespace {

struct DragConnector {
  bool DRAG_CONNECT_CASE( cargo, dock ) const {
    return holds<HarborDraggableObject2::unit>(
               draggable_from_src( S, src ) )
        .has_value();
  }
  bool DRAG_CONNECT_CASE( cargo, cargo ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    if( !is_unit_in_port( S.ss_.units, ship ) ) return false;
    if( src.slot == dst.slot ) return true;
    UNWRAP_CHECK( cargo_object,
                  draggable_to_cargo_object(
                      draggable_from_src( S, src ) ) );
    return overload_visit(
        cargo_object,
        [&]( Cargo::unit ) {
          return S.ss_.units.unit_for( ship )
              .cargo()
              .fits_with_item_removed(
                  S.ss_.units,
                  /*cargo=*/cargo_object,   //
                  /*remove_slot=*/src.slot, //
                  /*insert_slot=*/dst.slot  //
              );
        },
        [&]( Cargo::commodity const& c ) {
          // If at least one quantity of the commodity can be
          // moved then we will allow (at least a partial
          // transfer) to proceed.
          auto size_one     = c.obj;
          size_one.quantity = 1;
          return S.ss_.units.unit_for( ship ).cargo().fits(
              S.ss_.units,
              /*cargo=*/Cargo::commodity{ size_one },
              /*slot=*/dst.slot );
        } );
  }
  bool DRAG_CONNECT_CASE( outbound, inport ) const {
    UNWRAP_CHECK(
        info, S.ss_.units.maybe_harbor_view_state_of( src.id ) );
    ASSIGN_CHECK_V( outbound, info.port_status,
                    PortStatus::outbound );
    // We'd like to do == 0.0 here, but this will avoid rounding
    // errors.
    return outbound.turns == 0;
  }
  bool DRAG_CONNECT_CASE( dock, inport_ship ) const {
    return S.ss_.units.unit_for( dst.id ).cargo().fits_somewhere(
        S.ss_.units, Cargo::unit{ src.id } );
  }
  bool DRAG_CONNECT_CASE( cargo, inport_ship ) const {
    auto dst_ship = dst.id;
    UNWRAP_CHECK( cargo_object,
                  draggable_to_cargo_object(
                      draggable_from_src( S, src ) ) );
    return overload_visit(
        cargo_object,
        [&]( Cargo::unit u ) {
          if( is_unit_onboard( S.ss_.units, u.id ) == dst_ship )
            return false;
          return S.ss_.units.unit_for( dst_ship )
              .cargo()
              .fits_somewhere( S.ss_.units, u );
        },
        [&]( Cargo::commodity const& c ) {
          // If even 1 quantity can fit then we can proceed
          // with (at least) a partial transfer.
          auto size_one     = c.obj;
          size_one.quantity = 1;
          return S.ss_.units.unit_for( dst_ship )
              .cargo()
              .fits_somewhere( S.ss_.units,
                               Cargo::commodity{ size_one } );
        } );
  }
  bool DRAG_CONNECT_CASE( market, cargo ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    if( !is_unit_in_port( S.ss_.units, ship ) ) return false;
    auto comm = Commodity{
        /*type=*/src.type, //
        // If the commodity can fit even with just one quan-
        // tity then it is allowed, since we will just insert
        // as much as possible if we can't insert 100.
        /*quantity=*/1 //
    };
    return S.ss_.units.unit_for( ship ).cargo().fits_somewhere(
        S.ss_.units, Cargo::commodity{ comm }, dst.slot );
  }
  bool DRAG_CONNECT_CASE( market, inport_ship ) const {
    auto comm = Commodity{
        /*type=*/src.type, //
        // If the commodity can fit even with just one quan-
        // tity then it is allowed, since we will just insert
        // as much as possible if we can't insert 100.
        /*quantity=*/1 //
    };
    return S.ss_.units.unit_for( dst.id ).cargo().fits_somewhere(
        S.ss_.units, Cargo::commodity{ comm },
        /*starting_slot=*/0 );
  }
  bool DRAG_CONNECT_CASE( cargo, market ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    if( !is_unit_in_port( S.ss_.units, ship ) ) return false;
    return S.ss_.units.unit_for( ship )
        .cargo()
        .template slot_holds_cargo_type<Cargo::commodity>(
            src.slot )
        .has_value();
  }
  bool operator()( auto const&, auto const& ) const {
    return false;
  }
};

struct DragUserInput {
  static wait<maybe<int>> ask_for_quantity( PS&         S,
                                            e_commodity type,
                                            string_view verb ) {
    string text = fmt::format(
        "What quantity of @[H]{}@[] would you like to {}? "
        "(0-100):",
        commodity_display_name( type ), verb );

    // FIXME: add proper initial value.
    maybe<int> const res = co_await S.ts_.gui.optional_int_input(
        { .msg           = text,
          .initial_value = 0,
          .min           = 0,
          .max           = 100 } );
    co_return res;
  }

  wait<bool> DRAG_CONFIRM_CASE( market, cargo ) const {
    src.quantity =
        co_await ask_for_quantity( S, src.type, "buy" );
    co_return src.quantity.has_value();
  }
  wait<bool> DRAG_CONFIRM_CASE( market, inport_ship ) const {
    src.quantity =
        co_await ask_for_quantity( S, src.type, "buy" );
    co_return src.quantity.has_value();
  }
  wait<bool> DRAG_CONFIRM_CASE( cargo, market ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    CHECK( is_unit_in_port( S.ss_.units, ship ) );
    UNWRAP_CHECK(
        commodity_ref,
        S.ss_.units.unit_for( ship )
            .cargo()
            .template slot_holds_cargo_type<Cargo::commodity>(
                src.slot ) );
    src.quantity = co_await ask_for_quantity(
        S, commodity_ref.obj.type, "sell" );
    co_return src.quantity.has_value();
  }
  wait<bool> DRAG_CONFIRM_CASE( cargo, inport_ship ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    CHECK( is_unit_in_port( S.ss_.units, ship ) );
    auto maybe_commodity_ref =
        S.ss_.units.unit_for( ship )
            .cargo()
            .template slot_holds_cargo_type<Cargo::commodity>(
                src.slot );
    if( !maybe_commodity_ref.has_value() )
      // It's a unit.
      co_return true;
    src.quantity = co_await ask_for_quantity(
        S, maybe_commodity_ref->obj.type, "move" );
    co_return src.quantity.has_value();
  }
};

struct DragPerform {
  void DRAG_PERFORM_CASE( cargo, cargo ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    UNWRAP_CHECK( cargo_object,
                  draggable_to_cargo_object(
                      draggable_from_src( S, src ) ) );
    overload_visit(
        cargo_object,
        [&]( Cargo::unit u ) {
          // Will first "disown" unit which will remove it
          // from the cargo.
          S.ss_.units.change_to_cargo_somewhere( ship, u.id,
                                                 dst.slot );
        },
        [&]( Cargo::commodity const& ) {
          move_commodity_as_much_as_possible(
              S.ss_.units, ship, src.slot, ship, dst.slot,
              /*max_quantity=*/nothing,
              /*try_other_dst_slots=*/false );
        } );
  }
  void DRAG_PERFORM_CASE( outbound, inport ) const {
    unit_sail_to_harbor( S.ss_.terrain, S.ss_.units, S.player,
                         src.id );
  }
  void DRAG_PERFORM_CASE( inport, outbound ) const {
    HarborState& hb_state = S.harbor_state();
    unit_sail_to_new_world( S.ss_.terrain, S.ss_.units, S.player,
                            src.id );
    // This is not strictly necessary, but for a nice user expe-
    // rience we will auto-select another unit that is in-port
    // (if any) since that is likely what the user wants to work
    // with, as opposed to keeping the selection on the unit that
    // is now outbound. Or if there are no more units in port,
    // just deselect.
    hb_state.selected_unit = nothing;
    vector<UnitId> units_in_port =
        harbor_units_in_port( S.ss_.units, S.player.nation );
    hb_state.selected_unit = rl::all( units_in_port ).head();
  }
  void DRAG_PERFORM_CASE( dock, inport_ship ) const {
    S.ss_.units.change_to_cargo_somewhere( dst.id, src.id );
  }
  void DRAG_PERFORM_CASE( cargo, inport_ship ) const {
    UNWRAP_CHECK( cargo_object,
                  draggable_to_cargo_object(
                      draggable_from_src( S, src ) ) );
    overload_visit(
        cargo_object,
        [&]( Cargo::unit u ) {
          CHECK( !src.quantity.has_value() );
          // Will first "disown" unit which will remove it
          // from the cargo.
          S.ss_.units.change_to_cargo_somewhere( dst.id, u.id );
        },
        [&]( Cargo::commodity const& ) {
          UNWRAP_CHECK( src_ship,
                        active_cargo_ship( entities ) );
          move_commodity_as_much_as_possible(
              S.ss_.units, src_ship, src.slot,
              /*dst_ship=*/dst.id,
              /*dst_slot=*/0,
              /*max_quantity=*/src.quantity,
              /*try_other_dst_slots=*/true );
        } );
  }
  void DRAG_PERFORM_CASE( market, cargo ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    auto comm = Commodity{
        /*type=*/src.type, //
        /*quantity=*/src.quantity.value_or(
            k_default_market_quantity ) //
    };
    comm.quantity = std::min(
        comm.quantity,
        S.ss_.units.unit_for( ship )
            .cargo()
            .max_commodity_quantity_that_fits( src.type ) );
    // Cap it.
    comm.quantity =
        std::min( comm.quantity, k_default_market_quantity );
    CHECK( comm.quantity > 0 );
    add_commodity_to_cargo( S.ss_.units, comm, ship,
                            /*slot=*/dst.slot,
                            /*try_other_slots=*/true );
  }
  void DRAG_PERFORM_CASE( market, inport_ship ) const {
    auto comm = Commodity{
        /*type=*/src.type, //
        /*quantity=*/src.quantity.value_or(
            k_default_market_quantity ) //
    };
    comm.quantity = std::min(
        comm.quantity,
        S.ss_.units.unit_for( dst.id )
            .cargo()
            .max_commodity_quantity_that_fits( src.type ) );
    // Cap it.
    comm.quantity =
        std::min( comm.quantity, k_default_market_quantity );
    CHECK( comm.quantity > 0 );
    add_commodity_to_cargo( S.ss_.units, comm, dst.id,
                            /*slot=*/0,
                            /*try_other_slots=*/true );
  }
  void DRAG_PERFORM_CASE( cargo, market ) const {
    UNWRAP_CHECK( ship, active_cargo_ship( entities ) );
    UNWRAP_CHECK(
        commodity_ref,
        S.ss_.units.unit_for( ship )
            .cargo()
            .template slot_holds_cargo_type<Cargo::commodity>(
                src.slot ) );
    auto quantity_wants_to_sell =
        src.quantity.value_or( commodity_ref.obj.quantity );
    int       amount_to_sell = std::min( quantity_wants_to_sell,
                                         commodity_ref.obj.quantity );
    Commodity new_comm       = commodity_ref.obj;
    new_comm.quantity -= amount_to_sell;
    rm_commodity_from_cargo( S.ss_.units, ship, src.slot );
    if( new_comm.quantity > 0 )
      add_commodity_to_cargo( S.ss_.units, new_comm, ship,
                              /*slot=*/src.slot,
                              /*try_other_slots=*/false );
  }
  void operator()( auto const&, auto const& ) const {
    SHOULD_NOT_BE_HERE;
  }
};

} // namespace
} // namespace rn
