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
        
        RenderDeviceObjectHandle RegisterMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, std::vector<Submesh>& submeshes);
        RenderDeviceObjectHandle RegisterPBRMaterial(const RenderDevicePBRMaterialData& data);
        RenderDeviceObjectHandle RegisterObject(const RenderDeviceObjectHandle& meshHandle, const glm::mat4& transform, RenderDeviceObjectFlags flags);
        RenderDeviceObjectHandle RegisterCullView(const RenderDeviceCullViewData& data);

        void UpdateCamera(const CameraViewProjection& camera);
        void UpdateLightValues(const RenderDeviceSceneGlobalData::LightValues& lightValues);
        void UpdateGlobals(const RenderDeviceSceneGlobalData& data);
        void UpdateCullView(const RenderDeviceObjectHandle& handle, const RenderDeviceCullViewData& data);
        void UpdateObjectTransform(const RenderDeviceObjectHandle& handle, const glm::mat4& transform);

        void UpdatePBRMaterial(const RenderDeviceObjectHandle& handle, const RenderDevicePBRMaterialData& data);

        //will always be executed after the fence op
        void SyncFrame(uint32_t frameIndex);

        void RTDestroy();
    private:
        enum class RenderDeviceSceneBufferType {
            Globals,
            Objects,
            Materials,
            Meshes,
            MeshLODs,
            Submeshes,
            Meshlets,
            CullViews
        };

        struct RenderDeviceScenePendingUpdate {
            RenderDeviceSceneBufferType BufferType = RenderDeviceSceneBufferType::Globals;
            size_t Offset = 0;
            std::vector<std::byte> Data;
        };

        struct RenderDeviceSceneFrameData {
            RenderDeviceResourceHandle GlobalsBuffer;
            RenderDeviceResourceHandle ObjectsBuffer;
            RenderDeviceResourceHandle MaterialBuffer;
            RenderDeviceResourceHandle MeshBuffer;
            RenderDeviceResourceHandle MeshLODBuffer;
            RenderDeviceResourceHandle SubmeshBuffer;
            RenderDeviceResourceHandle MeshletBuffer;
            RenderDeviceResourceHandle CullViewsBuffer;

            std::vector<RenderDeviceScenePendingUpdate> PendingUpdates;

            bool Initialized = false;
        };
    public:
        [[nodiscard]] const RenderDeviceResourceHandle& GetGlobalVertexBufferHandle() const { return m_GlobalVertexBuffer; }
        [[nodiscard]] const RenderDeviceResourceHandle& GetGlobalIndexBufferHandle() const { return m_GlobalIndexBuffer; }

        constexpr RenderDeviceResourceHandle GetCurrentFrameBufferHandle(std::string_view name) { return GetCurrentFrameBufferHandle(name, Renderer::GetCurrentFrameIndex()); }
        std::vector<RenderDeviceResourceHandle> GetCurrentFrameBufferHandles(std::string_view name);

        static uint64_t GetSubmeshCapacity() { return s_SubmeshCapacity; }
        static uint64_t GetObjectCapacity() { return s_ObjectCapacity; }
        static uint64_t GetMeshletCapacity() { return s_MeshletCapacity; }
    private:
        RenderDeviceObjectHandle RTCreateSceneGlobals(const RenderDeviceSceneGlobalData& data);
        void RTRegisterMesh(const RenderDeviceObjectHandle& handle, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<Submesh>& submeshes);
        void RTRegisterPBRMaterial(const RenderDeviceObjectHandle& handle, const RenderDevicePBRMaterialData& data);
        void RTRegisterObject(const RenderDeviceObjectHandle& handle);
        void RTRegisterCullView(const RenderDeviceObjectHandle& handle);

        void RTUpdateCamera(const CameraViewProjection& camera);
        void RTUpdateLightValues(const RenderDeviceSceneGlobalData::LightValues& lightValues);
        void RTUpdateGlobals(const RenderDeviceSceneGlobalData& data);
        void RTUpdateCullView(const RenderDeviceObjectHandle& handle, const RenderDeviceCullViewData& data);
        void RTUpdateObjectTransform(const RenderDeviceObjectHandle& handle, const glm::mat4& transform);

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
                    case RenderDeviceSceneBufferType::MeshLODs:
                        return frameData.MeshLODBuffer;
                        break;
                    case RenderDeviceSceneBufferType::Submeshes:
                        return frameData.SubmeshBuffer;
                        break;
                    case RenderDeviceSceneBufferType::Meshlets:
                        return frameData.MeshletBuffer;
                        break;
                    case RenderDeviceSceneBufferType::CullViews:
                        return frameData.CullViewsBuffer;
                        break;
                    case RenderDeviceSceneBufferType::Objects:
                        return frameData.ObjectsBuffer;
                        break;
                }
            };

            for (const auto& update : frameData.PendingUpdates) {
                RenderDeviceResourceHandle bufferHandle = GetBufferHandleByType(update.BufferType);
                LUCY_ASSERT(bufferHandle);

                const auto& buffer = m_RenderDevice->AccessResource<RenderDeviceBuffer>(bufferHandle);
                buffer->RTLoadToDevice(m_RenderDevice, update.Data.data(), update.Data.size(), update.Offset);
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
                if (slot.Alive)
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
                buffer->RTLoadToDevice(m_RenderDevice, std::addressof(data), sizeof(TData), sizeof(TData) * static_cast<size_t>(handle.Index));
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

            auto newCreateInfo = buffer->GetCreateInfo();
            newCreateInfo.Size = newCapacity * sizeof(TData);
            RenderDeviceResourceHandle newBuffer = m_RenderDevice->CreateDeviceAddressBuffer(newCreateInfo);

            //COPY THE OLD DATA TODO:????

            if (bufferHandle)
                m_RenderDevice->RTDestroyResource(bufferHandle);

            bufferHandle = newBuffer;

            return true;
        }

        RenderDeviceObjectHandle RTRegisterSubmesh(const RenderDeviceSubmeshData& data);
        RenderDeviceObjectHandle RTRegisterMeshlet(const RenderDeviceMeshletData& data);
        RenderDeviceObjectHandle RTRegisterMeshLOD(const RenderDeviceMeshLODData& data);

        constexpr RenderDeviceResourceHandle GetCurrentFrameBufferHandle(std::string_view name, size_t frameIndex) {
            if (name == "GPUScene")
                return m_FrameData[frameIndex].GlobalsBuffer;
            else if (name == "GPUObjects")
                return m_FrameData[frameIndex].ObjectsBuffer;
            else if (name == "GPUMeshLODs")
                return m_FrameData[frameIndex].MeshLODBuffer;
            else if (name == "GPUCullViews")
                return m_FrameData[frameIndex].CullViewsBuffer;
            else if (name == "PBRMaterial")
                return m_FrameData[frameIndex].MaterialBuffer;
            else if (name == "GPUMeshes")
                return m_FrameData[frameIndex].MeshBuffer;
            else if (name == "GPUSubmeshes")
                return m_FrameData[frameIndex].SubmeshBuffer;
            else if (name == "GPUMeshlets")
                return m_FrameData[frameIndex].MeshletBuffer;
            LUCY_ASSERT(false, "Returning empty frame data buffer handle");
            return {};
        }

        GenerationalPool<RenderDeviceObjectHandle, RenderDeviceSceneGlobalData> m_Globals;
        GenerationalPool<RenderDeviceObjectHandle, RenderDeviceObjectData> m_Objects;
        GenerationalPool<RenderDeviceObjectHandle, RenderDeviceMeshData> m_Meshes;
        GenerationalPool<RenderDeviceObjectHandle, RenderDeviceMeshLODData> m_MeshLODs;
        GenerationalPool<RenderDeviceObjectHandle, RenderDeviceCullViewData> m_CullViews;
        GenerationalPool<RenderDeviceObjectHandle, RenderDeviceSubmeshData> m_Submeshes;
        GenerationalPool<RenderDeviceObjectHandle, RenderDeviceMeshletData> m_Meshlets;
        GenerationalPool<RenderDeviceObjectHandle, RenderDevicePBRMaterialData> m_PBRMaterials;

        std::vector<RenderDeviceSceneFrameData> m_FrameData;

        RenderDeviceObjectHandle m_GlobalsHandle;

        RenderDevice* m_RenderDevice = nullptr;

        RenderDeviceResourceHandle m_GlobalVertexBuffer;
        RenderDeviceResourceHandle m_GlobalIndexBuffer;

        uint64_t m_GlobalVertexCount = 0;
        uint64_t m_GlobalIndexCount = 0;

        static inline uint64_t s_ObjectCapacity = 1024;
        static inline uint64_t s_MaterialCapacity = 4096;
        static inline uint64_t s_MeshCapacity = 1024;
        static inline uint64_t s_SubmeshCapacity = 64 * 1024;
        static inline uint64_t s_MeshletCapacity = 1024 * 1024;

        static inline uint64_t s_GlobalVertexCapacity = 1024 * 1024 * 4;
        static inline uint64_t s_GlobalIndexCapacity = 3 * s_GlobalVertexCapacity * 10;
    };
}
