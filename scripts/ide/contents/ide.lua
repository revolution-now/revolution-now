-- Describes what opens when we edit the IDE scripts themselves.

return {
  type='vsplit',
  what={
    {
      type='hsplit',
      what={
        'scripts/ide/contents.lua',
        'scripts/ide/util.lua',
      }
    },
    'scripts/ide/edit-rn.lua',
    'scripts/ide/layout.lua',
    {
      type='hsplit',
      what={
        'scripts/ide/module-cpp.lua',
        'scripts/ide/win.lua',
      }
    }
  }
}