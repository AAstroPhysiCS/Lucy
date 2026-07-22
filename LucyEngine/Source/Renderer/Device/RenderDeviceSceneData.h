#pragma once

#include <cstdint>

#include "Renderer/RendererPasses.h"
#include "Renderer/Memory/Buffer/RenderDeviceBuffer.h"

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

    struct RenderDevicePBRMaterialData {
        glm::vec4 BaseColor{};
        glm::vec4 ORME{}; // x = Occlusion, y = Roughness, z = Metallic, w = Emissive

        RenderDeviceTextureResource AlbedoMap;
        RenderDeviceTextureResource NormalMap;
        RenderDeviceTextureResource AOMap;
        RenderDeviceTextureResource RoughnessMap;
        RenderDeviceTextureResource MetallicMap;
        //RenderDeviceTextureResource ORMMap;
        RenderDeviceTextureResource EmissiveMap;

        float NormalStrength = 1.0f;
        float Padding0 = 0.0f;
        float Padding1 = 0.0f;
        float Padding2 = 0.0f;

        auto operator<=>(const RenderDevicePBRMaterialData&) const = default;

        //TODO: in C++26, change this to reflection library...
        static inline constexpr auto Members = MembersList<
            &RenderDevicePBRMaterialData::BaseColor,
            &RenderDevicePBRMaterialData::ORME,
            &RenderDevicePBRMaterialData::AlbedoMap,
            &RenderDevicePBRMaterialData::NormalMap,
            &RenderDevicePBRMaterialData::AOMap,
            &RenderDevicePBRMaterialData::RoughnessMap,
            &RenderDevicePBRMaterialData::MetallicMap,
            //&RenderDevicePBRMaterialData::ORMMap,
            &RenderDevicePBRMaterialData::EmissiveMap,
            &RenderDevicePBRMaterialData::NormalStrength
        >{};
    };

    struct RenderDeviceMeshData {

    };

    struct RenderDeviceSubmeshData {

    };

    struct RenderDeviceMeshletData {

    };

    struct RenderDeviceSceneAddresses {
        RenderDeviceBufferReference Materials = 0;
        RenderDeviceBufferReference Meshes = 0;
        //RenderDeviceBufferReference Submeshes = 0;
        //RenderDeviceBufferReference Meshlets = 0;

        //RenderDeviceBufferReference Vertices = 0;
        //RenderDeviceBufferReference Indices = 0;

        //RenderDeviceBufferReference DrawCandidates = 0;
        //RenderDeviceBufferReference VisibleDraws = 0;
        //RenderDeviceBufferReference IndirectCommands = 0;
        //RenderDeviceBufferReference DrawCount = 0;

        auto operator<=>(const RenderDeviceSceneAddresses&) const = default;
    };

    struct RenderDeviceSceneGlobalData {
        CameraViewProjection Camera{};

        RenderDeviceSceneAddresses Addresses{};

        struct LightValues {
            glm::vec3 Direction;
            float _padding0;
            glm::vec3 Color;
            float _padding1;
            glm::mat4 DirLightShadowMatrices[ShadowPass::NUM_CASCADES];
            glm::vec4 DirLightShadowCascadeSplits; // x = 0, y = 1, z = 2, w = 3 cascadeIndex

            auto operator<=>(const LightValues&) const = default;
        } DirectionalLight{};

        auto operator<=>(const RenderDeviceSceneGlobalData&) const = default;

        static inline constexpr auto Members = MembersList<
            &RenderDeviceSceneGlobalData::Camera,
            &RenderDeviceSceneGlobalData::DirectionalLight
        >{};
    };

    template <typename T>
    struct GlobalPushConstant {
        RenderDeviceBufferReference Root;
        uint64_t Padding = 0;
        T Data;
    };
}