//--------------------------------------------------------------------------------------
// SkyboxEffect_VS.hlsl
//
// A sky box effect for DirectX 11.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "SkyboxEffect_Common.hlsli"

VSOutput main(float4 position : SV_Position)
{
    VSOutput vout;

    vout.PositionPS = mul(position, WorldViewProj);
    vout.PositionPS.z = vout.PositionPS.w; // Draw on far plane
    
#if 0
    vout.TexCoord.x = -position.x;
    vout.TexCoord.yz = position.yz;
#else
    vout.TexCoord = position.xyz;
#endif

    return vout;
}
