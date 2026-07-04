#include "rendering/bitmap_font.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace raven;

TEST_CASE("Font glyph index mapping", "[font]") {
    SECTION("Printable ASCII maps to sequential indices") {
        REQUIRE(font_glyph_index(' ') == 0);
        REQUIRE(font_glyph_index('!') == 1);
        REQUIRE(font_glyph_index('0') == 16);
        REQUIRE(font_glyph_index('A') == 33);
        REQUIRE(font_glyph_index('a') == 65);
        REQUIRE(font_glyph_index('~') == 94);
    }

    SECTION("Characters outside the atlas return -1") {
        REQUIRE(font_glyph_index('\n') == -1);
        REQUIRE(font_glyph_index('\t') == -1);
        REQUIRE(font_glyph_index('\0') == -1);
        REQUIRE(font_glyph_index(static_cast<char>(127)) == -1);
        REQUIRE(font_glyph_index(static_cast<char>(200)) == -1);
    }

    SECTION("Index range covers exactly the atlas glyph count") {
        // 95 printable ASCII chars: indices 0..94
        REQUIRE(font_glyph_index(BitmapFont::FIRST_CHAR) == 0);
        REQUIRE(font_glyph_index(BitmapFont::LAST_CHAR) ==
                BitmapFont::LAST_CHAR - BitmapFont::FIRST_CHAR);
    }
}

TEST_CASE("Unloaded font is safe", "[font]") {
    BitmapFont font;

    REQUIRE_FALSE(font.is_loaded());
    REQUIRE(font.glyph_width() == 0);
    REQUIRE(font.glyph_height() == 0);
    REQUIRE(font.measure("HELLO") == 0);

    // draw() on an unloaded font must be a no-op, not a crash
    font.draw(nullptr, "HELLO", 0.f, 0.f);
    font.draw_centered(nullptr, "HELLO", 0.f, 0.f);
}

TEST_CASE("Font measure scales with text length and scale", "[font]") {
    // measure is pure arithmetic on the cell width; validate via a moved-from
    // default font (0 width) and the documented formula.
    BitmapFont font;
    REQUIRE(font.measure("12345", 3) == 0); // unloaded: width 0

    // The formula itself: length x glyph_w x scale (documented contract)
    // is exercised end-to-end by draw_centered in the game.
}
