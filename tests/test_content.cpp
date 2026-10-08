// Shipped-data validation: loads the real stages, patterns, config and LDtk
// map from the source tree and checks them against each other and against
// what the code expects. A typo in content fails here instead of producing
// a softlocked room, an enemy that never fires, or a grey placeholder box.

#include "core/fs.hpp"
#include "core/string_id.hpp"
#include "ecs/components.hpp"
#include "ecs/enemy_library.hpp"
#include "ecs/systems/wave_system.hpp"
#include "patterns/pattern_library.hpp"
#include "rendering/animation_library.hpp"
#include "rendering/sheet_ids.hpp"
#include "rendering/tilemap.hpp"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

using namespace raven;

namespace {

const std::string SOURCE_DIR = RAVEN_SOURCE_DIR "/";

/// @brief Read and parse a JSON file from the source tree.
nlohmann::json read_json(const std::string& relative) {
    const auto text = fs::read_text(SOURCE_DIR + relative);
    REQUIRE(text.has_value());
    return nlohmann::json::parse(*text);
}

/// @brief Whether a file exists in the source tree.
bool source_file_exists(const std::string& relative) {
    SDL_PathInfo info{};
    return SDL_GetPathInfo((SOURCE_DIR + relative).c_str(), &info) &&
           info.type == SDL_PATHTYPE_FILE;
}

/// @brief Stage JSON files listed in the manifest, as source-relative paths.
std::vector<std::string> stage_paths() {
    const auto manifest = read_json("assets/data/stages/stage_manifest.json");
    std::vector<std::string> paths;
    for (const auto& p : manifest.at("stages")) {
        paths.push_back(p.get<std::string>());
    }
    return paths;
}

/// @brief Pattern library loaded from the shipped manifest.
PatternLibrary load_patterns(StringInterner& interner) {
    const auto manifest = read_json("assets/data/patterns/manifest.json");
    PatternLibrary patterns;
    patterns.set_interner(interner);
    for (const auto& p : manifest.at("patterns")) {
        REQUIRE(patterns.load_file(SOURCE_DIR + p.get<std::string>()));
    }
    REQUIRE_FALSE(patterns.names().empty());
    return patterns;
}

/// @brief Software renderer so Tilemap::load can create tileset textures headlessly.
struct SoftwareRenderer {
    SDL_Surface* surface = SDL_CreateSurface(16, 16, SDL_PIXELFORMAT_RGBA8888);
    SDL_Renderer* renderer = surface ? SDL_CreateSoftwareRenderer(surface) : nullptr;

    SoftwareRenderer() = default;
    SoftwareRenderer(const SoftwareRenderer&) = delete;
    SoftwareRenderer& operator=(const SoftwareRenderer&) = delete;
    SoftwareRenderer(SoftwareRenderer&&) = delete;
    SoftwareRenderer& operator=(SoftwareRenderer&&) = delete;

    ~SoftwareRenderer() {
        if (renderer) {
            SDL_DestroyRenderer(renderer);
        }
        if (surface) {
            SDL_DestroySurface(surface);
        }
    }
};

/// @brief Find a layer instance by name within a level of a parsed LDtk project.
nlohmann::json& layer_instance(nlohmann::json& project, const std::string& level,
                               const std::string& layer) {
    for (auto& lv : project.at("levels")) {
        if (lv.at("identifier") == level) {
            for (auto& li : lv.at("layerInstances")) {
                if (li.at("__identifier") == layer) {
                    return li;
                }
            }
        }
    }
    FAIL("layer '" << layer << "' not found in level '" << level << "'");
    throw std::logic_error("unreachable");
}

/// @brief Load Test_Room from a modified copy of the shipped LDtk map.
Tilemap load_modified_test_room(SDL_Renderer* renderer,
                                const std::function<void(nlohmann::json&)>& modify) {
    auto project = read_json("assets/maps/raven.ldtk");
    // The copy is written outside assets/maps/, so make tileset paths absolute
    for (auto& tileset : project.at("defs").at("tilesets")) {
        tileset["relPath"] = SOURCE_DIR + "assets/maps/" + tileset.at("relPath").get<std::string>();
    }
    modify(project);

    const std::string path = "test_tilemap_tmp.ldtk";
    REQUIRE(fs::write_text(path, project.dump()));
    Tilemap map;
    const bool loaded = map.load(renderer, path, "Test_Room");
    SDL_RemovePath(path.c_str());
    REQUIRE(loaded);
    return map;
}

/// @brief Whether a spawn point sits inside a solid collision cell.
bool in_solid_cell(const Tilemap& map, const SpawnPoint& sp) {
    return map.is_cell_solid(static_cast<int>(sp.x) / map.cell_size(),
                             static_cast<int>(sp.y) / map.cell_size());
}

} // namespace

TEST_CASE("Every enemy definition loads and uses a known pattern", "[content]") {
    const auto data = read_json("assets/data/enemies.json");
    EnemyLibrary enemies;
    REQUIRE(enemies.load_json(data, "enemies.json"));
    // A definition with a bad tier, archetype or value is skipped at load
    CHECK(enemies.count() == static_cast<int>(data.at("enemies").size()));

    StringInterner interner;
    const auto patterns = load_patterns(interner);
    for (const auto& name : enemies.names()) {
        const EnemyDef& def = *enemies.get(name);
        INFO("enemy: " << name);
        if (!def.pattern.empty()) {
            CHECK(patterns.get(def.pattern) != nullptr);
        }
    }
}

TEST_CASE("Every stage loads and places defined enemies", "[content]") {
    EnemyLibrary enemies;
    REQUIRE(enemies.load_json(read_json("assets/data/enemies.json"), "enemies.json"));

    const auto paths = stage_paths();
    REQUIRE_FALSE(paths.empty());

    for (const auto& path : paths) {
        INFO("stage file: " << path);
        const auto stage = read_json(path);

        StageLoader loader;
        REQUIRE(loader.load_file(SOURCE_DIR + path));
        const StageDef& loaded = *loader.get(0);

        // Entries without an enemy name are dropped at load
        REQUIRE(loaded.waves.size() == stage.at("waves").size());
        for (size_t w = 0; w < loaded.waves.size(); ++w) {
            INFO("wave: " << w);
            CHECK(loaded.waves[w].enemies.size() == stage.at("waves")[w].at("enemies").size());
            CHECK_FALSE(loaded.waves[w].enemies.empty());
            for (const auto& entry : loaded.waves[w].enemies) {
                INFO("enemy: " << entry.enemy);
                CHECK(enemies.get(entry.enemy) != nullptr);
            }
        }
    }
}

TEST_CASE("Every stage's level exists and can be played through", "[content]") {
    SoftwareRenderer sw;
    REQUIRE(sw.renderer != nullptr);

    StageLoader loader;
    const auto paths = stage_paths();
    for (const auto& path : paths) {
        REQUIRE(loader.load_file(SOURCE_DIR + path));
    }
    REQUIRE(loader.count() > 0);

    for (int i = 0; i < loader.count(); ++i) {
        const StageDef& stage = *loader.get(i);
        const StageDef* next = loader.get(i + 1);
        INFO("stage: " << stage.name << ", level: " << stage.level);

        Tilemap map;
        REQUIRE(map.load(sw.renderer, SOURCE_DIR + "assets/maps/raven.ldtk", stage.level));
        CHECK_FALSE(map.tiles().empty());
        CHECK_FALSE(map.textures().empty());

        const auto starts = map.find_all_spawns("PlayerStart");
        REQUIRE(starts.size() == 1);
        CHECK_FALSE(in_solid_cell(map, *starts[0]));

        // Enough spawn points for every spawn_index the stage uses
        const auto spawns = map.find_all_spawns("EnemySpawn");
        int max_index = 0;
        for (const auto& wave : stage.waves) {
            for (const auto& enemy : wave.enemies) {
                max_index = std::max(max_index, enemy.spawn_index);
            }
        }
        CHECK(spawns.size() > static_cast<size_t>(max_index));
        for (const auto* sp : spawns) {
            CHECK_FALSE(in_solid_cell(map, *sp));
        }

        // At least one reachable exit, pointing at the next stage's level
        const auto exits = map.find_all_spawns("Exit");
        REQUIRE_FALSE(exits.empty());
        for (const auto* exit : exits) {
            CHECK_FALSE(in_solid_cell(map, *exit));
            auto target = exit->fields.find("target_level");
            if (target != exit->fields.end() && !target->second.empty()) {
                REQUIRE(next != nullptr);
                CHECK(target->second == next->level);
            }
        }
    }
}

TEST_CASE("Every sprite sheet the code uses is registered and has its frames", "[content]") {
    const auto config = read_json("assets/data/config.json");

    struct SheetInfo {
        std::string path;
        int frame_w = 0;
        int frame_h = 0;
    };
    std::map<std::string, SheetInfo> registered;
    for (const auto& sheet : config.at("sprite_sheets")) {
        const auto id = sheet.at("id").get<std::string>();
        INFO("sheet: " << id);
        CHECK(registered.count(id) == 0); // ids are unique
        registered[id] = {sheet.at("path").get<std::string>(), sheet.at("frame_w").get<int>(),
                          sheet.at("frame_h").get<int>()};
        CHECK(source_file_exists(registered[id].path));
    }

    for (const char* id : sheets::ALL) {
        INFO("sheet id used in code: " << id);
        CHECK(registered.count(id) == 1);
    }

    // Enemy definitions name their sheets in data
    EnemyLibrary enemies;
    REQUIRE(enemies.load_json(read_json("assets/data/enemies.json"), "enemies.json"));
    for (const auto& name : enemies.names()) {
        INFO("enemy: " << name << ", sheet: " << enemies.get(name)->sheet);
        CHECK(registered.count(enemies.get(name)->sheet) == 1);
    }

    // Pattern bullets reference sheets from data
    StringInterner interner;
    const auto patterns = load_patterns(interner);
    for (const auto& name : patterns.names()) {
        for (const auto& emitter : patterns.get(name)->emitters) {
            const auto& sheet = interner.resolve(emitter.bullet_sheet);
            INFO("pattern: " << name << ", bullet_sheet: " << sheet);
            CHECK(registered.count(sheet) == 1);
        }
    }

    // Frames the code addresses by index must exist in the image
    const std::vector<std::pair<std::string, int>> frame_columns = {
        {sheets::PICKUPS, sheets::PICKUP_FRAME_WEAPON},
        {sheets::PICKUPS, sheets::PICKUP_FRAME_STABILIZER},
        {sheets::PROPS, sheets::PROP_FRAME_EXIT_CLOSED},
        {sheets::PROPS, sheets::PROP_FRAME_EXIT_OPEN},
    };
    for (const auto& [id, column] : frame_columns) {
        INFO("sheet: " << id << ", frame column: " << column);
        REQUIRE(registered.count(id) == 1);
        const auto& info = registered[id];
        SDL_Surface* image = IMG_Load((SOURCE_DIR + info.path).c_str());
        REQUIRE(image != nullptr);
        CHECK((column + 1) * info.frame_w <= image->w);
        CHECK(info.frame_h <= image->h);
        SDL_DestroySurface(image);
    }
}

TEST_CASE("Every animation export matches its sprite sheet", "[content]") {
    const auto config = read_json("assets/data/config.json");
    StringInterner interner;
    AnimationLibrary library;

    for (const auto& sheet : config.at("sprite_sheets")) {
        const auto id = sheet.at("id").get<std::string>();
        const auto animations = sheet.find("animations");
        if (animations == sheet.end()) {
            continue;
        }
        INFO("sheet: " << id);
        const auto data_path = animations->get<std::string>();
        REQUIRE(source_file_exists(data_path));

        const int fw = sheet.at("frame_w").get<int>();
        const int fh = sheet.at("frame_h").get<int>();
        const StringId sheet_id = interner.intern(id);
        REQUIRE(library.load_file(sheet_id, SOURCE_DIR + data_path, fw, fh, interner));
        REQUIRE(library.has_sheet(sheet_id));

        SDL_Surface* image = IMG_Load((SOURCE_DIR + sheet.at("path").get<std::string>()).c_str());
        REQUIRE(image != nullptr);
        const int image_w = image->w;
        const int image_h = image->h;
        SDL_DestroySurface(image);

        // A stale export (PNG re-exported without its JSON) shows up as a size mismatch
        const auto data = read_json(data_path);
        CHECK(data.at("meta").at("size").at("w").get<int>() == image_w);
        CHECK(data.at("meta").at("size").at("h").get<int>() == image_h);

        for (const auto& tag : data.at("meta").at("frameTags")) {
            const auto name = tag.at("name").get<std::string>();
            INFO("tag: " << name);
            const auto* clip = library.get(sheet_id, interner.intern(name));
            REQUIRE(clip != nullptr);
            for (const auto& frame : clip->frames) {
                CHECK((frame.frame_x + 1) * fw <= image_w);
                CHECK((frame.frame_y + 1) * fh <= image_h);
            }
        }
    }

    // Player characters need at least these to look alive
    for (const char* id : {sheets::PLAYER, sheets::KNIGHT}) {
        INFO("player sheet: " << id);
        CHECK(library.get(interner.intern(id), interner.intern(clips::IDLE)) != nullptr);
        CHECK(library.get(interner.intern(id), interner.intern(clips::WALK)) != nullptr);
    }
}

TEST_CASE("Every sound effect has a config entry and a file", "[content]") {
    const auto config = read_json("assets/data/config.json");
    const auto& sounds = config.at("sounds");

    for (int i = 0; i < static_cast<int>(Sfx::Count); ++i) {
        const std::string name = sfx_sound_name(static_cast<Sfx>(i));
        INFO("sound: " << name);
        REQUIRE(sounds.contains(name));
        REQUIRE(sounds.at(name).is_string());
        CHECK(source_file_exists(sounds.at(name).get<std::string>()));
    }
}

// ── LDtk import rules (the shipped map serves as the fixture) ───────

TEST_CASE("Hidden LDtk layers keep their collision and entities", "[tilemap]") {
    SoftwareRenderer sw;
    auto map = load_modified_test_room(sw.renderer, [](nlohmann::json& project) {
        layer_instance(project, "Test_Room", "Collision")["visible"] = false;
        layer_instance(project, "Test_Room", "Entities")["visible"] = false;
    });

    CHECK(map.is_cell_solid(0, 0)); // outer wall
    CHECK(map.find_spawn("PlayerStart") != nullptr);
    CHECK_FALSE(map.tiles().empty());
}

TEST_CASE("Hidden LDtk tile layers are not drawn", "[tilemap]") {
    SoftwareRenderer sw;
    auto map = load_modified_test_room(sw.renderer, [](nlohmann::json& project) {
        layer_instance(project, "Test_Room", "Tiles")["visible"] = false;
    });

    CHECK(map.tiles().empty());
    CHECK(map.is_cell_solid(0, 0));
}

TEST_CASE("Only the IntGrid layer named Collision is solid", "[tilemap]") {
    SoftwareRenderer sw;
    auto map = load_modified_test_room(sw.renderer, [](nlohmann::json& project) {
        // Rename the collision layer, as if it were a floor layer for auto-tiling
        for (auto& def : project.at("defs").at("layers")) {
            if (def.at("identifier") == Tilemap::COLLISION_LAYER) {
                def["identifier"] = "Floor";
            }
        }
        for (auto& lv : project.at("levels")) {
            for (auto& li : lv.at("layerInstances")) {
                if (li.at("__identifier") == Tilemap::COLLISION_LAYER) {
                    li["__identifier"] = "Floor";
                }
            }
        }
    });

    CHECK_FALSE(map.is_cell_solid(0, 0));
    CHECK(map.cell_size() == 16);
}
