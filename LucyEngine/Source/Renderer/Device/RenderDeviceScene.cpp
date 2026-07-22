#include "lypch.h"
#include "RenderDeviceScene.h"

namespace Lucy {

    RenderDeviceScene::RenderDeviceScene(RenderDevice* device)
        : m_RenderDevice(device) {
        const uint32_t maxFramesInFlight = Renderer::GetMaxFramesInFlight();
        m_FrameData.resize(maxFramesInFlight);

        for (auto& frameData : m_FrameData) {
            frameData.GlobalsBuffer = device->CreateDeviceAddressBuffer(sizeof(RenderDeviceSceneGlobalData));
            frameData.MaterialBuffer = device->CreateDeviceAddressBuffer(128 * sizeof(RenderDevicePBRMaterialData));
            frameData.MeshBuffer = device->CreateDeviceAddressBuffer(128 * sizeof(RenderDeviceMeshData));
            frameData.SubmeshBuffer = device->CreateDeviceAddressBuffer(128 * sizeof(RenderDeviceSubmeshData));
            frameData.MeshletBuffer = device->CreateDeviceAddressBuffer(128 * sizeof(RenderDeviceMeshletData));
        }

        m_GlobalsHandle = RTCreateSceneGlobals({});
    }

    RenderDeviceObjectHandle RenderDeviceScene::RTCreateSceneGlobals(const RenderDeviceSceneGlobalData& data) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTCreateSceneGlobals");
        LUCY_ASSERT(Renderer::IsOnRenderThread());
        return m_Globals.Create(data);
    }

    RenderDeviceObjectHandle RenderDeviceScene::RegisterMesh(const RenderDeviceMeshData& data) {
        const RenderDeviceObjectHandle& handle = m_Meshes.Create(data);
        Renderer::EnqueueToRenderCommandQueue([this, data, handle](const auto& device) {
            RTRegisterMesh(handle, data);
        });
        return handle;
    }

    RenderDeviceObjectHandle RenderDeviceScene::RegisterPBRMaterial(const RenderDevicePBRMaterialData& data) {
        const RenderDeviceObjectHandle& handle = m_PBRMaterials.Create(data);
        Renderer::EnqueueToRenderCommandQueue([this, data, handle](const auto& device) {
            RTRegisterPBRMaterial(handle, data);
        });
        return handle;
    }

    void RenderDeviceScene::RTRegisterMesh(const RenderDeviceObjectHandle& handle, const RenderDeviceMeshData& data) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTRegisterMesh");
        LUCY_ASSERT(Renderer::IsOnRenderThread());
        RTEnqueueUpdate(m_Meshes, RenderDeviceSceneBufferType::Meshes, handle);
    }

    void RenderDeviceScene::RTRegisterPBRMaterial(const RenderDeviceObjectHandle& handle, const RenderDevicePBRMaterialData& data) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTRegisterPBRMaterial");
        LUCY_ASSERT(Renderer::IsOnRenderThread());
        RTEnqueueUpdate(m_PBRMaterials, RenderDeviceSceneBufferType::Materials, handle);
    }

    RenderDeviceObjectHandle RenderDeviceScene::RTRegisterSubmesh(const RenderDeviceSubmeshData& data) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTRegisterSubmesh");
        LUCY_ASSERT(Renderer::IsOnRenderThread());

        const RenderDeviceObjectHandle handle = m_Submeshes.Create(data);
        RTEnqueueUpdate(m_Submeshes, RenderDeviceSceneBufferType::Submeshes, handle);

        return handle;
    }

    RenderDeviceObjectHandle RenderDeviceScene::RTRegisterMeshlet(const RenderDeviceMeshletData& data) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTRegisterMeshlet");
        LUCY_ASSERT(Renderer::IsOnRenderThread());

        const RenderDeviceObjectHandle handle = m_Meshlets.Create(data);
        RTEnqueueUpdate(m_Meshlets, RenderDeviceSceneBufferType::Meshlets, handle);

        return handle;
    }

    void RenderDeviceScene::UpdateCamera(const CameraViewProjection& camera) {
        Renderer::EnqueueToRenderCommandQueue([this, camera](const auto& device) {
            RTUpdateCamera(camera);
        });
    }

    void RenderDeviceScene::UpdateLightValues(const RenderDeviceSceneGlobalData::LightValues& lightValues) {
        Renderer::EnqueueToRenderCommandQueue([this, lightValues](const auto& device) {
            RTUpdateLightValues(lightValues);
        });
    }

    void RenderDeviceScene::UpdateGlobals(const RenderDeviceSceneGlobalData& data) {
        Renderer::EnqueueToRenderCommandQueue([this, data](const auto& device) {
            RTUpdateGlobals(data);
        });
    }

    void RenderDeviceScene::UpdatePBRMaterial(const RenderDeviceObjectHandle& handle, const RenderDevicePBRMaterialData& data) {
        Renderer::EnqueueToRenderCommandQueue([this, handle, data](const auto& device) {
            RTUpdatePBRMaterial(handle, data);
        });
    }

    void RenderDeviceScene::RTUpdateCamera(const CameraViewProjection& camera) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTUpdateCamera");
        LUCY_ASSERT(Renderer::IsOnRenderThread());

        auto& globals = m_Globals.Get(m_GlobalsHandle);

        if (globals.Camera == camera)
            return;

        RTEnqueueUpdatePartly<&RenderDeviceSceneGlobalData::Camera>(m_Globals, RenderDeviceSceneBufferType::Globals, m_GlobalsHandle, camera);
    }

    void RenderDeviceScene::RTUpdateLightValues(const RenderDeviceSceneGlobalData::LightValues& lightValues) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTUpdateLightValues");
        LUCY_ASSERT(Renderer::IsOnRenderThread());

        auto& globals = m_Globals.Get(m_GlobalsHandle);

        if (globals.DirectionalLight == lightValues)
            return;

        RTEnqueueUpdatePartly<&RenderDeviceSceneGlobalData::DirectionalLight>(m_Globals, RenderDeviceSceneBufferType::Globals, m_GlobalsHandle, lightValues);
    }

    void RenderDeviceScene::RTUpdateGlobals(const RenderDeviceSceneGlobalData& data) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTUpdateGlobals");
        LUCY_ASSERT(Renderer::IsOnRenderThread());

        auto& currData = m_Globals.Get(m_GlobalsHandle);
        if (currData == data)
            return;

        DetectChange<RenderDeviceSceneGlobalData>(currData, data, [&]<auto Member>(const auto& newValue) {
            RTEnqueueUpdatePartly<Member>(m_Globals, RenderDeviceSceneBufferType::Globals, m_GlobalsHandle, newValue);
        });
    }

    void RenderDeviceScene::RTUpdatePBRMaterial(const RenderDeviceObjectHandle& handle, const RenderDevicePBRMaterialData& data) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTUpdatePBRMaterial");
        LUCY_ASSERT(Renderer::IsOnRenderThread());

        const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

        auto& currData = m_PBRMaterials.Get(handle);
        if (currData == data)
            return;

        DetectChange<RenderDevicePBRMaterialData>(currData, data, [&]<auto Member>(const auto& newValue) {
            RTEnqueueUpdatePartly<Member>(m_PBRMaterials, RenderDeviceSceneBufferType::Materials, handle, newValue);
        });
    }

    void RenderDeviceScene::SyncFrame(uint32_t frameIndex) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::SyncFrame");
        LUCY_ASSERT(Renderer::IsOnRenderThread());

        auto& frameData = m_FrameData[frameIndex];

        const auto UploadAllAddressesToGlobalBuffer = [&]() {
            RenderDeviceSceneAddresses addresses{};
            addresses.Materials = m_RenderDevice->AccessResource<RenderDeviceBuffer>(frameData.MaterialBuffer)->GetDeviceAddress();
            addresses.Meshes = m_RenderDevice->AccessResource<RenderDeviceBuffer>(frameData.MeshBuffer)->GetDeviceAddress();
            //addresses.Submeshes = m_RenderDevice->AccessResource<RenderDeviceBuffer>(frameData.SubmeshBuffer)->GetDeviceAddress();
            //addresses.Meshlets = m_RenderDevice->AccessResource<RenderDeviceBuffer>(frameData.MeshletBuffer)->GetDeviceAddress();

            constexpr size_t addressesOffset = offsetof(RenderDeviceSceneGlobalData, Addresses);

            const auto& globalsBuffer = m_RenderDevice->AccessResource<RenderDeviceBuffer>(frameData.GlobalsBuffer);
            globalsBuffer->RTLoadToDevice(std::addressof(addresses), sizeof(addresses), addressesOffset);
        };

        bool materialBufferResized = RTEnsureDeviceAddressBufferCapacity<RenderDevicePBRMaterialData>(frameData.MaterialBuffer);
        bool meshBufferResized = RTEnsureDeviceAddressBufferCapacity<RenderDeviceMeshData>(frameData.MeshBuffer);
        //bool submeshBufferResized = RTEnsureDeviceAddressBufferCapacity<RenderDeviceSubmeshData>(frameData.SubmeshBuffer);
        //bool meshletBufferResized = RTEnsureDeviceAddressBufferCapacity<RenderDeviceMeshletData>(frameData.MeshletBuffer);

        if (materialBufferResized) {
            RTUploadEntirePool(m_PBRMaterials, frameData.MaterialBuffer);
            std::erase_if(frameData.PendingUpdates, [&](const RenderDeviceScenePendingUpdate& update) {
                return update.BufferType == RenderDeviceSceneBufferType::Materials;
            });
        }

        if (meshBufferResized) {
            RTUploadEntirePool(m_Meshes, frameData.MeshBuffer);
            std::erase_if(frameData.PendingUpdates, [&](const RenderDeviceScenePendingUpdate& update) {
                return update.BufferType == RenderDeviceSceneBufferType::Meshes;
            });
        }

        //if (submeshBufferResized) {
        //    RTUploadEntirePool(m_Submeshes, frameData.SubmeshBuffer);
        //    std::erase_if(frameData.PendingUpdates, [&](const RenderDeviceScenePendingUpdate& update) {
        //        return update.BufferType == RenderDeviceSceneBufferType::Submeshes;
        //    });
        //}
        //
        //if (meshletBufferResized) {
        //    RTUploadEntirePool(m_Meshlets, frameData.MeshletBuffer);
        //    std::erase_if(frameData.PendingUpdates, [&](const RenderDeviceScenePendingUpdate& update) {
        //        return update.BufferType == RenderDeviceSceneBufferType::Meshlets;
        //    });
        //}

        if (!frameData.Initialized) {
            UploadAllAddressesToGlobalBuffer();
            frameData.Initialized = true;
        }

        if (materialBufferResized || meshBufferResized/* || submeshBufferResized || meshletBufferResized*/) {
            UploadAllAddressesToGlobalBuffer();
        }

        if (frameData.PendingUpdates.empty())
            return;

        RTApplyPendingUpdates(frameIndex);
    }

	void RenderDeviceScene::RTDestroy() {
        LUCY_ASSERT(Renderer::IsOnRenderThread());

        for (auto& frameData : m_FrameData) {
            m_RenderDevice->RTDestroyResource(frameData.GlobalsBuffer);
            m_RenderDevice->RTDestroyResource(frameData.MaterialBuffer);
            m_RenderDevice->RTDestroyResource(frameData.MeshBuffer);
            m_RenderDevice->RTDestroyResource(frameData.SubmeshBuffer);
            m_RenderDevice->RTDestroyResource(frameData.MeshletBuffer);
        }

        m_Globals.Clear();
        m_PBRMaterials.Clear();
        m_Meshes.Clear();
        m_Submeshes.Clear();
        m_Meshlets.Clear();
	}
}