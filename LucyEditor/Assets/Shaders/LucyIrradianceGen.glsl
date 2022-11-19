//type vertex
#version 450
#extension GL_ARB_shader_viewport_layer_array : require
#extension GL_EXT_multiview : require

layout (location = 0) in vec3 a_Pos;

layout (location = 0) out vec3 a_PosOut;

layout (push_constant) uniform LucyCameraPushConstants {
    //proj will be imported as a uniform, since we want to control the "lense" of the camera in certain situations
    mat4 u_ProjMatrix;
};

mat4 LookAt(vec3 eye, vec3 at, vec3 up) {
  vec3 zaxis = normalize(at - eye);    
  vec3 xaxis = normalize(cross(zaxis, up));
  vec3 yaxis = cross(xaxis, zaxis);

  zaxis = -zaxis;

  mat4 viewMatrix = {
    vec4(xaxis.x, xaxis.y, xaxis.z, -dot(xaxis, eye)),
    vec4(yaxis.x, yaxis.y, yaxis.z, -dot(yaxis, eye)),
    vec4(zaxis.x, zaxis.y, zaxis.z, -dot(zaxis, eye)),
    vec4(0, 0, 0, 1)
  };

  return viewMatrix;
}

mat4 CaptureViews[6] = {
    LookAt(vec3(0.0f, 0.0f, 0.0f), vec3(1.0f,  0.0f,  0.0f), vec3(0.0f, -1.0f,  0.0f)),
    LookAt(vec3(0.0f, 0.0f, 0.0f), vec3(-1.0f,  0.0f,  0.0f),vec3(0.0f, -1.0f,  0.0f)),
    LookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f,  1.0f,  0.0f), vec3(0.0f,  0.0f,  1.0f)),
    LookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f,  0.0f), vec3(0.0f,  0.0f, -1.0f)),
    LookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f,  0.0f,  1.0f), vec3(0.0f, -1.0f,  0.0f)),
    LookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f,  0.0f, -1.0f), vec3(0.0f, -1.0f,  0.0f))
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

layout (set = 0, binding = 0) uniform samplerCube u_EnvironmentMap;

#define PI 3.1415926535897932384626433832795f
#define TWO_PI 6.283185307179586476925286766559f
#define HALF_PI 1.57079632679489661923132169163975f

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