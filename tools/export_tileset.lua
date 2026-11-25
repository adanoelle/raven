-- Write the tilesets of an Aseprite file's tilemap layers as one PNG: one
-- row of tiles per tilemap layer, bottom layer first. Run by
-- tools/export_tileset.sh, which passes the output path as "out".
local spr = app.activeSprite
local out = app.params["out"]
if not spr or not out then
  error("usage: aseprite -b FILE --script-param out=PNG --script export_tileset.lua")
end

local rows = {}
local columns = 0
for _, layer in ipairs(spr.layers) do
  if layer.isTilemap then
    rows[#rows + 1] = layer
    columns = math.max(columns, #layer.tileset)
  end
end
if #rows == 0 then
  error("no tilemap layers in " .. spr.filename)
end

local size = rows[1].tileset.grid.tileSize
local sheet = Image(columns * size.width, #rows * size.height, spr.colorMode)
for row, layer in ipairs(rows) do
  local tileset = layer.tileset
  local this = tileset.grid.tileSize
  if this.width ~= size.width or this.height ~= size.height then
    error("layer '" .. layer.name .. "' has a different tile size")
  end
  for tile = 0, #tileset - 1 do
    local image = tileset:getTile(tile)
    if image then
      sheet:drawImage(image, Point(tile * size.width, (row - 1) * size.height))
    end
  end
  print(string.format("row %d: %s, %d tiles (tile 0 is the empty tile)", row - 1, layer.name, #tileset))
end

local copy = Sprite(sheet.width, sheet.height, spr.colorMode)
copy:setPalette(spr.palettes[1])
copy.cels[1].image = sheet
copy:saveCopyAs(out)
copy:close()
print(string.format("wrote %s: %dx%d, tiles of %dx%d", out, sheet.width, sheet.height, size.width, size.height))
