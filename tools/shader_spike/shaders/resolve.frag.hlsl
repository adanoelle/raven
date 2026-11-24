// Palette resolve pass for the ADR-0028 spike.
//
// Input texture: the index target. R = ramp, G = step, B = flags
// (bit 0: unlit), each stored as a whole byte.
// Lookup table: one texel per (step, variant * ramp_count + ramp).
//
// All arithmetic that decides a colour uses whole numbers held in
// floats, so the CPU reference in main.cpp can match it exactly.

cbuffer Context : register(b0, space3) {
    float4 size_variants; // target w, target h, variant A, variant B
    float4 light;         // x, y, inner radius, outer radius (target pixels)
    float4 params;        // mix (0..16), ambient shift, ramp count, step count
};

Texture2D u_texture : register(t0, space2);
SamplerState u_sampler : register(s0, space2);
Texture2D u_lut : register(t1, space2);
SamplerState u_lut_sampler : register(s1, space2);

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

    int2 px = int2(floor(input.v_uv * size_variants.xy));
    float4 index = u_texture.Load(int3(px, 0));

    float ramp = round(index.r * 255.0);
    float step_in = round(index.g * 255.0);
    float flags = round(index.b * 255.0);
    bool unlit = fmod(flags, 2.0) >= 1.0;

    float threshold = bayer[(px.y & 3) * 4 + (px.x & 3)];

    // Light: +2 steps inside the inner radius, +1 inside the outer, and a
    // dithered half-step in the band just outside each.
    float2 d = (float2(px) + 0.5) - light.xy;
    float d2 = dot(d, d);
    float inner2 = light.z * light.z;
    float outer2 = light.w * light.w;
    float inner_band2 = (light.z + 4.0) * (light.z + 4.0);
    float outer_band2 = (light.w + 4.0) * (light.w + 4.0);

    float shift = params.y;
    if (light.w > 0.0) {
        if (d2 < inner2) {
            shift += 2.0;
        } else if (d2 < inner_band2) {
            shift += (threshold < 8.0) ? 2.0 : 1.0;
        } else if (d2 < outer2) {
            shift += 1.0;
        } else if (d2 < outer_band2) {
            shift += (threshold < 8.0) ? 1.0 : 0.0;
        }
    }
    if (unlit) {
        shift = 0.0;
    }

    float step_out = clamp(step_in + shift, 0.0, params.w - 1.0);

    // Cross-fade by dither: each pixel is wholly variant A or variant B,
    // so the frame never holds a colour that is in neither palette.
    float variant = (params.x > threshold) ? size_variants.w : size_variants.z;

    int2 lut_px = int2(int(step_out), int(variant * params.z + ramp));
    float4 colour = u_lut.Load(int3(lut_px, 0));

    output.o_color = float4(colour.rgb, 1.0);
    return output;
}
