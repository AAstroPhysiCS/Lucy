//type vertex
#version 450

layout (location = 0) in vec3 a_Pos;

layout (location = 0) out vec3 a_PosOut;

layout (set = 0, binding = 0) uniform LucyCamera {
	mat4 u_ViewMatrix;
	mat4 u_ProjMatrix;
	vec3 u_CamPos;
};

void main() {
	a_PosOut = a_Pos;
	a_PosOut.xy *= -1.0f;
	gl_Position = (u_ProjMatrix * mat4(mat3(u_ViewMatrix)) * vec4(a_Pos, 1.0f)).xyww;
}

//type fragment
#version 450

layout (location = 0) out vec4 a_Color;
layout (location = 0) in vec3 a_Pos;

layout (set = 0, binding = 1) uniform samplerCube u_EnvironmentMap;

void main() {
	vec3 envColor = texture(u_EnvironmentMap, a_Pos).rgb;

	//TODO: Make specialization constant for gamma
	const float gamma = 2.2f;
    envColor = pow(envColor, vec3(1.0/gamma));

	a_Color = vec4(envColor, 1.0f);
}