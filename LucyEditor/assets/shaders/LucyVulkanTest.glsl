//type vertex
#version 460 core

layout(location = 0) in vec2 a_Pos;

layout(set = 0, binding = 0) uniform Camera {
	mat4 u_ModelMatrix;
	mat4 u_ViewMatrix;
	mat4 u_ProjMatrix;
};

/*

layout(set = 0, binding = 1) uniform Add {
	vec4 addition;
};

layout(set = 1, binding = 0) uniform Add2 {
	vec4 addition2;
};

layout(set = 1, binding = 1) uniform Add3 {
	vec4 addition3;
};

*/

void main() {
	gl_Position = u_ProjMatrix * u_ViewMatrix * u_ModelMatrix * vec4(a_Pos, 0.0, 1.0);
}

//type fragment
#version 460 core

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0f, 0.5f, 0.0f, 1.0f);
}