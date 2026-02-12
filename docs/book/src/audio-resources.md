# Audio Learning Resources

Curated resources for sound design and music composition, with emphasis on
styles relevant to Raven. See the [Audio Specification](audio-spec.md) for
technical requirements and the [Audio Integration Guide](audio-integration.md)
for engine details.

## Style References

Games whose audio directly inspires or informs Raven's sonic direction:

- **Blazing Beaks** — Electronic score by Ivo Sissolak, developed over the
  game's 3-year production. Tight, punchy SFX that match the fast-paced
  roguelike gameplay. The soundtrack balances tension with energy — good
  reference for how audio pacing aligns with room-clearing combat loops.
- **Celeste** — Composed by Lena Raine using NI MASSIVE synthesizer patches,
  piano, and FMOD for adaptive music triggers. The soundtrack shifts dynamically
  with gameplay intensity. Hybrid approach: retro-inflected melodies with modern
  production. Interviews:
  [Native Instruments blog](https://blog.native-instruments.com/lena-raine-celeste/),
  [Original Sound Version](https://www.originalsoundversion.com/).
- **Nuclear Throne** — Score by Jukio Kallio. Aggressive, distorted electronic
  music that escalates with the game's increasing chaos. Strong example of how
  music can mirror difficulty curves.
- **Enter the Gungeon** — Audio by doseone (Adam Drucker). Eclectic mix of
  electronic, hip-hop, and experimental sound design. Each weapon has a distinct
  audio identity — relevant given Raven's weapon variety.

## SFX Creation Tools

Free tools for generating and designing sound effects:

- **[ChipTone](https://sfbgames.itch.io/chiptone)** — Browser-based SFX
  generator with visual waveform editing. More control than sfxr with an
  intuitive interface. Good for both chiptune and stylized SFX.
- **[jsfxr](https://sfxr.me/)** — Browser port of sfxr. Randomize parameters for
  instant retro sound effects. Export as WAV. Fast for prototyping placeholder
  SFX.
- **[Bfxr](https://www.bfxr.net/)** — Extended version of sfxr with more
  waveforms and filters. Browser-based with save/load.
- **[Audacity](https://www.audacityteam.org/)** — Full audio editor, available
  in the Raven dev shell (`flake.nix`). Use for trimming, normalizing, format
  conversion, and batch processing.
- **[ffmpeg](https://ffmpeg.org/)** — Command-line audio/video tool, also in the
  dev shell. Useful for format conversion
  (`ffmpeg -i input.wav -c:a libvorbis output.ogg`).

## Music Composition Tools

### Trackers (Chiptune / Retro)

- **[FamiTracker](http://famitracker.com/)** — NES-style tracker for authentic
  8-bit music. Windows-native but runs under Wine.
- **[Furnace](https://github.com/tildearrow/furnace)** — Multi-system chiptune
  tracker supporting NES, SNES, Genesis, Game Boy, and more. Cross-platform,
  actively maintained. Good choice if targeting an authentic retro sound.

### DAWs (Modern / Hybrid)

- **[KiraStudio](https://kirastudio.app/)** — Free DAW with a retro chip focus,
  launched February 2026. Designed specifically for game audio with built-in
  chiptune synths and a streamlined workflow.
- **[REAPER](https://www.reaper.fm/)** — Full-featured DAW with a generous
  evaluation license and low indie price ($60). Industry-standard for game audio
  production.
- **[Ardour](https://ardour.org/)** — Free and open-source DAW. Capable but less
  polished than REAPER. Good option for zero-budget production.
- **[LMMS](https://lmms.io/)** — Free, open-source, cross-platform. Built-in
  synths and samplers. Lower learning curve than REAPER/Ardour.

## Middleware (Future Reference)

If Raven outgrows SDL_mixer (see
[ADR-0010](decisions/0010-sdl2-mixer-audio.md)), these middleware solutions
support adaptive audio:

- **[FMOD](https://www.fmod.com/)** — Free for indie projects under $200K
  revenue. FMOD Studio provides a visual event editor for adaptive music
  (layers, triggers, transitions). Used by Celeste, Hades, and many others.
- **[Wwise](https://www.audiokinetic.com/wwise/)** — Free for indie projects
  under $150K budget. More complex than FMOD but extremely powerful. Used by
  large studios.

## AI-Assisted Audio Tools

Useful for generating placeholder audio during prototyping. Not a substitute for
final sound design, but can accelerate early development:

- **[Wondera](https://wondera.ai/)** — AI music generation with game audio
  focus.
- **[Soundverse](https://soundverse.ai/)** — AI-powered sound design and music
  generation.
- **[TYGER AI](https://www.tyger.ai/)** — AI audio tools including SFX
  generation.

These tools can produce draft audio for playtesting while final assets are being
created.

## YouTube and Learning

- **Lena Raine interviews** — Deep dives into the Celeste soundtrack process,
  adaptive audio with FMOD, and hybrid chiptune/modern production. Search for
  her GDC talks and the Native Instruments feature.
- **GDC Audio talks** — The GDC YouTube channel has extensive game audio
  content. Search for "GDC game audio" for talks on SFX design, adaptive music,
  and middleware workflows.
- **[Game Audio with Scott Game Sounds](https://www.youtube.com/@GameAudioWithScott)**
  — Practical game audio tutorials covering FMOD, Wwise, and general sound
  design principles.
- **[Marshall McGee](https://www.youtube.com/@MarshallMcGee)** — Sound designer
  who has worked on Brawlhalla and other games. Breaks down SFX creation
  process.

## Tips for Raven

Practical pointers tying the resources back to the project's
[audio spec](audio-spec.md):

- Start with jsfxr or ChipTone to generate placeholder SFX for every event in
  the spec's category tables. Having _any_ audio is better than silence during
  playtesting.
- Use Audacity (already in the dev shell) to normalize all SFX to the target
  loudness range (-16 to -12 LUFS).
- Study Blazing Beaks for SFX pacing in a room-clearing roguelike and Celeste
  for how music reinforces emotional beats.
- Follow the naming conventions in the audio spec (`sfx_` / `mus_` prefixes)
  from the start to avoid renaming later.
- Export SFX as WAV and music as OGG — matching the formats SDL_mixer expects
  (see the [integration guide](audio-integration.md)).
- Keep a placeholder music loop (even a simple 4-bar chip loop) running during
  development. It dramatically changes how the game _feels_ during playtesting.
