// Lighting pass for the shader lighting prototype (ADR-0028).
//
// Input texture: the world as the game drew it, in ordinary colour.
// u_reverse: colour to (ramp, step). A 64x64x64 colour cube as 8x8 slices;
//   R = ramp, G = step, A = 1 where the colour is known.
// u_ramps: one texel per (step, ramp).
//
// A known colour is moved along its ramp by the ambient shift plus the
// light at that pixel. An unknown colour is left as drawn. All arithmetic
// that decides a colour uses whole numbers.

#define MAX_LIGHTS 32

cbuffer Context : register(b0, space3) {
    float4 frame; // target w, target h, ambient shift (steps), step count
    float4 extra; // x: dither band half-width, sixteenths of a step (0..8)
                  // y: number of lights in use
    float4 lights[MAX_LIGHTS]; // each: x, y, radius (whole target pixels), zones
};

Texture2D u_texture : register(t0, space2);
SamplerState u_sampler : register(s0, space2);
Texture2D u_reverse : register(t1, space2);
SamplerState u_reverse_sampler : register(s1, space2);
Texture2D u_ramps : register(t2, space2);
SamplerState u_ramps_sampler : register(s2, space2);

struct PSInput {
    float4 v_color : COLOR0;
    float2 v_uv : TEXCOORD0;
};

struct PSOutput {
    float4 o_color : SV_Target;
};

// 4x4 ordered dither, values 0..15.
static const float bayer[16] = {
     0.0,  8.0,  2.0, 10.0,
    12.0,  4.0, 14.0,  6.0,
     3.0, 11.0,  1.0,  9.0,
    15.0,  7.0, 13.0,  5.0
};

PSOutput main(PSInput input) {
    PSOutput output;

    int2 px = int2(floor(input.v_uv * frame.xy));
    float4 drawn = u_texture.Load(int3(px, 0));
    output.o_color = float4(drawn.rgb, 1.0);

    // Which ramp and step is this colour?
    int3 c = int3(round(drawn.rgb * 255.0)) >> 2;
    int2 cell = int2(c.r + (c.b & 7) * 64, c.g + (c.b >> 3) * 64);
    float4 entry = u_reverse.Load(int3(cell, 0));
    if (entry.a < 0.5) {
        return output; // not in the palette: bullets, HUD, outlines
    }
    int ramp = int(round(entry.r * 255.0));
    int step_in = int(round(entry.g * 255.0));

    // Each light is a smooth falloff, (1 - (d/r)^2)^2, scaled to `zones`
    // steps at its centre, and the lights add. The sum is in sixteenths of
    // a step: the whole part shifts the step and the remainder is dithered.
    int level16 = 0;
    int count = min(int(extra.y), MAX_LIGHTS);
    for (int i = 0; i < count; ++i) {
        int radius = int(lights[i].z);
        int zones = int(lights[i].w);
        if (radius > 0 && zones > 0) {
            int2 dd = px * 2 + 1 - int2(lights[i].xy) * 2;
            int d2x4 = dd.x * dd.x + dd.y * dd.y;
            int r2x4 = 4 * radius * radius;
            if (d2x4 < r2x4) {
                int t = ((r2x4 - d2x4) * 256) / r2x4;
                level16 += (zones * 16 * t * t) >> 16;
            }
        }
    }

    int shift = int(frame.z);
    if (level16 > 0) {
        float threshold = bayer[(px.y & 3) * 4 + (px.x & 3)];
        int band = int(extra.x);
        if (band <= 0) {
            shift += (level16 + 8) >> 4;
        } else {
            int f = clamp((((level16 & 15) - (8 - band)) * 16) / (2 * band), 0, 16);
            shift += level16 >> 4;
            shift += (float(f) > threshold) ? 1 : 0;
        }
    }
    if (shift == 0) {
        return output; // nothing acts on this pixel: the art as drawn
    }

    int step_out = clamp(step_in + shift, 0, int(frame.w) - 1);
    float4 colour = u_ramps.Load(int3(step_out, ramp, 0));
    output.o_color = float4(colour.rgb, 1.0);
    return output;
}
