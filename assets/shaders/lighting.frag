#version 450

layout(location = 0) in vec2 v_UV;
layout(location = 0) out vec4 o_Color;

const vec3 LightPosition = vec3(5.0, 10.0, 0.0);
const vec3 LightColor = vec3(1.0, 1.0, 1.0);

layout(set = 1, binding = 0) uniform sampler2D u_Positions;
layout(set = 1, binding = 1) uniform sampler2D u_Normals;
layout(set = 1, binding = 2) uniform sampler2D u_Albedo;

layout(set = 0, binding = 0) uniform GlobalUBO
{
    mat4 Projection;
    mat4 View;
    mat4 InvView;
} u_UBO;


void main()
{
    vec3 pixelColor = vec3(0.0);

    vec3 worldPosition = texture(u_Positions, v_UV).rgb;
    vec3 N = texture(u_Normals, v_UV).rgb * 2.0 - 1.0;
    vec3 L = normalize(LightPosition - worldPosition);
    vec3 V = normalize(u_UBO.InvView[3].xyz - worldPosition);

    vec3 albedo = texture(u_Albedo, v_UV).rgb;

    float kD = max(0.0, dot(N, L));
    vec3 diffuse = LightColor * kD;

    vec3 H = normalize(V + L);
    float kS = pow(max(0.0, dot(N, H)), 64.0);

    vec3 specular = LightColor * kS;
    pixelColor = diffuse * albedo + specular * albedo;

    o_Color = vec4(pixelColor, 1.0);
}