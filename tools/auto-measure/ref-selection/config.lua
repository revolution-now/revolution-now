-- The purpose of this is to test how the OG decides how many REF
-- units (and which types) to send to a colony given the strength
-- of the colony's defenses.
--
-- NOTE: there are some requirements on NAMES.TXT that are
-- required to run this.  The tool will check these.
--
--   * man-o-war movement must be set to zero so that we don't
--     have to worry about waiting for the man-o-war to move
--     around after it lands (which could lead to it getting
--     fired on multiple times if there is a fort).
--
--   * man-or-war attack/combat set to 0 so that it always loses
--     when fired upon.
--
return {
  type='deterministic',

  combinatorial={
    difficulty={
      'discoverer', --
      'explorer', --
      'conquistador', --
      'governor', --
      'viceroy', --
    },

    fortification={
      'none', --
      'stockade', --
      'fort', --
      'fortress', --
    },

    -- This is the number of landing tiles around the ship on
    -- which REF units can be deployed. This value will actually
    -- be selected by the game given the land/water configuration
    -- around the colony. However, we need this here because it
    -- has to go into the output and we want to validate it
    -- against what we find in the map/sav file to ensure it is
    -- correct. It must be correct because the unit deployment
    -- formula depends on it.
    n_tiles=2,

    landed={ false, true },

    -- Doesn't seem to depend on horses count.
    horses=0,

    -- 2000 muskets appears to be enough to raise a single sol-
    -- dier to 2/2/2, so no need to go higher. That said, it is
    -- probably only necessary to have { 0, 50 }, that way we
    -- probe the even strength metrics, otherwise they would
    -- always be odd because all units have an even strength
    -- factor and the colony itself gets 1.
    muskets={ 0, 50, 100, 150, 200, 500, 1000, 2000 },

    -- Doesn't seem to depend on fortified status.
    orders='fortified',

    -- LuaFormatter off
    unit_set={
      -----------------------------------------------------------
      -- none
      -----------------------------------------------------------
      {},
      -----------------------------------------------------------
      -- soldier
      -----------------------------------------------------------
      { 'soldier' },
      { 'soldier', 'soldier' },
      { 'soldier', 'soldier', 'soldier' },
      { 'soldier', 'soldier', 'soldier', 'soldier' },
      { 'soldier', 'soldier', 'soldier', 'soldier', 'soldier' },
      -----------------------------------------------------------
      -- veteran_soldier
      -----------------------------------------------------------
      { 'veteran_soldier' },
      { 'veteran_soldier', 'veteran_soldier' },
      { 'veteran_soldier', 'veteran_soldier', 'veteran_soldier' },
      { 'veteran_soldier', 'veteran_soldier', 'veteran_soldier', 'veteran_soldier' },
      { 'veteran_soldier', 'veteran_soldier', 'veteran_soldier', 'veteran_soldier', 'veteran_soldier' },
      -----------------------------------------------------------
      -- dragoon
      -----------------------------------------------------------
      { 'dragoon' },
      { 'dragoon', 'dragoon' },
      { 'dragoon', 'dragoon', 'dragoon' },
      { 'dragoon', 'dragoon', 'dragoon', 'dragoon' },
      { 'dragoon', 'dragoon', 'dragoon', 'dragoon', 'dragoon' },
      -----------------------------------------------------------
      -- veteran_dragoon
      -----------------------------------------------------------
      { 'veteran_dragoon' },
      { 'veteran_dragoon', 'veteran_dragoon' },
      { 'veteran_dragoon', 'veteran_dragoon', 'veteran_dragoon' },
      { 'veteran_dragoon', 'veteran_dragoon', 'veteran_dragoon', 'veteran_dragoon' },
      { 'veteran_dragoon', 'veteran_dragoon', 'veteran_dragoon', 'veteran_dragoon', 'veteran_dragoon' },
      -----------------------------------------------------------
      -- artillery
      -----------------------------------------------------------
      { 'artillery' },
      { 'artillery', 'artillery' },
      { 'artillery', 'artillery', 'artillery' },
      { 'artillery', 'artillery', 'artillery', 'artillery' },
      { 'artillery', 'artillery', 'artillery', 'artillery', 'artillery' },
      -----------------------------------------------------------
      -- damaged_artillery
      -----------------------------------------------------------
      { 'damaged_artillery' },
      { 'damaged_artillery', 'damaged_artillery' },
      { 'damaged_artillery', 'damaged_artillery', 'damaged_artillery' },
      { 'damaged_artillery', 'damaged_artillery', 'damaged_artillery', 'damaged_artillery' },
      { 'damaged_artillery', 'damaged_artillery', 'damaged_artillery', 'damaged_artillery', 'damaged_artillery' },
      -----------------------------------------------------------
      -- continental_army
      -----------------------------------------------------------
      { 'continental_army' },
      { 'continental_army', 'continental_army' },
      { 'continental_army', 'continental_army', 'continental_army' },
      { 'continental_army', 'continental_army', 'continental_army', 'continental_army' },
      { 'continental_army', 'continental_army', 'continental_army', 'continental_army', 'continental_army' },
      -----------------------------------------------------------
      -- continental_cavalry
      -----------------------------------------------------------
      { 'continental_cavalry' },
      { 'continental_cavalry', 'continental_cavalry' },
      { 'continental_cavalry', 'continental_cavalry', 'continental_cavalry' },
      { 'continental_cavalry', 'continental_cavalry', 'continental_cavalry', 'continental_cavalry' },
      { 'continental_cavalry', 'continental_cavalry', 'continental_cavalry', 'continental_cavalry', 'continental_cavalry' },
      -----------------------------------------------------------
      -- mixed
      -----------------------------------------------------------
      { 'soldier','soldier','veteran_dragoon','veteran_dragoon' },
      { 'soldier','dragoon','dragoon','veteran_dragoon' },
      { 'dragoon','dragoon','dragoon','veteran_soldier' },
      { 'dragoon','dragoon','veteran_soldier','veteran_soldier' },
      { 'dragoon','veteran_soldier','veteran_soldier','veteran_soldier' },
      { 'soldier','veteran_soldier','veteran_soldier','veteran_dragoon' },
      { 'soldier','soldier','veteran_soldier','veteran_dragoon' },
      -----------------------------------------------------------
      -- soldier ramp up
      -----------------------------------------------------------
      { 'soldier', 'soldier', 'soldier', 'soldier', 'soldier',
        'soldier' },
      { 'soldier', 'soldier', 'soldier', 'soldier', 'soldier',
        'soldier', 'soldier' },
      { 'soldier', 'soldier', 'soldier', 'soldier', 'soldier',
        'soldier', 'soldier', 'soldier' },
      { 'soldier', 'soldier', 'soldier', 'soldier', 'soldier',
        'soldier', 'soldier', 'soldier', 'soldier' },
      { 'soldier', 'soldier', 'soldier', 'soldier', 'soldier',
        'soldier', 'soldier', 'soldier', 'soldier', 'soldier' },
      { 'soldier', 'soldier', 'soldier', 'soldier', 'soldier',
        'soldier', 'soldier', 'soldier', 'soldier', 'soldier',
        'soldier' },
      { 'soldier', 'soldier', 'soldier', 'soldier', 'soldier',
        'soldier', 'soldier', 'soldier', 'soldier', 'soldier',
        'soldier', 'soldier' },
      { 'soldier', 'soldier', 'soldier', 'soldier', 'soldier',
        'soldier', 'soldier', 'soldier', 'soldier', 'soldier',
        'soldier', 'soldier', 'soldier' },
      { 'soldier', 'soldier', 'soldier', 'soldier', 'soldier',
        'soldier', 'soldier', 'soldier', 'soldier', 'soldier',
        'soldier', 'soldier', 'soldier', 'soldier' },
      { 'soldier', 'soldier', 'soldier', 'soldier', 'soldier',
        'soldier', 'soldier', 'soldier', 'soldier', 'soldier',
        'soldier', 'soldier', 'soldier', 'soldier', 'soldier' },
    },
    -- LuaFormatter on
  },
}
