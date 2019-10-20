#include "RootSig.hlsli"

[RootSignature(RootSig)]
[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
}