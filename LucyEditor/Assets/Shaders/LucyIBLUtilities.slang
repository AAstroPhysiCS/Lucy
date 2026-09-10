#ifndef LUCY_IBL_UTILITIES_SLANG
#define LUCY_IBL_UTILITIES_SLANG

float4x4 LookAt(float3 eye, float3 at, float3 up) {
  float3 zaxis = normalize(at - eye);    
  float3 xaxis = normalize(cross(zaxis, up));
  float3 yaxis = cross(xaxis, zaxis);

  zaxis = -zaxis;

  float4x4 viewMatrix = {
    float4(xaxis.x, xaxis.y, xaxis.z, -dot(xaxis, eye)),
    float4(yaxis.x, yaxis.y, yaxis.z, -dot(yaxis, eye)),
    float4(zaxis.x, zaxis.y, zaxis.z, -dot(zaxis, eye)),
    float4(0, 0, 0, 1)
  };

  return viewMatrix;
}

static float4x4 CaptureViews[6] = {
    LookAt(float3(0.0f, 0.0f, 0.0f), float3(1.0f, 0.0f, 0.0f), float3(0.0f, -1.0f, 0.0f)),
    LookAt(float3(0.0f, 0.0f, 0.0f), float3(-1.0f, 0.0f, 0.0f), float3(0.0f, -1.0f, 0.0f)),
    LookAt(float3(0.0f, 0.0f, 0.0f), float3(0.0f, -1.0f, 0.0f), float3(0.0f, 0.0f, -1.0f)),
    LookAt(float3(0.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f), float3(0.0f, 0.0f, 1.0f)),
    LookAt(float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, 1.0f), float3(0.0f, -1.0f, 0.0f)),
    LookAt(float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, -1.0f), float3(0.0f, -1.0f, 0.0f))
};
#endif // LUCY_IBL_UTILITIES_SLANG