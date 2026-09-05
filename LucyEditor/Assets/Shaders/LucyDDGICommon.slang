#include "LucyCommons.slang"
#include "LucyResource.slang"

static const uint DDGI_IRRADIANCE_TEXELS = 8;
static const uint DDGI_IRRADIANCE_TILE_SIZE = DDGI_IRRADIANCE_TEXELS + 2;

static const uint DDGI_DEPTH_TEXELS = 16;
static const uint DDGI_DEPTH_TILE_SIZE = DDGI_DEPTH_TEXELS + 2;

struct DDGIRayResult {
	float4 RadianceAndDistance;
};

struct DDGISamplingData {
    float4* ProbeOffsets;

    GPUTextureResource IrradianceAtlas;
    GPUTextureResource DepthAtlas;

    float4 ProbeOriginAndMaxDistance;
    float4 ProbeSpacingAndBias;
    float4 Settings;
    uint3 ProbeCounts;
};

struct DDGISurface {
    float3 PositionWS;

    float2 UV;

    float3 NormalWS;
    // we need the actual triangle normal for determining whether
    // a DDGI ray hit the front or back side of geometry.
    float3 GeometricNormalWS;
    float3 TangentWS;
    float3 BitangentWS;

    uint ObjectIndex;
    uint SubmeshIndex;
    uint MaterialIndex;
};

float2 DDGISignNotZero(float2 value) {
    return float2(value.x >= 0.0f ? 1.0f : -1.0f, value.y >= 0.0f ? 1.0f : -1.0f);
}

float2 DDGIOctahedralWrap(float2 value) {
    return (1.0f - abs(value.yx)) * DDGISignNotZero(value);
}

float2 DDGIEncodeOctahedral(float3 direction) {
    direction /= abs(direction.x) + abs(direction.y) + abs(direction.z);
    float2 oct = direction.xy;

    if (direction.z < 0.0f)
        oct = DDGIOctahedralWrap(oct);

    return oct;
}

float3 DDGIDecodeOctahedral(float2 oct) {
    float3 direction = float3(oct.x, oct.y, 1.0f - abs(oct.x) - abs(oct.y));

    if (direction.z < 0.0f)
        direction.xy = DDGIOctahedralWrap(direction.xy);

    return normalize(direction);
}

uint2 GetProbeInteriorTexel(uint2 localTexel, uint interiorTexelCount) {
    uint tileSize = interiorTexelCount + 2;
    uint lastTexel = tileSize - 1;

    bool borderX = localTexel.x == 0 || localTexel.x == lastTexel;
    bool borderY = localTexel.y == 0 || localTexel.y == lastTexel;

    if (!borderX && !borderY)
        return localTexel;

    if (borderX && borderY)
        return uint2(localTexel.x == lastTexel ? 1 : interiorTexelCount, localTexel.y == lastTexel ? 1 : interiorTexelCount);
    if (borderY)
        return uint2(lastTexel - localTexel.x, localTexel.y == 0 ? 1 : interiorTexelCount);

    return uint2(localTexel.x == 0 ? 1 : interiorTexelCount, lastTexel - localTexel.y);
}

float3 GetProbeTexelDirection(uint2 localTexel, uint interiorTexelCount) {
    // localTexel includes the 1-pixel border:
    //
    // 0             = border
    // 1..N          = actual octahedral data
    // N + 1         = border

    uint2 interiorTexel = GetProbeInteriorTexel(localTexel, interiorTexelCount);
    float2 interiorCoordinate = float2(interiorTexel) - float2(1.0f, 1.0f);
    float2 oct = ((interiorCoordinate + 0.5f) / float(interiorTexelCount)) * 2.0f - 1.0f;

    return DDGIDecodeOctahedral(oct);
}

uint2 GetDDGIProbeAtlasTile(uint3 probeCoordinate, uint3 probeCounts) {
    return uint2(probeCoordinate.x + probeCoordinate.z * probeCounts.x, probeCoordinate.y);
}

float2 GetDDGIAtlasUV(uint3 probeCoordinate, float3 direction, uint interiorTexels, uint tileSize, uint3 probeCounts) {
    float2 oct = DDGIEncodeOctahedral(normalize(direction));
    float2 octUV = oct * 0.5f + 0.5f;

    uint2 atlasTile = GetDDGIProbeAtlasTile(probeCoordinate, probeCounts);
    uint2 atlasDimensions = uint2(probeCounts.x * probeCounts.z * tileSize, probeCounts.y * tileSize);

    float2 atlasTexel = float2(atlasTile * tileSize) + 1.5f + octUV * float(interiorTexels - 1);

    return atlasTexel / float2(atlasDimensions);
}

float3 SampleDDGIProbeIrradiance(DDGISamplingData data, uint3 probeCoordinate, float3 normalWS) {
    float2 uv = GetDDGIAtlasUV(probeCoordinate, normalWS, DDGI_IRRADIANCE_TEXELS, DDGI_IRRADIANCE_TILE_SIZE, data.ProbeCounts.xyz);
    return data.IrradianceAtlas.Sample2DTextureLevel(uv, 0.0f).rgb;
}

float2 SampleDDGIProbeDepth(DDGISamplingData data, uint3 probeCoordinate, float3 probeToPointDirection) {
    float2 uv = GetDDGIAtlasUV(probeCoordinate, probeToPointDirection, DDGI_DEPTH_TEXELS, DDGI_DEPTH_TILE_SIZE, data.ProbeCounts.xyz);
    return data.DepthAtlas.Sample2DTextureLevel_Float2(uv, 0.0f);
}

float GetDDGIProbeVisibility(float2 moments, float normalizedDistance) {
    if (normalizedDistance <= moments.x)
        return 1.0f;

    float variance = moments.y - moments.x * moments.x;
    variance = max(variance, 0.00001f);

    float difference = normalizedDistance - moments.x;

    return saturate(variance / (variance + difference * difference));
}

bool IsInsideDDGIVolume(DDGISamplingData data, float3 positionWS) {
    float3 gridPosition = (positionWS - data.ProbeOriginAndMaxDistance.xyz) / data.ProbeSpacingAndBias.xyz;
    float3 maxProbeCoordinate = float3(data.ProbeCounts.xyz - 1);

    return all(gridPosition >= 0.0f) && all(gridPosition <= maxProbeCoordinate);
}

float3 SampleDDGIIrradiance(DDGISamplingData data, float3 positionWS, float3 normalWS) {
    uint3 probeCounts = data.ProbeCounts.xyz;

    float3 biasedPosition = positionWS + normalWS * data.ProbeSpacingAndBias.w;
    float3 gridPosition = (biasedPosition - data.ProbeOriginAndMaxDistance.xyz) / data.ProbeSpacingAndBias.xyz;

    gridPosition = clamp(gridPosition, float3(0.0f), float3(probeCounts - 1));

    uint3 baseProbeCoordinate = min(uint3(floor(gridPosition)), probeCounts - 2);
    float3 alpha = gridPosition - float3(baseProbeCoordinate);

    float3 irradiance = float3(0.0f);
    float totalWeight = 0.0f;

    for (uint z = 0; z < 2; z++) {
        for (uint y = 0; y < 2; y++) {
            for (uint x = 0; x < 2; x++) {
                uint3 probeOffset = uint3(x, y, z);
                uint3 probeCoordinate = baseProbeCoordinate + probeOffset;

                float weightX = x == 0 ? 1.0f - alpha.x : alpha.x;
                float weightY = y == 0 ? 1.0f - alpha.y : alpha.y;
                float weightZ = z == 0 ? 1.0f - alpha.z : alpha.z;

                float weight = weightX * weightY * weightZ;

                float3 probePosition = GetDDGIProbePosition(data, probeCoordinate);
                float3 probeToPoint = biasedPosition - probePosition;

                float distanceToProbe = length(probeToPoint);

                if (distanceToProbe <= 0.0001f)
                    continue;

                float3 probeToPointDirection = probeToPoint / distanceToProbe;
                float3 pointToProbeDirection = -probeToPointDirection;

                float normalWeight = saturate(dot(normalWS, pointToProbeDirection) * 0.5f + 0.5f);
                normalWeight = normalWeight * normalWeight + 0.2f;

                float2 moments = SampleDDGIProbeDepth(data, probeCoordinate, probeToPointDirection);
                float normalizedDistance = saturate(distanceToProbe / data.ProbeOriginAndMaxDistance.w);
                float visibility = GetDDGIProbeVisibility(moments, normalizedDistance);

                weight *= normalWeight * visibility;

                if (weight <= 0.000001f)
                    continue;

                float3 probeIrradiance = SampleDDGIProbeIrradiance(data, probeCoordinate, normalWS);

                irradiance += probeIrradiance * weight;
                totalWeight += weight;
            }
        }
    }

    if (totalWeight <= 0.000001f)
        return float3(0.0f);

    return irradiance / totalWeight;
}

float3 DDGIInterpolate(float3 a, float3 b, float3 c, float2 barycentrics) {
    float barycentric0 = 1.0f - barycentrics.x - barycentrics.y;
    return a * barycentric0 + b * barycentrics.x + c * barycentrics.y;
}

float2 DDGIInterpolate(float2 a, float2 b, float2 c, float2 barycentrics) {
    float barycentric0 = 1.0f - barycentrics.x - barycentrics.y;
    return a * barycentric0 + b * barycentrics.x + c * barycentrics.y;
}

DDGISurface GetDDGISurface(GPUScene *scene, BuiltInTriangleIntersectionAttributes attributes) {
    DDGISurface surface;

    uint objectIndex = InstanceID();
    GPUObject object = scene->Addresses.Objects[objectIndex];

    uint meshIndex = object.Data.x;
    GPUMesh mesh = scene->Addresses.Meshes[meshIndex];

    uint localSubmeshIndex = GeometryIndex();
    uint submeshIndex = mesh.Data.z + localSubmeshIndex;
    GPUSubmesh submesh = scene->Addresses.Submeshes[submeshIndex];

    uint triangleIndex = PrimitiveIndex();
    uint firstIndex = submesh.Draw.x + triangleIndex * 3;

    uint index0 = scene->Addresses.Indices[firstIndex];
    uint index1 = scene->Addresses.Indices[firstIndex + 1];
    uint index2 = scene->Addresses.Indices[firstIndex + 2];

    int vertexOffset = asint(submesh.Draw.z);

    GPUVertex vertex0 = scene->Addresses.Vertices[vertexOffset + int(index0)];
    GPUVertex vertex1 = scene->Addresses.Vertices[vertexOffset + int(index1)];
    GPUVertex vertex2 = scene->Addresses.Vertices[vertexOffset + int(index2)];

    surface.PositionWS = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    surface.UV = DDGIInterpolate(
        vertex0.TextureCoords,
        vertex1.TextureCoords,
        vertex2.TextureCoords,
        attributes.barycentrics
    );

    float3 normalLS = normalize(DDGIInterpolate(
        vertex0.Normals,
        vertex1.Normals,
        vertex2.Normals,
        attributes.barycentrics
    ));

    float3 tangentLS = normalize(DDGIInterpolate(
        vertex0.Tangents,
        vertex1.Tangents,
        vertex2.Tangents,
        attributes.barycentrics
    ));

    float3 bitangentLS = normalize(DDGIInterpolate(
        vertex0.Bitangents,
        vertex1.Bitangents,
        vertex2.Bitangents,
        attributes.barycentrics
    ));

    float4x4 normalMatrix = mul(object.TransformInversedTransposed, submesh.TransformInversedTransposed);
    float4x4 modelMatrix = mul(object.Transform, submesh.Transform);

    float3 normalWS = normalize(mul(float3x3(normalMatrix), normalLS));
    float3 tangentWS = normalize(mul(float3x3(modelMatrix), tangentLS));
    float3 bitangentWS = normalize(mul(float3x3(modelMatrix), bitangentLS));

    tangentWS = normalize(tangentWS - normalWS * dot(normalWS, tangentWS));

    float handedness = dot(cross(normalWS, tangentWS), bitangentWS) < 0.0f ? -1.0f : 1.0f;
    bitangentWS = cross(normalWS, tangentWS) * handedness;

    float3 position0WS = mul(modelMatrix, float4(vertex0.Position, 1.0f)).xyz;
    float3 position1WS = mul(modelMatrix, float4(vertex1.Position, 1.0f)).xyz;
    float3 position2WS = mul(modelMatrix, float4(vertex2.Position, 1.0f)).xyz;

    float3 geometricNormalWS = normalize(cross(position1WS - position0WS, position2WS - position0WS));

    surface.NormalWS = normalWS;
    surface.GeometricNormalWS = geometricNormalWS;
    surface.TangentWS = tangentWS;
    surface.BitangentWS = bitangentWS;

    surface.ObjectIndex = objectIndex;
    surface.SubmeshIndex = submeshIndex;
    surface.MaterialIndex = submesh.Draw.w;

    return surface;
}

//TODO: clean it
float3 GetDDGISurfaceNormal(DDGISurface surface, PBRMaterial material, PBRMaterialSample materialSample) {
    float3 normalWS = normalize(surface.NormalWS);

    if (!material.HasNormalMap())
        return normalWS;

    float3 normalTS = materialSample.NormalTS;

    return normalize(
        surface.TangentWS * normalTS.x +
        surface.BitangentWS * normalTS.y +
        normalWS * normalTS.z
    );
}

float3 GetFibonacciDirection(uint rayIndex, uint rayCount) {
    float index = float(rayIndex) + 0.5f;
    float cosTheta = 1.0f - 2.0f * index / float(rayCount);

    float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));
    float phi = 2.0f * PI * frac(float(rayIndex) / GOLDEN_RATIO);

    return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

uint GetProbeIndex(uint3 probeCoordinate, uint3 probeCounts) {
    return probeCoordinate.x + probeCoordinate.y * probeCounts.x + probeCoordinate.z * probeCounts.x * probeCounts.y;
}

float3 GetDDGIProbePosition(DDGISamplingData data, uint3 probeCoordinate) {
    uint probeIndex = GetProbeIndex(probeCoordinate, data.ProbeCounts.xyz);
    float3 probeOffset = data.ProbeOffsets[probeIndex].xyz;

    return data.ProbeOriginAndMaxDistance.xyz + float3(probeCoordinate) * data.ProbeSpacingAndBias.xyz + probeOffset;
}

uint3 GetProbeCoordinate(uint probeIndex, uint3 probeCounts) {
    uint3 coordinate;
    coordinate.x = probeIndex % probeCounts.x;
    coordinate.y = (probeIndex / probeCounts.x) % probeCounts.y;
    coordinate.z = probeIndex / (probeCounts.x * probeCounts.y);

    return coordinate;
}

uint DDGIHash(uint value) {
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;

    return value;
}

float DDGIHash01(uint value) {
    return float(DDGIHash(value) & 0x00FFFFFFu) / 16777216.0f;
}

float4 GetDDGIRayRotation(uint frameNumber) {
    float u1 = DDGIHash01(frameNumber * 3u + 0u);
    float u2 = DDGIHash01(frameNumber * 3u + 1u);
    float u3 = DDGIHash01(frameNumber * 3u + 2u);

    float sqrtOneMinusU1 = sqrt(1.0f - u1);
    float sqrtU1 = sqrt(u1);

    float theta1 = 2.0f * PI * u2;
    float theta2 = 2.0f * PI * u3;

    return float4(sqrtOneMinusU1 * sin(theta1), sqrtOneMinusU1 * cos(theta1), sqrtU1 * sin(theta2), sqrtU1 * cos(theta2));
}

float3 RotateVectorByQuaternion(float3 value, float4 quaternion) {
    float3 q = quaternion.xyz;
    return value + 2.0f * cross(q, cross(q, value) + quaternion.w * value);
}

static const uint DDGI_FIXED_RAY_COUNT = 16;

float3 GetDDGIRayDirection(uint rayIndex, uint rayCount, uint frameNumber) {
    uint fixedRayCount = min(DDGI_FIXED_RAY_COUNT, rayCount);

    if (rayIndex < fixedRayCount)
        return GetFibonacciDirection(rayIndex, fixedRayCount);

    uint rotatingRayCount = rayCount - fixedRayCount;

    if (rotatingRayCount == 0)
        return GetFibonacciDirection(rayIndex, rayCount);

    uint rotatingRayIndex = rayIndex - fixedRayCount;

    float3 direction = GetFibonacciDirection(rotatingRayIndex, rotatingRayCount);
    float4 rotation = GetDDGIRayRotation(frameNumber);

    return normalize(RotateVectorByQuaternion(direction, rotation));
}

/*
float3 GetDDGIRayDirection(uint rayIndex, uint rayCount, uint frameNumber) {
    float3 direction = GetFibonacciDirection(rayIndex, rayCount);
    float4 rotation = GetDDGIRayRotation(frameNumber);
    return normalize(RotateVectorByQuaternion(direction, rotation));
}*/