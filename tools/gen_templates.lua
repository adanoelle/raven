-- Generates the blank Aseprite tier templates described in the book's
-- Aseprite Setup Guide (docs/book/src/art/art-aseprite-guide.md).
--
-- Usage:  aseprite -b --script tools/gen_templates.lua
--
-- Aseprite's real guide lines (View > Guides) are not scriptable, so each
-- template instead carries a locked, semi-transparent "guides" layer with
-- the same information drawn as pixels:
--   - cyan box      body zone boundary (drawn just OUTSIDE the body zone,
--                   in the padding, so it never covers body pixels)
--   - orange dots   proportion lines (HT head top, EY eye line, CH chin,
--                   WA waist)
--   - red dots      FT feet/ground line (the per-frame anchor lock)
--   - grey dots     center lines
-- Hide or delete the layer before export; drawing happens on the "art"
-- layer above it.

local pc = app.pixelColor
local COL_BODY = pc.rgba(0, 200, 255, 160)
local COL_PROP = pc.rgba(255, 140, 0, 170)
local COL_FEET = pc.rgba(255, 60, 60, 210)
local COL_CENT = pc.rgba(170, 170, 170, 90)

-- Tier definitions. body = {x0, y0, x1, y1} inclusive pixel range of the
-- body zone; props = proportion line rows (nil for tiers without them).
-- Values match the tables in the Aseprite Setup Guide sections 2.2 / 3.
local tiers = {
    {
        name = "small_24x24", size = 24,
        body = { 2, 2, 21, 21 }, center = 12,
        props = { ht = 3, ey = 8, ch = 12, wa = 17, ft = 21 },
    },
    {
        name = "medium_32x32", size = 32,
        body = { 4, 4, 27, 27 }, center = 16,
        props = { ht = 6, ey = 11, ch = 17, wa = 22, ft = 27 },
    },
    {
        -- Proportion rows are the 2-head chibi boss (guide section 3.3,
        -- Option A). For a 2.5-head boss, add Option B lines by hand.
        name = "large_48x48", size = 48,
        body = { 6, 6, 41, 41 }, center = 24,
        props = { ht = 9, ey = 17, ch = 25, wa = 33, ft = 40 },
    },
    {
        name = "xlarge_64x64", size = 64,
        body = { 8, 8, 55, 55 }, center = 32,
        props = nil,
    },
}

local function hline(img, y, x0, x1, color, step)
    for x = x0, x1, step do img:drawPixel(x, y, color) end
end

local function vline(img, x, y0, y1, color, step)
    for y = y0, y1, step do img:drawPixel(x, y, color) end
end

for _, t in ipairs(tiers) do
    local spr = Sprite(t.size, t.size, ColorMode.RGB)
    spr.gridBounds = Rectangle(0, 0, 8, 8)

    -- Bottom layer: locked guides.
    local guides = spr.layers[1]
    guides.name = "guides (hide before export)"
    local img = Image(t.size, t.size, ColorMode.RGB)

    -- Center cross (dotted, faint) first so other lines draw over it.
    vline(img, t.center, 0, t.size - 1, COL_CENT, 2)
    hline(img, t.center, 0, t.size - 1, COL_CENT, 2)

    -- Body zone boundary: solid box one pixel outside the zone.
    local bx0, by0, bx1, by1 = t.body[1] - 1, t.body[2] - 1,
                               t.body[3] + 1, t.body[4] + 1
    hline(img, by0, bx0, bx1, COL_BODY, 1)
    hline(img, by1, bx0, bx1, COL_BODY, 1)
    vline(img, bx0, by0, by1, COL_BODY, 1)
    vline(img, bx1, by0, by1, COL_BODY, 1)

    -- Proportion lines (dotted), spanning the body zone width.
    if t.props then
        local p = t.props
        for _, row in ipairs({ p.ht, p.ey, p.ch, p.wa }) do
            hline(img, row, t.body[1], t.body[3], COL_PROP, 2)
        end
        hline(img, p.ft, t.body[1], t.body[3], COL_FEET, 2)
    end

    spr:newCel(guides, 1, img, Point(0, 0))
    guides.isEditable = false

    -- Top layer: empty, ready to draw on.
    local art = spr:newLayer()
    art.name = "art"

    spr:saveAs("art/templates/" .. t.name .. ".aseprite")
    spr:close()
end

print("Templates written to art/templates/")
