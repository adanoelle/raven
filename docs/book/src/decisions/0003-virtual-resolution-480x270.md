# 3. Virtual Resolution 480x270

Date: 2026-02-08 Status: Accepted

## Context

Pixel art games need consistent pixel sizes across different display
resolutions. Rendering directly at the window resolution produces non-uniform
pixel scaling. We needed a virtual resolution that:

- Has a 16:9 aspect ratio (standard for modern displays and Switch)
- Is small enough for authentic pixel art aesthetics
- Scales to common resolutions by integer multiples

## Decision

Render to a 480x270 virtual target, then scale to the window size.

- **480x270** is exactly 1920/4 x 1080/4
- Integer scales: 2x = 960x540, 3x = 1440x810, 4x = 1920x1080 (native 1080p)
- 16:9 aspect ratio matches most monitors and the Nintendo Switch screen

Implementation: SDL2's render target system. The game draws to an `SDL_Texture`
at 480x270, then `SDL_RenderCopy` scales it to the window with nearest-neighbour
filtering for sharp pixels.

This replaced the original 384x448 resolution (roughly 7:8, a vertical aspect
ratio from the bullet hell genre).

## Consequences

**Positive:**

- Pixel-perfect rendering at 1080p (exact 4x scale)
- Standard 16:9 ratio works on all modern displays without letterboxing
- Small render target is trivially fast to fill — SDL2_Renderer can handle it
  easily
- Appropriate canvas size for roguelike rooms and UI

**Negative:**

- 480x270 is small — UI elements need careful sizing to remain readable
- Non-integer window sizes produce slight scaling artefacts (acceptable for
  windowed mode)
- The resolution change from 384x448 required adjusting all spawn positions and
  screen bounds
