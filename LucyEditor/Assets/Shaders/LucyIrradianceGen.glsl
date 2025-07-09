//type vertex
#version 450
#extension GL_ARB_shader_viewport_layer_array : require
#extension GL_EXT_multiview : require

#include "LucyIBLUtilities"

layout (location = 0) in vec3 a_Pos;

layout (location = 0) out vec3 a_PosOut;

layout (push_constant) uniform LucyCameraPushConstants {
    //proj will be imported as a uniform, since we want to control the "lense" of the camera in certain situations
    mat4 u_ProjMatrix;
};

void main() {
	a_PosOut = a_Pos;
    gl_Position = u_ProjMatrix * inverse(CaptureViews[gl_ViewIndex]) * vec4(a_Pos, 1.0f);
}

//type fragment
#version 450

#include "LucySamplingUtilities"

layout (location = 0) in vec3 a_Pos;

layout (location = 0) out vec4 a_LayeredColorAttachments; //6 layers

layout (set = 0, binding = 0) uniform samplerCube u_EnvironmentMap;

void main() {
	vec3 normalVector = normalize(a_Pos);
	vec3 upVector = vec3(0.0f, 1.0f, 0.0f);
	vec3 rightVector = normalize(cross(upVector, normalVector));
	upVector = cross(normalVector, rightVector);

	vec3 irradianceColor = vec3(0.0f);

	const float iterDeltaPhi = TWO_PI / 360.0;
	const float iterDeltaTheta = HALF_PI / 90.0;

	for (float phi = 0.0f; phi < TWO_PI; phi += iterDeltaPhi) {
        float sinPhi = sin(phi);
        float cosPhi = cos(phi);

		for (float theta = 0.0f; theta < HALF_PI; theta += iterDeltaTheta) {
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            vec3 sphereCoord = vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
			vec3 sampleVector = sphereCoord.x * rightVector + sphereCoord.y * upVector + sphereCoord.z * normalVector;

            irradianceColor += texture(u_EnvironmentMap, sampleVector).rgb * cosTheta * sinTheta;
		}
	}

	float totalSampleCount = (TWO_PI / iterDeltaPhi) * (HALF_PI / iterDeltaTheta);

    irradianceColor *= PI * (1.0f / totalSampleCount);
	a_LayeredColorAttachments = vec4(irradianceColor, 1.0f);
}