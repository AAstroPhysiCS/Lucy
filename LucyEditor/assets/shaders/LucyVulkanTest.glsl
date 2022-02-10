//type vertex
#version 460 core

layout(location = 0) in vec2 a_Pos;

layout(set = 0, binding = 0) uniform Camera {
	mat4 u_ViewMatrix;
	mat4 u_ProjMatrix;
	mat4 u_ModelMatrix;
};

layout(set = 0, binding = 1) uniform Test {
	mat4 u_Test;
};

void main() {
    gl_Position = u_ProjMatrix * u_ViewMatrix * u_ModelMatrix * u_Test * vec4(a_Pos.x, a_Pos.y, 0.0, 1.0);
}

//type fragment
#version 460 core

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D u_Texture;
layout(set = 1, binding = 1) uniform sampler2D u_TextureTest;

void main() {
	float r = texture(u_Texture, vec2(1.0f, 1.0f)).r;
	float rTest = texture(u_TextureTest, vec2(1.0f, 1.0f)).r;
    outColor = vec4(r * rTest, 0.5f, 0.0f, 1.0f);
}