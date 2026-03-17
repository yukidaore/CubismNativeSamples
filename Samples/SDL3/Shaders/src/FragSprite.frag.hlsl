/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

cbuffer UBO : register(b0, space3)
{
    float4 u_baseColor;
};

Texture2D s_texture0 : register(t0, space2);
SamplerState s_sampler0 : register(s0, space2);

Texture2D s_texture1 : register(t1, space2);
SamplerState s_sampler1 : register(s1, space2);

struct PSInput
{
    float2 v_texCoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_Target0
{
    return s_texture0.Sample(s_sampler0, input.v_texCoord) * u_baseColor;
}
