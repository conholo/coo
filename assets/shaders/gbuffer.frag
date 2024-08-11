#version 450

layout (location = 0) in vec3 v_WorldPos;
layout (location = 1) in vec3 v_Color;
layout (location = 2) in vec3 v_Normal;
layout (location = 3) in vec3 v_Tangent;
layout (location = 4) in vec2 v_UV;

layout (location = 0) out vec4 o_Position;
layout (location = 1) out vec4 o_Normal;
layout (location = 2) out vec4 o_Albedo;

layout(set = 1, binding = 1) uniform sampler2D u_DiffuseMap;
layout(set = 1, binding = 2) uniform sampler2D u_NormalMap;

vec3 decode(vec3 c)
{
    return pow(c, vec3(2.2));
}

void main()
{
    o_Position = vec4(v_WorldPos, 1.0);

    vec3 N = normalize(v_Normal);
    vec3 T = normalize(v_Tangent);
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);

    vec3 tNormals = TBN * normalize(texture(u_NormalMap, v_UV).xyz * 2.0 - vec3(1.0));
    o_Normal = vec4(tNormals, 1.0);

    vec3 linearColor = decode(texture(u_DiffuseMap, v_UV).rgb);
    o_Albedo = vec4(linearColor, 1.0);
}