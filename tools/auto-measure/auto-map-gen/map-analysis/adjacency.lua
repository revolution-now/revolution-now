local dims = { w=10, h=10 }

local m = {}

local function create()
  for _ = 1, dims.h do
    local row = {}
    table.insert( m, row )
    for _ = 1, dims.w do table.insert( row, false ) end
  end
end

local function print_map()
  io.write( string.rep( '-', dims.w + 2 ) )
  io.write( '\n' )
  for y = 1, dims.h do
    io.write( '|' )
    for x = 1, dims.w do io.write( m[y][x] and 'x' or ' ' ) end
    io.write( '|\n' )
  end
  io.write( string.rep( '-', dims.w + 2 ) )
  io.write( '\n' )
end

local function assign( p )
  for y = 1, dims.h do
    for x = 1, dims.w do m[y][x] = math.random() < p end
  end
end

local function exists( coord )
  if coord.x < 1 or coord.y < 1 then return false end
  if coord.x > dims.w then return false end
  if coord.y > dims.h then return false end
  return true
end

local function on_surrounding( center_x, center_y, fn )
  local S = {
    { w=-1, h=-1 }, --
    { w=-1, h=0 }, --
    { w=-1, h=1 }, --
    { w=0, h=-1 }, --
    -- {w=0,h=0}, --
    { w=0, h=1 }, --
    { w=1, h=-1 }, --
    { w=1, h=0 }, --
    { w=1, h=1 }, --
  }
  for _, delta in ipairs( S ) do
    local coord = { x=center_x + delta.w, y=center_y + delta.h }
    if exists( coord ) then fn( coord ) end
  end
end

local function adjacency_inland()
  local res = 0
  local total_land = 0
  for y = 2, dims.h - 1 do
    for x = 2, dims.w - 1 do
      if m[y][x] then
        total_land = total_land + 1
        on_surrounding( x, y, function( coord )
          if m[coord.y][coord.x] then res = res + 1 end
        end )
      end
    end
  end
  return res -- / total_land
end

local function adjacency_full()
  local res = 0
  local total_land = 0
  for y = 1, dims.h do
    for x = 1, dims.w do
      if m[y][x] then
        total_land = total_land + 1
        on_surrounding( x, y, function( coord )
          if m[coord.y][coord.x] then res = res + 1 end
        end )
      end
    end
  end
  return res -- / total_land
end

local function predicted_inland( p )
  local total_inland = (dims.h - 2) * (dims.w - 2)
  return total_inland * p * 8 * p
end

local function predicted_full( p )
  local res = 0

  local total_inland = (dims.h - 2) * (dims.w - 2)
  res = res + total_inland * 8

  local total_border_non_corner =
      (dims.w - 2) * 2 + (dims.h - 2) * 2
  res = res + total_border_non_corner * 5

  local total_corners = 4
  res = res + total_corners * 3

  return res * p * p
  -- return (dims.h * dims.w) * 8 * p * p
end

create()
-- print_map()
local interval = .1
for p = interval, 1 + .0001, interval do
  local real = 0
  local iters = 100000
  for _ = 1, iters do
    assign( p )
    real = real + adjacency_full()
  end
  real = real / iters
  -- print_map()
  local predict = predicted_full( p )
  local error = 100 * (predict - real) / real
  print( string.format( '%.3f  %.3f  %.3f  %.1f%%', p, real,
                        predict, error ) )
end