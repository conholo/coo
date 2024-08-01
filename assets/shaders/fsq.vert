#version 450

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Color;
layout (location = 2) in vec3 a_Normal;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec2 a_UV;

layout(location = 0) out vec2 v_UV;

void main()
{
    v_UV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(v_UV * 2.0f - 1.0f, 0.0f, 1.0f);
}
