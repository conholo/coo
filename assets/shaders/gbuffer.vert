#version 450

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Color;
layout (location = 2) in vec3 a_Normal;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec2 a_UV;

layout (location = 0) out vec3 v_WorldPos;
layout (location = 1) out vec3 v_Color;
layout (location = 2) out vec3 v_Normal;
layout (location = 3) out vec3 v_Tangent;
layout (location = 4) out vec2 v_UV;

layout(set = 0, binding = 0) uniform GlobalUBO
{
    mat4 Projection;
    mat4 View;
    mat4 InvView;
    PointLight PointLights[10];
    int NumPointLights;
} u_UBO;

layout(set = 1, binding = 0) uniform GameObjectBufferData
{
    mat4 ModelMatrix;
    mat4 NormalMatrix;
} u_GameObject;

void main()
{
    v_UV = a_UV;
    v_WorldPos = mat3(u_GameObject.ModelMatrix) * a_Position;

    v_Normal = mat3(u_GameObject.NormalMatrix) * normalize(a_Normal);
    v_Tangent = mat3(u_GameObject.NormalMatrix) * normalize(a_Tangent);
    v_Color = a_Color;

    gl_Position = u_UBO.Projection * u_UBO.View * u_GameObject.ModelMatrix * vec4(a_Position, 1.0);
}