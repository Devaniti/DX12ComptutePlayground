#include "RootSig.hlsli"

RWTexture2D<unorm float4> inputTexture : register(u0);
RWTexture2D<unorm float4> outputTexture : register(u1);

float4 Noise4D(float2 seed)
{
    float4 noise = seed.x;
    float4 noiseTmp = seed.y;
    noise.x = (frac(sin(dot(seed, float2(25.9796, 15.6466))) * 43758.5453));
    noise.y = (frac(sin(dot(seed, float2(92.4651, 43.3390))) * 71463.0813));
    noise.z = (frac(sin(dot(seed, float2(39.1734, 55.9543))) * 24515.6764));
    noise.w = (frac(sin(dot(seed, float2(29.7840, 88.7433))) * 88721.9240));
    return abs(noise);
}

float Noise1D(float2 seed)
{
    return Noise4D(seed).x;
}

float2 Noise2D(float2 seed)
{
    return Noise4D(seed).xy;
}

float3 Noise3D(float2 seed)
{
    return Noise4D(seed).xyz;
}

[RootSignature(RootSig)]
[numthreads(1, 1, 1)]
void main( uint3 threadID : SV_DispatchThreadID )
{
    uint2 uv = threadID.xy;
    uint2 offset = 0;
    uint2 loadLocation = clamp(uv + offset, 0, uint2(1919,1079));

    float noise = (Noise1D(uv) - 0.5) * 0.25;

    outputTexture[threadID.xy] = inputTexture[loadLocation] + float4(noise.xxx, 0);
}