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
      -- 'discoverer', --
      'conquistador', --
      -- 'viceroy', --
    },

    fortification={
      -- 'none', --
      -- 'stockade', --
      'fort', --
      -- 'fortress', --
    },

    already_landed={
      false, --
      -- true, --
    },

    horses={
      0, --
      -- 50, --
      -- 100, --
      -- 150, --
      -- 200, --
    },

    muskets={
      0, --
      -- 50, --
      -- 100, --
      -- 150, --
      -- 200, --
    },

    -- Doesn't seem to depend on fortified status.
    orders={
      'fortified', --
    },

    -- LuaFormatter off
    unit_set={
    -----------------------------------------------------------
    -- soldier
    -----------------------------------------------------------
    { 'soldier' },
    { 'soldier', 'soldier' },
    { 'soldier', 'soldier', 'soldier' },
    { 'soldier', 'soldier', 'soldier', 'soldier' },
    { 'soldier', 'soldier', 'soldier', 'soldier', 'soldier' },
    --   -----------------------------------------------------------
    --   -- veteran_soldier
    --   -----------------------------------------------------------
    --   { 'veteran_soldier' },
    --   { 'veteran_soldier', 'veteran_soldier' },
    --   { 'veteran_soldier', 'veteran_soldier', 'veteran_soldier' },
    --   { 'veteran_soldier', 'veteran_soldier', 'veteran_soldier', 'veteran_soldier' },
    --   { 'veteran_soldier', 'veteran_soldier', 'veteran_soldier', 'veteran_soldier', 'veteran_soldier' },
    --   -----------------------------------------------------------
    --   -- dragoon
    --   -----------------------------------------------------------
    --   { 'dragoon' },
    --   { 'dragoon', 'dragoon' },
    --   { 'dragoon', 'dragoon', 'dragoon' },
    --   { 'dragoon', 'dragoon', 'dragoon', 'dragoon' },
    --   { 'dragoon', 'dragoon', 'dragoon', 'dragoon', 'dragoon' },
    --   -----------------------------------------------------------
    --   -- veteran_dragoon
    --   -----------------------------------------------------------
    --   { 'veteran_dragoon' },
    --   { 'veteran_dragoon', 'veteran_dragoon' },
    --   { 'veteran_dragoon', 'veteran_dragoon', 'veteran_dragoon' },
    --   { 'veteran_dragoon', 'veteran_dragoon', 'veteran_dragoon', 'veteran_dragoon' },
    --   { 'veteran_dragoon', 'veteran_dragoon', 'veteran_dragoon', 'veteran_dragoon', 'veteran_dragoon' },
    -----------------------------------------------------------
    -- artillery
    -----------------------------------------------------------
    -- { 'artillery' },
    -- { 'artillery', 'artillery' },
    -- { 'artillery', 'artillery', 'artillery' },
    -- { 'artillery', 'artillery', 'artillery', 'artillery' },
    -- { 'artillery', 'artillery', 'artillery', 'artillery', 'artillery' },
    -----------------------------------------------------------
    -- damaged_artillery
    -----------------------------------------------------------
    -- { 'damaged_artillery' },
    -- { 'damaged_artillery', 'damaged_artillery' },
    -- { 'damaged_artillery', 'damaged_artillery', 'damaged_artillery' },
    -- { 'damaged_artillery', 'damaged_artillery', 'damaged_artillery', 'damaged_artillery' },
    -- { 'damaged_artillery', 'damaged_artillery', 'damaged_artillery', 'damaged_artillery', 'damaged_artillery' },
    -----------------------------------------------------------
    -- continental_army
    -----------------------------------------------------------
    -- { 'continental_army' },
    -- { 'continental_army', 'continental_army' },
    -- { 'continental_army', 'continental_army', 'continental_army' },
    -- { 'continental_army', 'continental_army', 'continental_army', 'continental_army' },
    -- { 'continental_army', 'continental_army', 'continental_army', 'continental_army', 'continental_army' },
    -----------------------------------------------------------
    -- continental_cavalry
    -----------------------------------------------------------
    -- { 'continental_cavalry' },
    -- { 'continental_cavalry', 'continental_cavalry' },
    -- { 'continental_cavalry', 'continental_cavalry', 'continental_cavalry' },
    -- { 'continental_cavalry', 'continental_cavalry', 'continental_cavalry', 'continental_cavalry' },
    -- { 'continental_cavalry', 'continental_cavalry', 'continental_cavalry', 'continental_cavalry', 'continental_cavalry' }
    -----------------------------------------------------------
    -- mixed
    -----------------------------------------------------------
    -- { 'soldier','soldier','veteran_dragoon','veteran_dragoon' },
    -- { 'soldier','dragoon','dragoon','veteran_dragoon' },
    -- { 'dragoon','dragoon','dragoon','dragoon' },
    -- { 'dragoon','dragoon','dragoon','veteran_soldier' },
    -- { 'dragoon','dragoon','veteran_soldier','veteran_soldier' },
    -- { 'dragoon','veteran_soldier','veteran_soldier','veteran_soldier' },
    -- { 'veteran_soldier','veteran_soldier','veteran_soldier','veteran_soldier' },
    -- { 'soldier','veteran_soldier','veteran_soldier','veteran_dragoon' },
    -- { 'soldier','soldier','veteran_soldier','veteran_dragoon' },
    },
    -- LuaFormatter on
  },
}
