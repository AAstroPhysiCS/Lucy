//type vertex
#version 450
#extension GL_ARB_shader_viewport_layer_array : require
#extension GL_EXT_multiview : require

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

layout (location = 0) in vec3 a_Pos;

layout (location = 0) out vec4 a_Color;

layout (set = 0, binding = 0) uniform samplerCube u_EnvironmentMap;

layout (push_constant) uniform LucyPrefilterParams {
    //x, y => width, height of mipmap
    //z => roughnessValue;
    layout (offset = 64) vec4 u_PrefilterParams;
};

#define PI 3.1415926535897932384626433832795f
#define TWO_PI 6.283185307179586476925286766559f
#define HALF_PI 1.57079632679489661923132169163975f

float DistributionGGX(float dotNH, float roughness) {
    float r2 = roughness * roughness;
    float dotNH2 = dotNH*dotNH;

    float nom = r2;
    float denom = (dotNH2 * (r2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

// Van Der Corpus sequence
// @see http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float VdcSequence(uint bits) {
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N) {
	return vec2(float(i)/float(N), VdcSequence(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
	float a = roughness*roughness;
	
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	// from spherical coordinates to cartesian coordinates - halfway vector
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	// from tangent-space H vector to world-space sample vector
	vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

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