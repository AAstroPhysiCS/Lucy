#ifndef LUCY_RESOURCES_SLANG
#define LUCY_RESOURCES_SLANG

static const uint OBJECT_FLAG_ALIVE = 1u << 0u;
static const uint OBJECT_FLAG_FORCE_HIGHEST_LOD = 1u << 1u;
static const uint OBJECT_FLAG_FORCE_LOWEST_LOD = 1u << 2u;

static const uint VIEW_FLAG_FRUSTUM = 1u << 0u;
static const uint VIEW_FLAG_CONE = 1u << 1u;
static const uint VIEW_FLAG_OCCLUSION = 1u << 2u;
static const uint VIEW_FLAG_ORTHOGRAPHIC = 1u << 4u;

#include "LucyCommons.slang"

static constexpr const uint INVALID_BINDLESS_INDEX = 0xFFFFFFFFu;

enum class TextureType : uint {
    Texture,
    Sampler
};

struct GPUResource {
    uint Index;

    bool IsValid() { return Index != INVALID_BINDLESS_INDEX; }
};

public struct GPUTextureResource {
    GPUResource TextureResource = { INVALID_BINDLESS_INDEX };
    GPUResource SamplerResource = { INVALID_BINDLESS_INDEX };

    private static constexpr float4 FALLBACK_VALUE = float4(0.0f, 0.0f, 0.0f, 1.0f);

    public __init(uint textureIndex) {
        TextureResource.Index = textureIndex;
    }

    public bool IsValid() { return TextureResource.IsValid() && SamplerResource.IsValid(); }

    public float4 Sample2DTexture(float2 uv, float4 fallbackValue = FALLBACK_VALUE) {
        return !IsValid() ? fallbackValue : BindlessTable::Textures2D[NonUniformResourceIndex(TextureResource.Index)].Sample(BindlessTable::Samplers[NonUniformResourceIndex(SamplerResource.Index)], uv);
    }
    
    public float2 Sample2DTexture_Float2(float2 uv, float2 fallbackValue = FALLBACK_VALUE.xy) {
        return !IsValid() ? fallbackValue : BindlessTable::Textures2D_Float2[NonUniformResourceIndex(TextureResource.Index)].Sample(BindlessTable::Samplers[NonUniformResourceIndex(SamplerResource.Index)], uv);
    }
    
    public float4 SampleCubeTextureLevel(float3 direction, float level, float4 fallbackValue = FALLBACK_VALUE) {
        return !IsValid() ? fallbackValue : BindlessTable::CubeTextures[NonUniformResourceIndex(TextureResource.Index)].SampleLevel(BindlessTable::Samplers[NonUniformResourceIndex(SamplerResource.Index)], direction, level);
    }
    
    public float4 Sample2DArrayTexture(float3 uv, float4 fallbackValue = FALLBACK_VALUE) {
        return !IsValid() ? fallbackValue : BindlessTable::TextureArrays2D[NonUniformResourceIndex(TextureResource.Index)].Sample(BindlessTable::Samplers[NonUniformResourceIndex(SamplerResource.Index)], uv);
    }
    
    public float2 Sample2DArrayTexture_Float2(float3 uv, float2 fallbackValue = FALLBACK_VALUE.xy) {
        return !IsValid() ? fallbackValue : BindlessTable::TextureArrays2D_Float2[NonUniformResourceIndex(TextureResource.Index)].Sample(BindlessTable::Samplers[NonUniformResourceIndex(SamplerResource.Index)], uv);
    }

    public float4 SampleCubeTexture(float3 uv, float4 fallbackValue = FALLBACK_VALUE) {
        return !IsValid() ? fallbackValue : BindlessTable::CubeTextures[NonUniformResourceIndex(TextureResource.Index)].Sample(BindlessTable::Samplers[NonUniformResourceIndex(SamplerResource.Index)], uv);
    }
    
    public float4 SampleCubeArrayTexture(float4 uv, float4 fallbackValue = FALLBACK_VALUE) {
        return !IsValid() ? fallbackValue : BindlessTable::CubeTextureArrays[NonUniformResourceIndex(TextureResource.Index)].Sample(BindlessTable::Samplers[NonUniformResourceIndex(SamplerResource.Index)], uv);
    }
    
    public float4 LoadStorageTexture2D(int2 uv, int2 offset, float4 fallbackValue = FALLBACK_VALUE) {
        return !IsValid() ? fallbackValue : BindlessTable::StorageTextures2D[NonUniformResourceIndex(TextureResource.Index)].Load(uv, offset);
    }

    public float2 LoadStorageTexture2D_Float2(int2 uv, int2 offset, float2 fallbackValue = FALLBACK_VALUE.xy) {
        return !IsValid() ? fallbackValue : BindlessTable::StorageTextures2D_Float2[NonUniformResourceIndex(TextureResource.Index)].Load(uv, offset);
    }

    public float4 LoadStorageTexture2DArray(int3 uv, int3 offset, float4 fallbackValue = FALLBACK_VALUE) {
        return !TextureResource.IsValid() ? fallbackValue : BindlessTable::StorageTextureArrays2D[NonUniformResourceIndex(TextureResource.Index)].Load(uv, offset);
    }
    
    public float2 LoadStorageTexture2DArray_Float2(int3 uv, int3 offset, float2 fallbackValue = FALLBACK_VALUE.xy) {
        return !TextureResource.IsValid() ? fallbackValue : BindlessTable::StorageTextureArrays2D_Float2[NonUniformResourceIndex(TextureResource.Index)].Load(uv, offset);
    }
};

struct PBRMaterialSample {
    float4 BaseColor;
    float4 EmissiveColorAndStrength; // xyz = color, w = strength
    float3 NormalTS;
    float AO;
    float Roughness;
    float Metallic;
    float AlphaCutoff;
};

public struct PBRMaterial {
    float4 BaseColor;
    float4 ORME; // x = Occlusion, y = Roughness, z = Metallic, w = Emissive

    GPUTextureResource AlbedoMap;
    GPUTextureResource NormalMap;
    GPUTextureResource AOMap;
    GPUTextureResource RoughnessMap;
    GPUTextureResource MetallicMap;
    //GPUTextureResource ORMMap;
    GPUTextureResource EmissiveMap;

    float NormalStrength = 1.0f;

    private static constexpr float AlphaCutoff = 0.5f;

    public static float GetAlphaCutoff() { return AlphaCutoff; }

    public bool HasNormalMap() { return NormalMap.IsValid(); }

    PBRMaterialSample Sample(float2 uv) {
        PBRMaterialSample result;
        result.BaseColor = AlbedoMap.Sample2DTexture(uv, BaseColor);
        result.AO = ORME.x;
        result.Roughness = ORME.y;
        result.Metallic = ORME.z;

        if (AOMap.IsValid()) {
            float3 ao = AOMap.Sample2DTexture(uv).xyz;
            result.AO *= ao.r;
        }

        if (RoughnessMap.IsValid()) {
            float3 r = RoughnessMap.Sample2DTexture(uv).xyz;
            result.Roughness *= r.g;
        }

        if (MetallicMap.IsValid()) {
            float3 m = MetallicMap.Sample2DTexture(uv).xyz;
            result.Metallic *= m.b;
        }

        /*
        if (ORMMap.IsValid()) {
            float3 orm = ORMMap.Sample2DTexture(uv, ORME).xyz;

            result.AO *= orm.r;
            result.Roughness *= orm.g;
            result.Metallic *= orm.b;
        }*/

        float2 normalXY = NormalMap.Sample2DTexture(uv).xy * 2.0f - 1.0f;
        normalXY *= NormalStrength;
        //basically x^2 + y^2 + z^2 = 1 -> z = sqrt(1 - x^2 - y^2)
        //we need this for BC5 normals (bistro etc...)
        float normalZ = sqrt(saturate(1.0f - dot(normalXY, normalXY)));
        result.NormalTS = normalize(float3(normalXY, normalZ));

        float3 emissive = EmissiveMap.Sample2DTexture(uv, float4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
        result.EmissiveColorAndStrength = float4(emissive, ORME.w);

        result.AlphaCutoff = AlphaCutoff;

        return result;
    }
};

struct GPUVertex {
    float3 Position;
    float3 MeshID;
    float2 TextureCoords;
    float3 Normals;
    float3 Tangents;
    float3 Bitangents;
};

struct GPUObject {
    float4x4 Transform;
    float4x4 TransformInversedTransposed;
    float4x4 PreviousTransform;

    float4 BoundingSphere;
    float4 ObjectID;
    uint4 Data; // x = MeshIndex y = RenderDeviceObjectFlags z = unused w = float LOD bias encoded using floatBitsToUint
};

struct GPUMesh {
    float3 AABBCenter;
    float3 AABBExtents;
    uint4 Data; // x = unused y = unused z = FirstSubmesh w = SubmeshCount
};

struct GPUMeshLOD {
    uint4 Meshlets; // x = FirstMeshlet y = MeshletCount z = unused w = unused
    float4 LODData; // x = minimum projected radius in pixels yzw = unused
};

struct GPUSubmesh {
    float4x4 Transform;
    float4x4 TransformInversedTransposed;

    float4 BoundingSphere;

    float3 AABBCenter;
    float3 AABBExtents;

    uint4 Draw; // x = FirstIndex y = IndexCount z = int32 VertexOffset bit-cast to uint32 w = MaterialIndex
    uint4 Meshlets; // x = FirstMeshlet y = MeshletCount z = RenderBin w = flags
    uint4 LODs; // x = FirstLOD, y = LODCount
};

struct GPUMeshlet {
    float4 BoundingSphere;
    float4 NormalCone;
    
    float3 AABBCenter;
    float3 AABBExtents;

    uint4 Draw; // x = FirstIndex y = IndexCount z = int32 VertexOffset bit-cast to uint32 w = SubmeshIndex
};

struct GPUVisibleObject {
    uint ObjectIndex;
};

struct GPUVisibleSubmesh {
    uint ObjectIndex;
    uint SubmeshIndex;
    uint LODIndex;
};

struct GPUVisibleDraw {
    uint ObjectIndex;
    uint SubmeshIndex;
    uint MeshletIndex;
};

struct GPUIndexedIndirectCommand {
    uint IndexCount;
    uint InstanceCount;
    uint FirstIndex;
    int VertexOffset;
    uint FirstInstance;
};

struct GPUDispatchIndirectCommand {
    uint GroupCountX;
    uint GroupCountY;
    uint GroupCountZ;
};

struct GPUCullView {
    float4x4 View;
    float4x4 Projection;
    float4x4 ViewProjection;

    float4 FrustumPlanes[6];

    float4 CameraPosition;
    float4 Viewport;
    uint4 Data; // x = Hi-Z texture index, y = Hi-Z sampler index, z = Hi-Z mip count, w = GPUCullViewFlags
};

struct GPUSceneAddresses {
    PBRMaterial* PBRMaterials;
    GPUObject* Objects;
    GPUMesh* Meshes;
    GPUMeshLOD* MeshLODs;
    GPUSubmesh* Submeshes;
    GPUMeshlet* Meshlets;
    GPUVertex* Vertices;
    uint* Indices;
    GPUCullView* CullViews;
};

struct SceneCamera {
    float4x4 u_ViewMatrix;
    float4x4 u_ProjMatrix;
    float4 u_CamPos;
};

struct DirectionalLight {
    float3 Direction;
    float3 Color;
    float4x4 DirLightShadowMatrices[NUM_CASCADES];
    float4 DirLightShadowCascadeSplits; // x = 0, y = 1, z = 2, w = 3 cascadeIndex
};

struct GPUScene {
    SceneCamera Camera;
    GPUSceneAddresses Addresses;
    DirectionalLight DirectionalLight;
};

//adds 8 bytes to the already existing sizeof(T)
//take that in to consideration okidoki!!!!
struct PushConstantGlobal<T> {
    GPUScene* Root;
    T Data;
};

namespace BindlessTable {
    [[vk::binding(0, 0)]]
    Texture2D<float4> Textures2D[];

    [[vk::binding(1, 0)]]
    Texture2DArray<float4> TextureArrays2D[];

    [[vk::binding(2, 0)]]
    TextureCube<float4> CubeTextures[];

    [[vk::binding(3, 0)]]
    TextureCubeArray<float4> CubeTextureArrays[];

    [[vk::binding(4, 0)]]
    RWTexture2D<float4> StorageTextures2D[];

    [[vk::binding(5, 0)]]
    RWTexture2DArray<float4> StorageTextureArrays2D[];

    [[vk::binding(6, 0)]]
    Texture2D<float2> Textures2D_Float2[];

    [[vk::binding(7, 0)]]
    RWTexture2D<float2> StorageTextures2D_Float2[];

    [[vk::binding(8, 0)]]
    Texture2DArray<float2> TextureArrays2D_Float2[];

    [[vk::binding(9, 0)]]
    RWTexture2DArray<float2> StorageTextureArrays2D_Float2[];

    [[vk::binding(10, 0)]]
    Texture2D<float> Textures2D_Float[];

    [[vk::binding(11, 0)]]
    RWTexture2D<float> StorageTextures2D_Float[];

    [[vk::binding(12, 0)]]
    SamplerState Samplers[];
}

float GetMaximumScale(float4x4 transform) {
    const float3 column0 = float3(transform[0][0], transform[1][0], transform[2][0]);
    const float3 column1 = float3(transform[0][1], transform[1][1], transform[2][1]);
    const float3 column2 = float3(transform[0][2], transform[1][2], transform[2][2]);

    return max(length(column0),max(length(column1), length(column2)));
}

float GetPixelWorldSize(float3 center, float radius, GPUCullView view) {
    if ((view.Data.w & VIEW_FLAG_ORTHOGRAPHIC) != 0)
        return 2.0f / (abs(view.Projection[1][1]) * view.Viewport.y);
    float distanceToSurface = max(0.0f, length(view.CameraPosition.xyz - center) - radius);
    return distanceToSurface * 2.0f / (abs(view.Projection[1][1]) * view.Viewport.y);
}

uint SelectLOD(GPUSubmesh submesh, float3 worldCenter, float worldScale, GPUCullView view, uint objectFlags, GPUMeshLOD* meshLODs) {
    uint firstLOD = submesh.LODs.x;
    uint lodCount = submesh.LODs.y;

    if (lodCount == 0)
        return INVALID_BINDLESS_INDEX;

    if ((objectFlags & OBJECT_FLAG_FORCE_HIGHEST_LOD) != 0)
        return firstLOD;

    if ((objectFlags & OBJECT_FLAG_FORCE_LOWEST_LOD) != 0)
        return firstLOD + lodCount - 1;

    float pixelWorldSize = GetPixelWorldSize(worldCenter, submesh.BoundingSphere.w * worldScale, view);
    float maxPixelError = 2.0f;

    // Try the cheapest LOD first.
    for (int lodOffset = int(lodCount) - 1; lodOffset >= 0; lodOffset--) {
        uint lodIndex = firstLOD + uint(lodOffset);
        GPUMeshLOD lod = meshLODs[lodIndex];

        float worldError = lod.LODData.x * worldScale;

        if (pixelWorldSize * maxPixelError >= worldError)
            return lodIndex;
    }

    return firstLOD;
}

bool IsSphereInsideFrustum(float3 center, float radius, GPUCullView view) {
    [unroll]
    for (uint planeIndex = 0; planeIndex < 6; planeIndex++) {
        float4 plane = view.FrustumPlanes[planeIndex];
        float distance = dot(plane.xyz, center) + plane.w;
        // basic stuff... n*p+d < -r => sphere is outside of the plane, where p = center of sphere, n = (x,y,z)
        if (distance < -radius)
            return false;
    }
    return true;
}

bool IsAABBInsideFrustum(float3 center, float3 extents, GPUCullView view) {
    [unroll]
    for (uint i = 0; i < 6; i++) {
        float4 plane = view.FrustumPlanes[i];
        float projectedRadius = dot(abs(plane.xyz), extents);
        float distance = dot(plane.xyz, center) + plane.w;

        //sometimes, some objects still flicker... so give some margin (0.5% is good in bistro)
        float epsilon = max(0.05f, projectedRadius * 0.05f);

        if (distance + projectedRadius < 0.0f)
            return false;
    }

    return true;
}

#endif