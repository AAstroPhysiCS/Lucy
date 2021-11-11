//type vertex
#version 460 core

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in float a_ID;
layout (location = 2) in vec2 a_TextureCoords;
layout (location = 3) in vec3 a_Normals;
layout (location = 4) in vec3 a_Tangents;
layout (location = 5) in vec3 a_BiTangents;

uniform mat4 u_ModelMatrix;
uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjMatrix;

void main() {
	gl_Position = u_ProjMatrix * u_ViewMatrix * u_ModelMatrix * vec4(a_Pos, 1.0f);
}

//type fragment
#version 460 core

layout (location = 0) out vec4 a_Color;

void main() {
	a_Color = vec4(0.5f);
}