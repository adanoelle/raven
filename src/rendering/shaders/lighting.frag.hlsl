// Lighting pass for the shader lighting prototype (ADR-0028).
//
// Input texture: the world as the game drew it, in ordinary colour.
// u_reverse: colour to (ramp, step). A 64x64x64 colour cube as 8x8 slices;
//   R = ramp, G = step, A = 1 where the colour is known.
// u_ramps: one texel per (step, ramp). The ramps appear twice: rows
//   0..ramps-1 as they are, then the same ramps under the cool light.
// u_light: the light buffer, the size of the frame. Red is the light at
//   that pixel in sixteenths of a step, summed over the lights stamped
//   into it (light.frag.hlsl). Green is the part of it that is cool light.
//
// A known colour is moved along its ramp by the ambient shift plus the
// light at that pixel. An unknown colour is left as drawn. All arithmetic
// that decides a colour uses whole numbers.

cbuffer Context : register(b0, space3) {
    float4 frame; // target w, target h, ambient shift (steps), step count
    float4 extra; // x: dither band half-width, sixteenths of a step (0..8)
                  // w: most light one pixel takes, in sixteenths of a step
    float4 tint;  // x: number of ramps (rows in one layer of u_ramps)
                  // y: cool light for a full tint, sixteenths of a step; 0 is no tint
};

Texture2D u_texture : register(t0, space2);
SamplerState u_sampler : register(s0, space2);
Texture2D u_reverse : register(t1, space2);
SamplerState u_reverse_sampler : register(s1, space2);
Texture2D u_ramps : register(t2, space2);
SamplerState u_ramps_sampler : register(s2, space2);
Texture2D u_light : register(t3, space2);
SamplerState u_light_sampler : register(s3, space2);

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
    // The frame's colour modulation, white unless someone sets it. It must
    // be used: an input the shader ignores is dropped when the shader is
    // translated for Direct3D, the inputs left no longer line up with what
    // the vertex shader passes, and v_uv arrives holding the colour.
    float4 mod = float4(input.v_color.rgb, 1.0);
    output.o_color = float4(drawn.rgb, 1.0) * mod;

    // Which ramp and step is this colour?
    int3 c = int3(round(drawn.rgb * 255.0)) >> 2;
    int2 cell = int2(c.r + (c.b & 7) * 64, c.g + (c.b >> 3) * 64);
    float4 entry = u_reverse.Load(int3(cell, 0));
    if (entry.a < 0.5) {
        return output; // not in the palette: bullets, HUD, outlines
    }
    int ramp = int(round(entry.r * 255.0));
    int step_in = int(round(entry.g * 255.0));

    // The light at this pixel, summed over every light stamped into the
    // light buffer. It is in sixteenths of a step: the whole part shifts
    // the step and the remainder is dithered.
    float4 light = u_light.Load(int3(px, 0));
    int level16 = int(round(light.r * 255.0));
    int cool16 = int(round(light.g * 255.0));

    // Where many lights overlap, stop short of the top of the ramp
    level16 = min(level16, int(extra.w));

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
    // Cool light picks the tinted layer. How much of it there is decides
    // how many pixels take the tint, dithered on a pattern offset from the
    // one above so the two do not line up.
    int row = ramp;
    int full = int(tint.y);
    if (cool16 > 0 && full > 0) {
        float cool_threshold = bayer[((px.y + 2) & 3) * 4 + ((px.x + 1) & 3)];
        if (float(min(cool16 * 16 / full, 16)) > cool_threshold) {
            row += int(tint.x);
        }
    }
    if (shift == 0 && row == ramp) {
        return output; // nothing acts on this pixel: the art as drawn
    }

    int step_out = clamp(step_in + shift, 0, int(frame.w) - 1);
    float4 colour = u_ramps.Load(int3(step_out, row, 0));
    output.o_color = float4(colour.rgb, 1.0) * mod;
    return output;
}
