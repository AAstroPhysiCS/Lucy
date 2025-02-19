//type vertex
#version 450
#extension GL_ARB_shader_viewport_layer_array : require
#extension GL_EXT_multiview : require

#include "LucyIBLUtilities.glsl"

layout (location = 0) in vec3 a_Pos;

layout (location = 0) out vec3 a_PosOut;

layout (push_constant) uniform LucyCameraPushConstants {
    //proj will be imported as a uniform, since we want to control the "lense" of the camera in certain situations
    mat4 u_ProjMatrix;
};

void main() {
    a_PosOut = a_Pos;
    gl_Layer = gl_ViewIndex;
    gl_Position = u_ProjMatrix * inverse(CaptureViews[gl_ViewIndex]) * vec4(a_Pos, 1.0f);
}

//type fragment
#version 450

layout (location = 0) in vec3 a_Pos;

layout (location = 0) out vec4 a_LayeredColorAttachments; //6 layers

layout (set = 0, binding = 0) uniform sampler2D u_EquirectangularMap;

vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= vec2(0.1591, 0.3183);
    uv += 0.5f;
    return uv;
}

void main() {
    vec4 color = texture(u_EquirectangularMap, SampleSphericalMap(normalize(a_Pos)));
    a_LayeredColorAttachments = vec4(color.rgb, 1.0);
}