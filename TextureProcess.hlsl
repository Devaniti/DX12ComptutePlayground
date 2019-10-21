#include "RootSig.hlsli"

RWTexture2D<unorm float4> inputTexture : register(u0);
RWTexture2D<unorm float4> outputTexture : register(u1);

[RootSignature(RootSig)]
[numthreads(1, 1, 1)]
void main( uint3 threadID : SV_DispatchThreadID )
{
    outputTexture[threadID.xy] = 2 * inputTexture[threadID.xy];
}