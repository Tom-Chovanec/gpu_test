struct Input
{
    float3 Position : TEXCOORD0;
};

struct Output
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
};

Output main(Input input)
{
    Output output;
    output.Position = float4(input.Position, 1.0f);
    output.UV = input.Position.xy * 0.5f + 0.5f; // Normalize to [0, 1]
    return output;
}
