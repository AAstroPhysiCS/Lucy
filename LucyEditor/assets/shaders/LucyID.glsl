//type vertex
#version 460 core

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_ID;

uniform mat4 u_ModelMatrix;
uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjMatrix;

out vec3 o_ID;

void main() {
	o_ID = a_ID;
	gl_Position = u_ProjMatrix * u_ViewMatrix * u_ModelMatrix * vec4(a_Pos, 1.0f);
}

//type fragment
#version 460 core

layout (location = 0) out vec3 a_IDBuffer;
layout (location = 1) out vec3 a_Depth;

in vec3 o_ID;

void main() {
	a_IDBuffer = round(o_ID / 255 * 10e4) / 10e4;
	//we dont need to do anything with the depth
}