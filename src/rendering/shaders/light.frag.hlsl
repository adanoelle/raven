// One light, stamped into the light buffer of the shader lighting
// prototype (ADR-0028).
//
// Drawn as a square over the light's reach with additive blending. The
// colour modulation carries the light's strength at its centre: red is
// sixteenths of a ramp step, over 255. The falloff is (1 - (d/r)^2)^2,
// the same curve as the loop in lighting.frag.hlsl.

Texture2D u_texture : register(t0, space2);
SamplerState u_sampler : register(s0, space2);

struct PSInput {
    float4 v_color : COLOR0;
    float2 v_uv : TEXCOORD0;
};

struct PSOutput {
    float4 o_color : SV_Target;
};

PSOutput main(PSInput input) {
    PSOutput output;
    float2 d = input.v_uv * 2.0 - 1.0;
    float t = max(1.0 - dot(d, d), 0.0);
    float white = u_texture.Sample(u_sampler, input.v_uv).a;
    output.o_color = float4(input.v_color.rgb * (t * t * white), 1.0);
    return output;
}
