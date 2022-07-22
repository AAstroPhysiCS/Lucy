//type vertex
#version 450

layout (location = 0) in vec3 a_Pos;
layout (location = 2) in vec4 a_ID;

layout(set = 0, binding = 0) uniform Camera {
	mat4 u_ViewMatrix;
	mat4 u_ProjMatrix;
};

layout (push_constant) uniform LocalPushConstant {
	mat4 u_ModelMatrix;
};

layout (location = 0) out vec4 o_ID;

void main() {
	o_ID = a_ID;
	gl_Position = u_ProjMatrix * u_ViewMatrix * u_ModelMatrix * vec4(a_Pos, 1.0f);
}

//type fragment
#version 450

layout (location = 0) out vec4 a_IDBuffer;

layout (location = 0) in vec4 o_ID;

void main() {
	a_IDBuffer = round(o_ID / 255 * 10e4) / 10e4;
	//we dont need to do anything with the depth
}