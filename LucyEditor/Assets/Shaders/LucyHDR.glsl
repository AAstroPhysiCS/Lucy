//type vertex
#version 450
#extension GL_ARB_shader_viewport_layer_array : require

layout (location = 0) in vec3 a_Pos;

layout (location = 0) out vec3 a_PosOut;

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

layout (push_constant) uniform LucyCameraPushConstants {
    //proj will be imported as a uniform, since we want to control the "lense" of the camera in certain areas
	mat4 u_ProjMatrix;
    int u_ColorAttachmentOutputIndex;
};

void main() {
	a_PosOut = a_Pos;
    gl_Layer = u_ColorAttachmentOutputIndex;
	gl_Position = u_ProjMatrix * CaptureViews[u_ColorAttachmentOutputIndex] * vec4(a_Pos, 1.0f);
}

//type fragment
#version 450

layout (location = 0) in vec3 a_Pos;

layout (location = 0) out vec4 a_LayeredColorAttachments; //6 layers

layout (set = 0, binding = 0) uniform sampler2D u_EquirectangularMap;

vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= vec2(0.1591, 0.3183);
    uv += 0.5f;
    return uv;
}

void main() {
    vec4 color = texture(u_EquirectangularMap, SampleSphericalMap(normalize(a_Pos)));
    a_LayeredColorAttachments = vec4(color.rgb, 1.0);
}