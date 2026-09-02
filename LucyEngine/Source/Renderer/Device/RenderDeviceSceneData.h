#pragma once

#include <cstdint>

#include "Renderer/Memory/Buffer/RenderDeviceBuffer.h"

#include "Scene/Camera.h"

namespace Lucy {

    template <auto ... Ts>
    struct MembersList {};

    template <typename T>
    concept CanDetectChange = requires() {
        T::Members;
    };

    struct RenderDeviceTextureResource {
        uint32_t TextureIndex = INVALID_INDEX;
        uint32_t SamplerIndex = INVALID_INDEX;

        [[nodiscard]] bool IsValid() const { return TextureIndex != INVALID_INDEX; }

        auto operator<=>(const RenderDeviceTextureResource&) const = default;
    };

    enum class RenderDeviceObjectFlags : uint32_t {
        None = 0,
        Alive = 1 << 0,
        ForceHighestLOD = 1 << 1,
        ForceLowestLOD = 1 << 2
    };

    struct RenderDeviceObjectData {
        glm::mat4 Transform = glm::mat4{ 1.0f };
        glm::mat4 TransformInversedTransposed = glm::mat4{ 1.0f };
        glm::mat4 PreviousTransform = glm::mat4{ 1.0f };

        glm::vec4 BoundingSphere = glm::vec4{ 0.0f }; //Local-space mesh bounding sphere.
        
        glm::vec4 ObjectID = glm::vec4{ -1.0f }; //xyz = picking ID, w = unused.

        // x = MeshIndex y = RenderDeviceObjectFlags z = unused w = float LOD bias encoded using floatBitsToUint
        glm::uvec4 Data = glm::uvec4{ INVALID_INDEX, static_cast<uint32_t>(RenderDeviceObjectFlags::None), 0, 0 }; 

        auto operator<=>(const RenderDeviceObjectData&) const = default;

        static inline constexpr auto Members = MembersList<
            &RenderDeviceObjectData::Transform,
            &RenderDeviceObjectData::TransformInversedTransposed,
            &RenderDeviceObjectData::PreviousTransform,
            &RenderDeviceObjectData::BoundingSphere,
            &RenderDeviceObjectData::ObjectID,
            &RenderDeviceObjectData::Data
        >{};
    };

    struct RenderDevicePBRMaterialData {
        glm::vec4 BaseColor{};
        glm::vec4 ORME{ 1.0f, 0.0f, 0.0f, 0.0f }; // x = Occlusion, y = Roughness, z = Metallic, w = Emissive

        RenderDeviceTextureResource AlbedoMap;
        RenderDeviceTextureResource NormalMap;
        RenderDeviceTextureResource ORMMap;
        RenderDeviceTextureResource AOMap;
        RenderDeviceTextureResource RoughnessMap;
        RenderDeviceTextureResource MetallicMap;
        //RenderDeviceTextureResource ORMMap;
        RenderDeviceTextureResource EmissiveMap;

        float NormalStrength = 1.0f;

        auto operator<=>(const RenderDevicePBRMaterialData&) const = default;

        //TODO: in C++26, change this to reflection library...
        static inline constexpr auto Members = MembersList<
            &RenderDevicePBRMaterialData::BaseColor,
            &RenderDevicePBRMaterialData::ORME,
            &RenderDevicePBRMaterialData::AlbedoMap,
            &RenderDevicePBRMaterialData::NormalMap,
            &RenderDevicePBRMaterialData::ORMMap,
            &RenderDevicePBRMaterialData::AOMap,
            &RenderDevicePBRMaterialData::RoughnessMap,
            &RenderDevicePBRMaterialData::MetallicMap,
            &RenderDevicePBRMaterialData::EmissiveMap,
            &RenderDevicePBRMaterialData::NormalStrength
        >{};
    };

    struct RenderDeviceMeshData {
        glm::vec3 AABBCenter{};
        glm::vec3 AABBExtents{};
        glm::uvec4 Data = glm::uvec4{ 0 }; // x = FirstLOD y = LODCount z = FirstSubmesh w = SubmeshCount

        auto operator<=>(const RenderDeviceMeshData&) const = default;
    };

    struct RenderDeviceMeshLODData {
        glm::uvec4 Meshlets = glm::uvec4{ 0 }; // x = FirstMeshlet y = MeshletCount z = unused w = unused
        glm::vec4 LODData = glm::vec4{ 0.0f };

        auto operator<=>(const RenderDeviceMeshLODData&) const = default;
    };

    struct RenderDeviceSubmeshData {
        glm::mat4 Transform = glm::mat4{ 1.0f };
        glm::mat4 TransformInversedTransposed = glm::mat4{ 1.0f };

        glm::vec4 BoundingSphere = glm::vec4{ 0.0f };

        glm::vec3 AABBCenter{};
        glm::vec3 AABBExtents{};

        glm::uvec4 Draw = glm::uvec4{ 0 }; // x = FirstIndex y = IndexCount z = int32 VertexOffset bit-cast to uint32 w = MaterialIndex
        glm::uvec4 Meshlets = glm::uvec4{ 0 }; // x = FirstMeshlet y = MeshletCount z = RenderBin w = flags
        glm::uvec4 LODs = glm::uvec4{ INVALID_INDEX, 0, 0, 0 }; // x = FirstLOD, y = LODCount

        auto operator<=>(const RenderDeviceSubmeshData&) const = default;
    };

    struct RenderDeviceMeshletData {
        glm::vec4 BoundingSphere = glm::vec4{ 0.0f };
        glm::vec4 NormalCone = glm::vec4{ 0.0f };

        glm::vec3 AABBCenter;
        glm::vec3 AABBExtents;

        glm::uvec4 Draw = glm::uvec4{ 0 }; // x = FirstIndex y = IndexCount z = int32 VertexOffset bit-cast to uint32 w = SubmeshIndex

        auto operator<=>(const RenderDeviceMeshletData&) const = default;
    };

    struct RenderDeviceVisibleObjectData {
        uint32_t ObjectIndex = INVALID_INDEX;

        auto operator<=>(const RenderDeviceVisibleObjectData&) const = default;
    };

    struct RenderDeviceVisibleSubmeshData {
        uint32_t ObjectIndex = INVALID_INDEX;
        uint32_t SubmeshIndex = INVALID_INDEX;
        uint32_t LODIndex = INVALID_INDEX;

        auto operator<=>(const RenderDeviceVisibleSubmeshData&) const = default;
    };

    struct RenderDeviceVisibleDrawData {
        uint32_t ObjectIndex = INVALID_INDEX;
        uint32_t SubmeshIndex = INVALID_INDEX;
        uint32_t MeshletIndex = INVALID_INDEX;

        auto operator<=>(const RenderDeviceVisibleDrawData&) const = default;
    };

    enum class GPUCullViewFlags : uint32_t {
        None = 0,
        EnableFrustumCulling = 1 << 0,
        EnableConeCulling = 1 << 1,
        EnableOcclusionCulling = 1 << 2,
        CameraFreeze = 1 << 3,
        Orthographic = 1 << 4
    };

    struct RenderDeviceCullViewData {
        glm::mat4 View = glm::mat4{ 1.0f };
        glm::mat4 Projection = glm::mat4{ 1.0f };
        glm::mat4 ViewProjection = glm::mat4{ 1.0f };

        glm::vec4 FrustumPlanes[6]{};

        glm::vec4 CameraPosition = glm::vec4{ 0.0f };
        glm::vec4 Viewport = glm::vec4{ 0.0f };

        //x = Hi-Z texture index, y = Hi-Z sampler index, z = Hi-Z mip count, w = GPUCullViewFlags
        glm::uvec4 Data = glm::uvec4{ INVALID_INDEX, INVALID_INDEX, 0, GPUCullViewFlags::None };

        auto operator<=>(const RenderDeviceCullViewData&) const = default;
    };

    struct RenderDeviceIndexedIndirectCommand {
        uint32_t IndexCount = 0;
        uint32_t InstanceCount = 0;
        uint32_t FirstIndex = 0;
        int32_t VertexOffset = 0;
        uint32_t FirstInstance = 0;
    };

    struct RenderDeviceSceneAddresses {
        RenderDeviceBufferReference Materials = 0;
        RenderDeviceBufferReference Objects = 0;
        RenderDeviceBufferReference Meshes = 0;
        RenderDeviceBufferReference MeshLODs = 0;
        RenderDeviceBufferReference Submeshes = 0;
        RenderDeviceBufferReference Meshlets = 0;
        RenderDeviceBufferReference Vertices = 0;
        RenderDeviceBufferReference Indices = 0;
        RenderDeviceBufferReference CullViews = 0;

        auto operator<=>(const RenderDeviceSceneAddresses&) const = default;
    };

    struct RenderDeviceSceneGlobalData {
        static inline constexpr const uint32_t NUM_CASCADES = 4;

        CameraViewProjection Camera{};

        RenderDeviceSceneAddresses Addresses{};

        struct LightValues {
            glm::vec3 Direction;
            glm::vec3 Color;
            glm::mat4 DirLightShadowMatrices[NUM_CASCADES];
            glm::vec4 DirLightShadowCascadeSplits; // x = 0, y = 1, z = 2, w = 3 cascadeIndex

            auto operator<=>(const LightValues&) const = default;
        } DirectionalLight{};

        auto operator<=>(const RenderDeviceSceneGlobalData&) const = default;

        static inline constexpr auto Members = MembersList<
            &RenderDeviceSceneGlobalData::Camera,
            &RenderDeviceSceneGlobalData::Addresses,
            &RenderDeviceSceneGlobalData::DirectionalLight
        >{};
    };

    template <typename T>
    struct GlobalPushConstant {
        RenderDeviceBufferReference Root = 0;
        T Data{};

		explicit constexpr operator bool() const { return Root != 0; }
    };
}