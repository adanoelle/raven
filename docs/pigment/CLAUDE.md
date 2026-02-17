# Pigment

Pixel art palette tool. Tauri v2, React + TypeScript, FettePalette, Rust backend.
Companion tool to Aseprite for hue-shifted color ramp generation.

## Build (requires nix develop / direnv)

just dev       # Start Tauri dev server with hot-reload
just build     # Production build
just test      # Run tests
just fmt       # Format all source (Rust + TypeScript)
just lint      # Lint all source (clippy + eslint)

## Layout

src/                     React + TypeScript frontend
src/components/ui/       Pixel-art styled base components (Panel, Slider, Button)
src/components/          Feature components (RampList, RampEditor, SwatchStrip)
src/engine/              FettePalette wrapper, ramp linking, color math
src/store/               Zustand state management
src/styles/              Pixel theme CSS, bitmap font setup
src-tauri/src/           Rust backend (Tauri commands, file export)
src-tauri/src/palette/   .gpl and .pal format serializers

## Tech Stack

- App framework: Tauri v2 (Rust backend + WebView frontend)
- Frontend: React + TypeScript
- Color engine: FettePalette (MIT, npm: fettepalette)
- State: Zustand
- Styling: Tailwind CSS + custom pixel-art theme
- Font: Press Start 2P (bitmap pixel font)
- Canvas: HTML Canvas (gradients/curves) + SVG (swatches)
- Backend: Rust (file I/O, palette format serialization)

## Conventions

### TypeScript (Frontend)
- Functional components with hooks
- PascalCase component names, camelCase functions/variables
- Named exports (no default exports)
- Strict TypeScript — no `any`
- Co-locate component styles with components when possible

### Rust (Backend)
- snake_case functions/variables, PascalCase types
- Tauri commands in commands.rs, decorated with #[tauri::command]
- Palette serializers in palette/ module
- Use serde for JSON project save/load
- Handle all errors with Result, no unwrap() in production code

### Styling
- Aseprite-like aesthetic: grey beveled panels, pixel-perfect borders
- All elements aligned to integer pixel positions
- `image-rendering: pixelated` on all visual elements
- `-webkit-font-smoothing: none` for crisp bitmap fonts
- Press Start 2P at 8px or 16px multiples only
- No rounded corners, no anti-aliasing, no gradients on UI chrome
- Hard pixel shadows via box-shadow with 0 blur

## Architecture

### Frontend (React + TypeScript)
- FettePalette runs entirely in the frontend (pure math, no DOM)
- Each ramp is a set of FettePalette parameters + generated colors
- Zustand store holds all ramps, links, and UI state
- Ramp linking is post-processing: merge/average dark-end swatches
- Canvas renders curve visualizations; SVG renders swatch grids

### Backend (Rust)
- Tauri commands for file operations only:
  - export_gpl: serialize palette to .gpl format, write to disk
  - export_pal: serialize palette to JASC .pal format, write to disk
  - save_project: serialize project state to JSON, write to disk
  - load_project: read JSON from disk, return to frontend
- No color math in Rust — FettePalette handles generation in JS

### Data Flow
1. User adjusts slider → React state updates → FettePalette regenerates ramp
2. Ramp colors flow to SwatchStrip, CurveEditor, PreviewBar
3. On export: frontend sends RGB arrays to Rust via invoke()
4. Rust serializes to .gpl/.pal format and writes to disk

## Key Dependencies

- fettepalette (npm): Color ramp generation engine
- @tauri-apps/api: Frontend-to-backend communication
- zustand: State management
- tailwindcss: Layout and utility classes
- serde + serde_json (Rust): Project serialization

## Commits

- Conventional commits (feat:, fix:, refactor:, test:, docs:, chore:)
- Do not mention Claude or AI in commit messages
- Run fmt before committing

## Export Formats

### .gpl (GIMP Palette)
Text format. Header + RGB values + optional color names.
Supported by: Aseprite, GIMP, Krita, Inkscape.

### JASC .pal
Text format. Header + color count + RGB values.
Supported by: Aseprite, Paint Shop Pro, many editors.

## Product Context

Pigment is being built for the Raven roguelike's palette workflow, with the
intent to release as a standalone paid product on itch.io ($5-$15).

Target audience: pixel artists using Aseprite who need structured, hue-shifted
color ramp generation with ramp linking and direct .gpl/.pal export.

Positioning: "The palette tool built for pixel artists."

Repository: github.com/adanoelle/pigment (private)
