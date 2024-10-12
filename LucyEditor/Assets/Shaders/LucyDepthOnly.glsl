//type vertex
#version 450

#extension GL_ARB_shader_viewport_layer_array : require
#extension GL_NV_viewport_array2 : enable
#extension GL_EXT_multiview : enable

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_ID;
layout (location = 2) in vec2 a_TextureCoords;
layout (location = 3) in vec3 a_Normals;
layout (location = 4) in vec3 a_Tangents;
layout (location = 5) in vec3 a_Bitangents;

#define NUM_CASCADES 4

struct CameraViewProjection {
	mat4 ViewMatrix;
	mat4 ProjMatrix;
	vec4 CamPos;
};

layout(set = 0, binding = 0) uniform LucyCamera {
	CameraViewProjection u_ShadowCameraVPs[NUM_CASCADES];
};

layout (push_constant) uniform LocalPushConstant {
	mat4 u_ModelMatrix;
};

void main() {
	gl_Layer = gl_ViewIndex;
	gl_Position = u_ShadowCameraVPs[gl_ViewIndex].ProjMatrix * u_ShadowCameraVPs[gl_ViewIndex].ViewMatrix * u_ModelMatrix * vec4(a_Pos, 1.0f);
}

//type fragment
#version 450

void main() {

}