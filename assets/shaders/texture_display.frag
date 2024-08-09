#version 450

layout (location = 0) in vec2 v_UV;
layout (location = 0) out vec4 o_FragColor;

layout (set = 0, binding = 0) uniform sampler2D u_Texture;

void main()
{
    o_FragColor = texture(u_Texture, v_UV);
}