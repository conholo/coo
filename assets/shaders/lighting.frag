#version 450

layout(location = 0) in vec2 v_UV;
layout(location = 0) out vec4 o_FragColor;

layout(set = 1, binding = 0) uniform sampler2D u_Positions;
layout(set = 1, binding = 1) uniform sampler2D u_Normals;
layout(set = 1, binding = 2) uniform sampler2D u_Albedo;

#define DISPLAY_ALL 0
#define DISPLAY_COLOR 1
#define DISPLAY_POSITIONS 2
#define DISPLAY_NORMALS 3

layout(set = 0, binding = 0) uniform GlobalUBO
{
    mat4 Projection;
    mat4 View;
    mat4 InvView;
    mat4 InvProjection;
    vec4 CameraPosition;
} u_UBO;

layout(push_constant) uniform DebugDisplayPushConstants
{
    int DebugDisplayIndex;
} u_Debug;


void main()
{
    switch(u_Debug.DebugDisplayIndex)
    {
        case DISPLAY_ALL: break;
        case DISPLAY_COLOR:
        {
            o_FragColor = vec4(texture(u_Albedo, v_UV).rgb, 1.0);
            return;
        }
        case DISPLAY_POSITIONS:
        {
            o_FragColor = vec4(texture(u_Positions, v_UV).rgb, 1.0);
            return;
        }
        case DISPLAY_NORMALS:
        {
            o_FragColor = vec4(texture(u_Normals, v_UV).rgb, 1.0);
            return;
        }
    }

    const vec3 LightPosition = vec3(5.0, 0.0, 10.0);
    const vec3 LightColor = vec3(1.0, 1.0, 1.0);

    vec3 pixelColor = vec3(0.0);

    vec3 P = texture(u_Positions, v_UV).rgb;
    vec3 N = texture(u_Normals, v_UV).rgb;
    vec3 L = normalize(LightPosition - P);
    vec3 V = normalize(u_UBO.CameraPosition.xyz - P);

    vec3 albedo = texture(u_Albedo, v_UV).rgb;

    float kD = max(0.0, dot(N, L));
    vec3 diffuse = LightColor * kD;

    vec3 H = normalize(L + V);
    float kS = pow(max(0.0, dot(N, H)), 64.0);

    vec3 specular = 0.5 * LightColor * kS;
    pixelColor = diffuse * albedo + specular * albedo;

    o_FragColor = vec4(pixelColor, 1.0);
}