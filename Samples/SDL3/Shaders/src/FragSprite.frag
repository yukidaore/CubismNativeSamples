/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

// SDL3 GPU API requires specific descriptor set layout for SPIR-V:
//   Fragment samplers:        set = 2
//   Fragment uniform buffers: set = 3

#version 460

layout(set = 3, binding = 0) uniform UBO
{
    vec4 u_baseColor;
}ubo;

layout(set = 2, binding = 0) uniform sampler2D s_texture0;
layout(set = 2, binding = 1) uniform sampler2D s_texture1;

layout(location = 0) in vec2 v_texCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 color = texture(s_texture0, v_texCoord) * ubo.u_baseColor;
    outColor = color;
}
