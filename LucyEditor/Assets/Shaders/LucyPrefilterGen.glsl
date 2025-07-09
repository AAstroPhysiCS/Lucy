//type vertex
#version 450
#extension GL_ARB_shader_viewport_layer_array : require

layout (location = 0) in vec3 a_Pos;

layout (location = 0) out vec3 a_PosOut;

layout (push_constant) uniform LucyPrefilterParams {
    mat4 u_MVP; //mvp bcs we do not want to exceed the 128 byte limit
    //x, y => width, height of mipmap
    //z => roughnessValue;
    vec4 u_PrefilterParams;
};

void main() {
	a_PosOut = a_Pos;
    gl_Position = u_MVP * vec4(a_Pos, 1.0f);
}

//type fragment
#version 450

#include "LucySamplingUtilities"

layout (location = 0) in vec3 a_Pos;

layout (location = 0) out vec4 a_Color;

layout (set = 0, binding = 0) uniform samplerCube u_EnvironmentMap;

layout (push_constant) uniform LucyPrefilterParams {
    //x, y => width, height of mipmap
    //z => roughnessValue;
    layout (offset = 64) vec4 u_PrefilterParams;
};

void main() {
	vec3 N = normalize(a_Pos);
    // assume view direction always equal to outgoing direction
    vec3 R = N;
    vec3 V = N;

    float currentRoughnessValue = u_PrefilterParams.z;
    uint totalSamples = uint(u_PrefilterParams.x * u_PrefilterParams.y);
    
	float roughness = max(currentRoughnessValue, 0.04);
    float totalWeight = 0.0f;
    vec3 outputColor = vec3(0.0f);

    for (uint i = 0; i < totalSamples; i++) {
        vec2 Xi = Hammersley(i, totalSamples);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);

        float dotHV = dot(H, V);
        vec3 L = normalize(2.0 * dotHV * H - V);

        float dotNL = max(dot(N, L), 0.0);
        if(dotNL > 0.0) {
           float dotNH = max(dot(N, H), 0.0);
           dotHV = max(dotHV, 0.0);

           float D = DistributionGGX(dotNH, roughness);
           float pdf = D * dotNH / (4.0 * dotHV) + 0.0001; 

           float saTexel  = 4.0 * PI / (6.0 * totalSamples);
           float saSample = 1.0 / (totalSamples * pdf + 0.0001);
           float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 

           outputColor += textureLod(u_EnvironmentMap, L, mipLevel).rgb * dotNL;
           totalWeight += dotNL;
        }
    }
    outputColor /= totalWeight;

    a_Color = vec4(outputColor, 1.0f);
}