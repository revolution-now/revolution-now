-- The purpose of this is to test how the OG decides how many REF
-- units (and which types) to send to a colony given the strength
-- of the colony's defenses.
return {
  type='deterministic',

  combinatorial={
    difficulty={
      -- 'discoverer', --
      'conquistador', --
      -- 'viceroy', --
    },

    fortification={
      'none', --
      -- 'stockade', --
      -- 'fort', --
      -- 'fortress', --
    },

    horses={
      0, --
      50, --
      -- 100, --
      -- 150, --
      -- 200, --
    },

    muskets={
      0, --
      50, --
      -- 100, --
      -- 150, --
      -- 200, --
    },

    unit_sets={
      -- soldier
      'soldier', --
      'soldier-soldier', --
      'soldier-soldier-soldier', --
      -- 'soldier-soldier-soldier-soldier', --
      -- 'soldier-soldier-soldier-soldier-soldier', --
      -- -- veteran_soldier
      -- 'veteran_soldier', --
      -- 'veteran_soldier-veteran_soldier', --
      -- 'veteran_soldier-veteran_soldier-veteran_soldier', --
      -- 'veteran_soldier-veteran_soldier-veteran_soldier-veteran_soldier', --
      -- 'veteran_soldier-veteran_soldier-veteran_soldier-veteran_soldier-veteran_soldier', --
      -- -- dragoon
      -- 'dragoon', --
      -- 'dragoon-dragoon', --
      -- 'dragoon-dragoon-dragoon', --
      -- 'dragoon-dragoon-dragoon-dragoon', --
      -- 'dragoon-dragoon-dragoon-dragoon-dragoon', --
      -- -- veteran_dragoon
      -- 'veteran_dragoon', --
      -- 'veteran_dragoon-veteran_dragoon', --
      -- 'veteran_dragoon-veteran_dragoon-veteran_dragoon', --
      -- 'veteran_dragoon-veteran_dragoon-veteran_dragoon-veteran_dragoon', --
      -- 'veteran_dragoon-veteran_dragoon-veteran_dragoon-veteran_dragoon-veteran_dragoon', --
      -- -- 'artillery', --
      -- 'artillery', --
      -- 'artillery-artillery', --
      -- 'artillery-artillery-artillery', --
      -- 'artillery-artillery-artillery-artillery', --
      -- 'artillery-artillery-artillery-artillery-artillery', --
      -- -- 'damaged_artillery', --
      -- 'damaged_artillery', --
      -- 'damaged_artillery-damaged_artillery', --
      -- 'damaged_artillery-damaged_artillery-damaged_artillery', --
      -- 'damaged_artillery-damaged_artillery-damaged_artillery-damaged_artillery', --
      -- 'damaged_artillery-damaged_artillery-damaged_artillery-damaged_artillery-damaged_artillery', --
      -- -- 'continental_army', --
      -- 'continental_army', --
      -- 'continental_army-continental_army', --
      -- 'continental_army-continental_army-continental_army', --
      -- 'continental_army-continental_army-continental_army-continental_army', --
      -- 'continental_army-continental_army-continental_army-continental_army-continental_army', --
      -- -- 'continental_cavalry', --
      -- 'continental_cavalry', --
      -- 'continental_cavalry-continental_cavalry', --
      -- 'continental_cavalry-continental_cavalry-continental_cavalry', --
      -- 'continental_cavalry-continental_cavalry-continental_cavalry-continental_cavalry', --
      -- 'continental_cavalry-continental_cavalry-continental_cavalry-continental_cavalry-continental_cavalry', --
    },
  },
}
