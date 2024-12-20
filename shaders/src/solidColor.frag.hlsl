cbuffer TimeBuffer : register(b0)
{
    float time : packoffset(c0);  // Time in seconds
};

float4 main(float2 UV : TEXCOORD0) : SV_Target0
{
    // Combine UV with Time for dynamic gradient effect
    float r = UV.x + 0.5f * sin(time);
    float g = UV.y + 0.5f * cos(time);
    float b = 0.5f + 0.5f * sin(UV.x * 10.0f + time);

    return float4(r, g, b, 1.0f); // Ensure values are clamped in [0, 1]
}
