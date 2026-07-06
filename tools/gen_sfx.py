#!/usr/bin/env python3
"""Generate placeholder sound effects for the SDL3 audio engine.

Synthesizes short retro-style effects (square waves, noise bursts, sweeps)
as 16-bit mono 22050 Hz WAVs into assets/audio/sfx/. Uses only the Python
standard library. Replace with designed sounds during the audio pass —
file names are the contract (they match the config.json "sounds" section).

Usage:
    python3 tools/gen_sfx.py [output_dir]
"""

import math
import os
import random
import struct
import sys
import wave

RATE = 22050
random.seed(0x5EED)  # deterministic output for reproducible builds


def envelope(t: float, duration: float, attack: float = 0.005) -> float:
    """Linear attack, exponential-ish decay to zero at the end."""
    if t < attack:
        return t / attack
    remaining = 1.0 - (t - attack) / max(duration - attack, 1e-6)
    return max(0.0, remaining) ** 1.5


def square(freq: float, t: float) -> float:
    return 1.0 if math.fmod(t * freq, 1.0) < 0.5 else -1.0


def render(duration: float, sample_fn) -> list[float]:
    n = int(duration * RATE)
    return [sample_fn(i / RATE) * envelope(i / RATE, duration) for i in range(n)]


def gen_shoot() -> list[float]:
    # Quick descending square blip
    return render(0.08, lambda t: 0.5 * square(880.0 - 4000.0 * t, t))


def gen_player_hit() -> list[float]:
    # Harsh low buzz + noise
    return render(0.18, lambda t: 0.45 * square(110.0, t) + 0.35 * random.uniform(-1, 1))


def gen_enemy_hit() -> list[float]:
    # Short mid thock
    return render(0.06, lambda t: 0.5 * square(330.0 - 1200.0 * t, t))


def gen_enemy_down() -> list[float]:
    # Descending sweep with noise tail
    return render(0.28, lambda t: 0.4 * square(520.0 - 1400.0 * t, t) +
                  0.25 * random.uniform(-1, 1) * min(1.0, t * 8))


def gen_pickup() -> list[float]:
    # Two ascending chirps
    def f(t: float) -> float:
        freq = 660.0 if t < 0.06 else 990.0
        return 0.45 * math.sin(2 * math.pi * freq * t)
    return render(0.14, f)


def gen_dash() -> list[float]:
    # Filtered noise whoosh (cheap lowpass: average of noise samples)
    state = {"prev": 0.0}

    def f(t: float) -> float:
        raw = random.uniform(-1, 1)
        state["prev"] = state["prev"] * 0.85 + raw * 0.15
        return 2.2 * state["prev"]
    return render(0.12, f)


def gen_melee() -> list[float]:
    # Fast low swipe
    return render(0.07, lambda t: 0.55 * square(220.0 + 900.0 * t, t))


SOUNDS = {
    "shoot": gen_shoot,
    "player_hit": gen_player_hit,
    "enemy_hit": gen_enemy_hit,
    "enemy_down": gen_enemy_down,
    "pickup": gen_pickup,
    "dash": gen_dash,
    "melee": gen_melee,
}


def write_wav(path: str, samples: list[float]) -> None:
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        frames = b"".join(
            struct.pack("<h", int(max(-1.0, min(1.0, s)) * 32767)) for s in samples
        )
        w.writeframes(frames)


def main() -> None:
    out_dir = sys.argv[1] if len(sys.argv) > 1 else "assets/audio/sfx"
    os.makedirs(out_dir, exist_ok=True)

    for name, gen in SOUNDS.items():
        path = os.path.join(out_dir, f"{name}.wav")
        samples = gen()
        write_wav(path, samples)
        print(f"Wrote {path} ({len(samples) / RATE:.2f}s)")


if __name__ == "__main__":
    main()
