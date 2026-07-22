#pragma once

#include <compare>

#include "RenderDeviceSceneData.h"

#include "Renderer/RendererPasses.h"
#include "Renderer/Renderer.h"

#include "Renderer/Device/RenderDevice.h"

#include "RenderDeviceHandles.h"

namespace Lucy {

    class Image;
    class ImageSampler;
    
    class RenderDeviceScene final {
    public:
        RenderDeviceScene(RenderDevice* device);
        ~RenderDeviceScene() = default;

        RenderDeviceScene(const RenderDeviceScene&) = delete;
        RenderDeviceScene& operator=(const RenderDeviceScene&) = delete;
        RenderDeviceScene(RenderDeviceScene&&) = delete;
        RenderDeviceScene& operator=(RenderDeviceScene&&) = delete;
        
        RenderDeviceObjectHandle RegisterMesh(const RenderDeviceMeshData& data);
        RenderDeviceObjectHandle RegisterPBRMaterial(const RenderDevicePBRMaterialData& data);

        void UpdateCamera(const CameraViewProjection& camera);
        void UpdateLightValues(const RenderDeviceSceneGlobalData::LightValues& lightValues);
        void UpdateGlobals(const RenderDeviceSceneGlobalData& data);

        void UpdatePBRMaterial(const RenderDeviceObjectHandle& handle, const RenderDevicePBRMaterialData& data);

        //will always be executed after the fence op
        void SyncFrame(uint32_t frameIndex);

        void RTDestroy();
    private:
        enum class RenderDeviceSceneBufferType {
            Globals,
            Materials,
            Meshes,
            Submeshes,
            Meshlets
        };

        struct RenderDeviceScenePendingUpdate {
            RenderDeviceSceneBufferType BufferType;
            size_t Offset = 0;
            std::vector<std::byte> Data;
        };

        struct RenderDeviceSceneFrameData {
            RenderDeviceResourceHandle GlobalsBuffer;
            RenderDeviceResourceHandle MaterialBuffer;
            RenderDeviceResourceHandle MeshBuffer;
            RenderDeviceResourceHandle SubmeshBuffer;
            RenderDeviceResourceHandle MeshletBuffer;

            std::vector<RenderDeviceScenePendingUpdate> PendingUpdates;

            bool Initialized = false;
        };
    public:
        constexpr RenderDeviceResourceHandle GetBufferHandleByName(std::string_view name) { 
            if (name == "GPUScene")
                return m_FrameData[Renderer::GetCurrentFrameIndex()].GlobalsBuffer;
            if (name == "PBRMaterial")
                return m_FrameData[Renderer::GetCurrentFrameIndex()].MaterialBuffer;
            else if (name == "GPUMesh")
                return m_FrameData[Renderer::GetCurrentFrameIndex()].MeshBuffer;
            else if (name == "GPUSubmesh")
                return m_FrameData[Renderer::GetCurrentFrameIndex()].SubmeshBuffer;
            else if (name == "GPUMeshlet")
                return m_FrameData[Renderer::GetCurrentFrameIndex()].MeshletBuffer;
            return {};
        }
    private:
        RenderDeviceObjectHandle RTCreateSceneGlobals(const RenderDeviceSceneGlobalData& data);
        void RTRegisterMesh(const RenderDeviceObjectHandle& handle, const RenderDeviceMeshData& data);
        void RTRegisterPBRMaterial(const RenderDeviceObjectHandle& handle, const RenderDevicePBRMaterialData& data);

        void RTUpdateCamera(const CameraViewProjection& camera);
        void RTUpdateLightValues(const RenderDeviceSceneGlobalData::LightValues& lightValues);
        void RTUpdateGlobals(const RenderDeviceSceneGlobalData& data);

        void RTUpdatePBRMaterial(const RenderDeviceObjectHandle& handle, const RenderDevicePBRMaterialData& data);

        template <CanDetectChange TData, typename TFunc>
        void DetectChange(const TData& old, const TData& newData, TFunc&& func) {
            const auto ForEach = [&]<auto... Members>(MembersList<Members...>) {
                ([&] {
                    if (old.*Members != newData.*Members)
                        func.template operator()<Members>(/*old.*Members, */newData.*Members);
                }(), ...);
            };
            ForEach(TData::Members);
        }

        template <auto Member>
        void RTEnqueueUpdatePartly(auto& pool, RenderDeviceSceneBufferType bufferType,
            const RenderDeviceObjectHandle& handle, const auto& newValue) {
            using TPool = std::remove_cvref_t<decltype(pool)>;
            using Data = TPool::Data;

            LUCY_ASSERT(Renderer::IsOnRenderThread());
            LUCY_ASSERT(pool.IsValid(handle), "Uploading an invalid pool handle.");

            Data& data = pool.Get(handle);

            const auto* objectAddress = reinterpret_cast<const uint8_t*>(std::addressof(data));
            const auto* memberAddress = reinterpret_cast<const uint8_t*>(std::addressof(data.*Member));

            const size_t memberOffset = static_cast<size_t>(memberAddress - objectAddress);
            const size_t elementOffset = static_cast<size_t>(handle.Index) * sizeof(Data);
            const size_t bufferOffset = elementOffset + memberOffset;

            data.*Member = newValue;

            for (auto& frameData : m_FrameData) {
                RenderDeviceScenePendingUpdate update;
                update.BufferType = bufferType;
                update.Offset = bufferOffset;
                update.Data.resize(sizeof(data.*Member));

                memcpy(update.Data.data(), std::addressof(data.*Member), sizeof(data.*Member));

                frameData.PendingUpdates.push_back(std::move(update));
            }
        }

        void RTApplyPendingUpdates(uint32_t frameIndex) {
            LUCY_ASSERT(Renderer::IsOnRenderThread());
            LUCY_ASSERT(frameIndex < m_FrameData.size());

            auto& frameData = m_FrameData[frameIndex];

            const auto GetBufferHandleByType = [&](auto type) {
                switch (type) {
                    case RenderDeviceSceneBufferType::Globals:
                        return frameData.GlobalsBuffer;
                        break;
                    case RenderDeviceSceneBufferType::Materials:
                        return frameData.MaterialBuffer;
                        break;
                    case RenderDeviceSceneBufferType::Meshes:
                        return frameData.MeshBuffer;
                        break;
                    case RenderDeviceSceneBufferType::Submeshes:
                        return frameData.SubmeshBuffer;
                        break;
                    case RenderDeviceSceneBufferType::Meshlets:
                        return frameData.MeshletBuffer;
                        break;
                }
            };

            for (const auto& update : frameData.PendingUpdates) {
                RenderDeviceResourceHandle bufferHandle = GetBufferHandleByType(update.BufferType);
                LUCY_ASSERT(bufferHandle);

                const auto& buffer = m_RenderDevice->AccessResource<RenderDeviceBuffer>(bufferHandle);
                buffer->RTLoadToDevice(update.Data.data(), update.Data.size(), update.Offset);
            }

            frameData.PendingUpdates.clear();
        }

        template<typename TPool>
        void RTEnqueueUpdate(const TPool& pool, RenderDeviceSceneBufferType bufferType, const TPool::Handle& handle) {
            using TData = TPool::Data;

            LUCY_ASSERT(Renderer::IsOnRenderThread());
            LUCY_ASSERT(pool.IsValid(handle));

            const TData& data = pool.Get(handle);
            const size_t offset = sizeof(TData) * static_cast<size_t>(handle.Index);

            for (auto& frameData : m_FrameData) {
                RenderDeviceScenePendingUpdate update;
                update.BufferType = bufferType;
                update.Offset = offset;
                update.Data.resize(sizeof(TData));

                memcpy(update.Data.data(), std::addressof(data), sizeof(TData));

                frameData.PendingUpdates.push_back(std::move(update));
            }
        }

        template<typename TPool, typename TFunction>
        void ForEachAlive(const TPool& pool, TFunction&& function) const {
            using IndexType = TPool::IndexType;
            using Handle = TPool::Handle;

            for (size_t index = 0; auto& slot : pool) {
                if (!slot.Alive)
                    continue;
                std::invoke(function, Handle{ .Index = static_cast<IndexType>(index), .Generation = slot.Generation }, slot.Data);
                index++;
            }
        }

        template<typename TPool>
        void RTUploadEntirePool(const TPool& pool, const RenderDeviceResourceHandle& bufferHandle) {
            LUCY_ASSERT(Renderer::IsOnRenderThread());
            LUCY_ASSERT(bufferHandle, "Uploading to an invalid buffer.");

            using TData = typename TPool::Data;

            const auto& buffer = m_RenderDevice->AccessResource<RenderDeviceBuffer>(bufferHandle);

            ForEachAlive(pool, [&](typename TPool::Handle handle, const TData& data) {
                buffer->RTLoadToDevice(std::addressof(data), sizeof(TData), sizeof(TData) * static_cast<size_t>(handle.Index));
            });
        }

        template<typename TData>
        bool RTEnsureDeviceAddressBufferCapacity(RenderDeviceResourceHandle& bufferHandle) {
            LUCY_ASSERT(bufferHandle, "Trying to upload dirty render data without a valid buffer.");
            auto buffer = m_RenderDevice->AccessResource<RenderDeviceBuffer>(bufferHandle);

            constexpr auto sizeOfData = sizeof(TData);
            bool needToRegrow = sizeOfData >= buffer->GetSize();
            needToRegrow &= (buffer->GetSize() - buffer->GetAllocatedSize() - sizeOfData) < 0;

            if (!needToRegrow)
                return false;

            size_t newCapacity = buffer->GetSize() * 4;

            RenderDeviceResourceHandle newBuffer = m_RenderDevice->CreateDeviceAddressBuffer(newCapacity * sizeof(TData));

            //COPY THE OLD DATA TODO:????

            if (bufferHandle)
                m_RenderDevice->RTDestroyResource(bufferHandle);

            bufferHandle = newBuffer;

            return true;
        }

        RenderDeviceObjectHandle RTRegisterSubmesh(const RenderDeviceSubmeshData& data);
        RenderDeviceObjectHandle RTRegisterMeshlet(const RenderDeviceMeshletData& data);

        GenerationalPool<RenderDeviceObjectHandle, RenderDevicePBRMaterialData> m_PBRMaterials;
        GenerationalPool<RenderDeviceObjectHandle, RenderDeviceSceneGlobalData> m_Globals;
        GenerationalPool<RenderDeviceObjectHandle, RenderDeviceMeshData> m_Meshes;
        GenerationalPool<RenderDeviceObjectHandle, RenderDeviceSubmeshData> m_Submeshes;
        GenerationalPool<RenderDeviceObjectHandle, RenderDeviceMeshletData> m_Meshlets;

        std::vector<RenderDeviceSceneFrameData> m_FrameData;

        RenderDeviceObjectHandle m_GlobalsHandle;

        RenderDevice* m_RenderDevice = nullptr;

        /* TODO: later
        RenderDeviceResourceHandle m_VertexBuffer;
        RenderDeviceResourceHandle m_IndexBuffer;

        RenderDeviceResourceHandle m_DrawCandidateBuffer;
        RenderDeviceResourceHandle m_VisibleDrawBuffer;
        RenderDeviceResourceHandle m_IndirectCommandBuffer;
        RenderDeviceResourceHandle m_DrawCountBuffer;
        */  
    };
}
