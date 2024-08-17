#version 450

layout (location = 0) in vec2 a_Pos;
layout (location = 1) in vec2 a_UV;
layout (location = 2) in vec4 a_Color;

layout (push_constant) uniform PushConstants
{
    vec2 Scale;
    vec2 Translate;
} p_Transform;

layout (location = 0) out vec2 v_UV;
layout (location = 1) out vec4 v_Color;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    v_UV = a_UV;
    v_Color = a_Color;
    gl_Position = vec4(a_Pos * p_Transform.Scale + p_Transform.Translate, 0.0, 1.0);
}