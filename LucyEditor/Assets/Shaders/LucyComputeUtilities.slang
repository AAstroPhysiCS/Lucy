#ifndef LUCY_COMPUTE_UTILITIES_SLANG
#define LUCY_COMPUTE_UTILITIES_SLANG

float3 ThreadIDToSamplingPos(uint2 threadID, uint face, float2 cubemapSize) {
    float2 uv = (float2(threadID) + 0.5) / cubemapSize;
    uv = uv * 2.0 - 1.0;
    
    // Face-dependent math without conditionals
    const float3[6] faceUVtoDir = {
        float3( 1.0, -uv.y, -uv.x),   // +X
        float3(-1.0, -uv.y,  uv.x),   // -X
        float3( uv.x,  1.0,  uv.y),   // +Y
        float3( uv.x, -1.0, -uv.y),   // -Y
        float3( uv.x, -uv.y,  1.0),   // +Z
        float3(-uv.x, -uv.y, -1.0)    // -Z
    };
    return normalize(faceUVtoDir[face]);
}

// Precomputed per-face basis vectors
float3 GetFaceNormal(int face) {
    const float3[6] normals = {
        float3(1.0f, 0.0f, 0.0f), 
        float3(-1.0f, 0.0f, 0.0f),
        float3(0.0f, 1.0f, 0.0f),
        float3(0.0f, -1.0f, 0.0f), 
        float3(0.0f, 0.0f, 1.0f), 
        float3(0.0f, 0.0f, -1.0f)
    };
    return normals[face];
}

float2 ProjectToFacePlane(float3 sampleDir, int face) {
    // Branchless projection using face index
    const float3[6] axisSign = float3[](
        float3( 1.0, -1.0, -1.0), // +X: (x=1, flip zy)
        float3(-1.0, -1.0,  1.0), // -X: (x=-1, flip z)
        float3( 1.0,  1.0,  1.0), // +Y: (y=1, x/z)
        float3( 1.0, -1.0, -1.0), // -Y: (y=-1, flip z)
        float3( 1.0, -1.0,  1.0), // +Z: (z=1)
        float3(-1.0, -1.0, -1.0)  // -Z: (z=-1, flip x)
    );

    // Select projection rules based on face
    float3 signedDir = sampleDir * axisSign[face];

    // Branchless component selection
    bool3 faceMask = bool3(
        face < 2,               // X faces
        face >= 2 && face < 4,  // Y faces
        face >= 4               // Z faces
    );

    return float2(
        dot(float3(signedDir.z, signedDir.x, signedDir.x), float3(faceMask)),
        dot(float3(-signedDir.y, signedDir.z, -signedDir.y), float3(faceMask))
    );
}

float3 SampleVectorToFaceUV(float3 sampleDir, int srcFace, float2 cubemapSize) {
    // Project sampleDir onto source face's plane
    float2 uv = ProjectToFacePlane(sampleDir, srcFace);
    uv = (uv * 0.5 + 0.5) * cubemapSize; // [0,size]
    return float3(uv, srcFace);
}

/*
    Thanks to Alin for this conversion function
*/
float3 CubeCoordToWorld(float3 cubeCoord, float2 cubemapSize) {
    float2 texCoord = float2(cubeCoord.xy) / cubemapSize;
    texCoord = texCoord  * 2.0 - 1.0;
    switch(int(cubeCoord.z)) {
        case 0:
            return float3(1.0, -texCoord.yx); // +x
        case 1: 
            return float3(-1.0, -texCoord.y, texCoord.x); //-x
        case 2: 
            return float3(texCoord.x, 1.0, texCoord.y); //+y
        case 3: 
            return float3(texCoord.x, -1.0, -texCoord.y); //-y
        case 4: 
            return float3(texCoord.x, -texCoord.y, 1.0); //+z
        case 5: 
            return float3(-texCoord.xy, -1.0); //-z
    }
    return float3(0.0);
}
#endif // LUCY_COMPUTE_UTILITIES_SLANG