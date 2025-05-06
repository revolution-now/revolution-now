local NATIONS = { ENGLISH=1, FRENCH=2, SPANISH=3, DUTCH=4 }

local function lambda( sav )
  assert( sav )
  local HEADER = assert( sav.HEADER )
  local regulars = assert( HEADER.expeditionary_force.regulars )
  local cavalry = assert( HEADER.expeditionary_force.dragoons )
  local artillery =
      assert( HEADER.expeditionary_force.artillery )
  local ships =
      assert( HEADER.expeditionary_force['man-o-wars'] )
  local nation_idx = NATIONS.DUTCH
  local nation = assert( sav.NATION[nation_idx] )
  local royal_money = assert( nation.royal_money )
  local player_total_income =
      assert( nation.player_total_income )

  local total = regulars + cavalry + artillery + ships
  local total_land = regulars + cavalry + artillery

  if total_land == 0 then return end

  local pc = function( n, tot )
    return math.floor( ((n * 1.0) / tot) * 1000 ) / 10
  end

  local regulars_pc = pc( regulars, total )
  local cavalry_pc = pc( cavalry, total )
  local artillery_pc = pc( artillery, total )
  local ships_pc = pc( ships, total )

  local regulars_pc_land = pc( regulars, total_land )
  local cavalry_pc_land = pc( cavalry, total_land )
  local artillery_pc_land = pc( artillery, total_land )

  printf( '------------------------------------------------' )
  printf( 'regulars:  %3d  (%2.1f%%)  (%2.1f%%)', regulars,
          regulars_pc, regulars_pc_land )
  printf( 'cavalry:   %3d  (%2.1f%%)  (%2.1f%%)', cavalry,
          cavalry_pc, cavalry_pc_land )
  printf( 'artillery: %3d  (%2.1f%%)  (%2.1f%%)', artillery,
          artillery_pc, artillery_pc_land )
  printf( 'ships:     %3d  (%2.1f%%)', ships, ships_pc )
  print()
  printf( 'royal_money:          %d', royal_money )
  printf( 'player_total_income:  %d', player_total_income )
end

return lambda