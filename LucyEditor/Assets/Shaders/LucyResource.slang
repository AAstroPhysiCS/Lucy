#ifndef LUCY_RESOURCES_SLANG
#define LUCY_RESOURCES_SLANG

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
    float3 _padding0;

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

        result.NormalTS = NormalMap.Sample2DTexture(uv).xyz * 2.0f - 1.0f;
        result.NormalTS.rg *= NormalStrength;
        result.NormalTS = normalize(result.NormalTS);

        float3 emissive = EmissiveMap.Sample2DTexture(uv, float4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
        result.EmissiveColorAndStrength = float4(emissive, ORME.w);

        result.AlphaCutoff = AlphaCutoff;

        return result;
    }
};

struct GPUSceneAddresses {
    PBRMaterial* PBRMaterials;
    GPUMesh* Meshes;
    //GPUInstance* Instances;
    //GPUSubmesh *Submeshes;
    //uint64_t _padding0;
    //GPUMeshlet* Meshlets;
    //PackedVertex* Vertices;
    //uint* MeshletVertexIndices;
    //GPUVisibleDraw* VisibleDraws;
};

struct GPUMesh {
    uint64_t VertexBufferAddress;
    uint64_t IndexBufferAddress;

    float4x4 Transform;

    uint FirstSubmesh;
    uint SubmeshCount;

    float4 BoundsSphere;
};

struct GPUSubmesh {
    uint MeshIndex;

    uint FirstIndex;
    uint IndexCount;
    int VertexOffset;

    uint FirstMeshlet;
    uint MeshletCount;

    uint MaterialIndex;

    float4 BoundsSphere;
};

struct SceneCamera {
    float4x4 u_ViewMatrix;
    float4x4 u_ProjMatrix;
    float4 u_CamPos;
};

struct DirectionalLight {
    float3 Direction;
    float _padding0;
    float3 Color;
    float _padding1;
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
    GPUScene *Root;
    uint64_t _padding0;
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
    SamplerState Samplers[];
}

#endif