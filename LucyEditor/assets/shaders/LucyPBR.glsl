//type vertex
#version 460

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec2 a_TextureCoords;
layout (location = 2) in vec3 a_Normals;
layout (location = 3) in vec3 a_Tangents;
layout (location = 4) in vec3 a_BiTangents;

uniform mat4 u_ModelMatrix;
uniform mat4 u_ViewMatrix;
uniform mat4 u_Proj;

void main() {
	gl_Position = u_ModelMatrix * u_ViewMatrix * u_Proj * vec4(a_Pos, 1.0f); 	
}

//type fragment
#version 460

layout (location = 0) out vec4 a_Color;

void main() {
	a_Color = vec4(1.0f);
}