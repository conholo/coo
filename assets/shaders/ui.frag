#version 450

layout (set = 0, binding = 0) uniform sampler2D u_FontSampler;

layout (location = 0) in vec2 v_UV;
layout (location = 1) in vec4 v_Color;

layout (location = 0) out vec4 o_Color;

void main()
{
    o_Color = v_Color * texture(u_FontSampler, v_UV);
}