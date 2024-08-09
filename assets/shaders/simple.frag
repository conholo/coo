#version 450

layout(location = 0) in vec2 v_UV;
layout(location = 0) out vec4 o_FragColor;

layout(set = 0, binding = 0) uniform sampler2D u_TextureA;
layout(set = 0, binding = 1) uniform sampler2D u_TextureB;

void main()
{
    vec4 a = texture(u_TextureA, v_UV);
    vec4 b = texture(u_TextureB, v_UV);
    o_FragColor = vec4(a.rgb * b.rgb, 1.0);
}