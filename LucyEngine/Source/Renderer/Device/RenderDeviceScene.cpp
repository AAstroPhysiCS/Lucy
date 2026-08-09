#include "lypch.h"
#include "RenderDeviceScene.h"

namespace Lucy {

    RenderDeviceScene::RenderDeviceScene(RenderDevice* device)
        : m_RenderDevice(device) {
        const uint32_t maxFramesInFlight = Renderer::GetMaxFramesInFlight();
        m_FrameData.resize(maxFramesInFlight);

        /*
        * I am allocating here a large chunk of memory... i guess so around 600mb... lets say 1gb... in the future, make it so that it can grow...
        */

        for (auto& frameData : m_FrameData) {
            frameData.GlobalsBuffer = device->CreateDeviceAddressBuffer({ "Globalsbuffer", sizeof(RenderDeviceSceneGlobalData) });
            frameData.ObjectsBuffer = device->CreateDeviceAddressBuffer({ "ObjectBuffer", s_ObjectCapacity * sizeof(RenderDeviceObjectData) });
            frameData.MaterialBuffer = device->CreateDeviceAddressBuffer({ "Materialbuffer", s_MaterialCapacity * sizeof(RenderDevicePBRMaterialData) });
            frameData.MeshBuffer = device->CreateDeviceAddressBuffer({ "Meshbuffer", s_MeshCapacity * sizeof(RenderDeviceMeshData) });
            frameData.MeshLODBuffer = device->CreateDeviceAddressBuffer({ "MeshLODBuffer", 4 * s_SubmeshCapacity * sizeof(RenderDeviceMeshLODData) });
            frameData.SubmeshBuffer = device->CreateDeviceAddressBuffer({ "Submeshbuffer", s_SubmeshCapacity * sizeof(RenderDeviceSubmeshData) });
            frameData.MeshletBuffer = device->CreateDeviceAddressBuffer({ "Meshletbuffer", s_MeshletCapacity * sizeof(RenderDeviceMeshletData) });
            frameData.CullViewsBuffer = device->CreateDeviceAddressBuffer({ "CullViewsBuffer", 10 * sizeof(RenderDeviceCullViewData) });
        }

        for (auto& frameData : m_FrameData) {
            const auto& buffer = m_RenderDevice->AccessResource<RenderDeviceBuffer>(frameData.ObjectsBuffer);
            RenderDeviceObjectData emptyData{
                .Data = {
                    INVALID_INDEX, RenderDeviceObjectFlags::None, 0, 0
                }
            };
            for (size_t i = 0; i < s_ObjectCapacity; i++) {
				buffer->RTLoadToDevice(m_RenderDevice, reinterpret_cast<const void*>(&emptyData), sizeof(RenderDeviceObjectData), sizeof(RenderDeviceObjectData) * i);
            }
        }

        m_GlobalVertexBuffer = device->CreateDeviceAddressBuffer({ "GlobalVertexBuffer", s_GlobalVertexCapacity * sizeof(Vertex),
            BufferUsage::Storage | BufferUsage::TransferDestination | BufferUsage::TransferSource });

        m_GlobalIndexBuffer = device->CreateDeviceAddressBuffer({ "GlobalIndexBuffer", s_GlobalIndexCapacity * sizeof(uint32_t),
            BufferUsage::Storage | BufferUsage::Index | BufferUsage::TransferDestination | BufferUsage::TransferSource });

        m_GlobalsHandle = RTCreateSceneGlobals({});
    }

    std::vector<RenderDeviceResourceHandle> RenderDeviceScene::GetCurrentFrameBufferHandles(std::string_view name) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::GetCurrentFrameBufferHandles");
        std::vector<RenderDeviceResourceHandle> result;
        result.resize(Renderer::GetMaxFramesInFlight());

        for (size_t i = 0; i < Renderer::GetMaxFramesInFlight(); i++)
            result[i] = GetCurrentFrameBufferHandle(name, i);

        return result;
    }

    RenderDeviceObjectHandle RenderDeviceScene::RTCreateSceneGlobals(const RenderDeviceSceneGlobalData& data) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTCreateSceneGlobals");
        LUCY_ASSERT(Renderer::IsOnRenderThread());
        return m_Globals.Create(data);
    }

    RenderDeviceObjectHandle RenderDeviceScene::RegisterMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, std::vector<Submesh>& submeshes) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTRegisterMesh");
        const RenderDeviceObjectHandle& handle = m_Meshes.Create(RenderDeviceMeshData{}); //uninitialized
        Renderer::EnqueueToRenderCommandQueue([this, vertices = std::move(vertices), indices = std::move(indices), submeshes = std::move(submeshes), handle](const auto& device) {
            RTRegisterMesh(handle, vertices, indices, submeshes);
        });
        return handle;
    }

    RenderDeviceObjectHandle RenderDeviceScene::RegisterObject(const RenderDeviceObjectHandle& meshHandle, const glm::mat4& transform, RenderDeviceObjectFlags flags) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RegisterObject");
        RenderDeviceObjectData data{};
        data.Transform = transform;
        data.TransformInversedTransposed = glm::transpose(glm::inverse(transform));
        data.PreviousTransform = transform;
        data.Data.x = meshHandle.Index;
        data.Data.y = static_cast<uint32_t>(flags) | static_cast<uint32_t>(RenderDeviceObjectFlags::Alive);

        const RenderDeviceObjectHandle& handle = m_Objects.Create(data);
        Renderer::EnqueueToRenderCommandQueue([this, meshHandle, transform, handle](const auto& device) {
            RTRegisterObject(handle);
        });
        return handle;
    }

    RenderDeviceObjectHandle RenderDeviceScene::RegisterCullView(const RenderDeviceCullViewData& data) {
		LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RegisterCullView");
		const RenderDeviceObjectHandle& handle = m_CullViews.Create(data);
		Renderer::EnqueueToRenderCommandQueue([this, data, handle](const auto& device) {
			RTRegisterCullView(handle);
		});
		return handle;
    }

    void RenderDeviceScene::RTRegisterObject(const RenderDeviceObjectHandle& handle) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTRegisterObject");
        LUCY_ASSERT(Renderer::IsOnRenderThread());
        RTEnqueueUpdate(m_Objects, RenderDeviceSceneBufferType::Objects, handle);
    }

    RenderDeviceObjectHandle RenderDeviceScene::RegisterPBRMaterial(const RenderDevicePBRMaterialData& data) {
        const RenderDeviceObjectHandle& handle = m_PBRMaterials.Create(data);
        Renderer::EnqueueToRenderCommandQueue([this, data, handle](const auto& device) {
            RTRegisterPBRMaterial(handle, data);
        });
        return handle;
    }

    void RenderDeviceScene::RTRegisterCullView(const RenderDeviceObjectHandle& handle) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTRegisterCullView");
        LUCY_ASSERT(Renderer::IsOnRenderThread());
        RTEnqueueUpdate(m_CullViews, RenderDeviceSceneBufferType::CullViews, handle);
    }

    void RenderDeviceScene::UpdateCullView(const RenderDeviceObjectHandle& handle, const RenderDeviceCullViewData& data) {
        Renderer::EnqueueToRenderCommandQueue([this, handle, data](const auto& device) {
            RTUpdateCullView(handle, data);
        });
    }

    void RenderDeviceScene::UpdateObjectTransform(const RenderDeviceObjectHandle& handle, const glm::mat4& transform) {
        Renderer::EnqueueToRenderCommandQueue([this, handle, transform](const auto& device) {
            RTUpdateObjectTransform(handle, transform);
        });
    }

    void RenderDeviceScene::RTUpdateCullView(const RenderDeviceObjectHandle& handle, const RenderDeviceCullViewData& data) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTUpdateCullView");
        LUCY_ASSERT(Renderer::IsOnRenderThread());
        LUCY_ASSERT(m_CullViews.IsValid(handle));

        RenderDeviceCullViewData& currentData = m_CullViews.Get(handle);
        currentData = data;

        RTEnqueueUpdate(m_CullViews, RenderDeviceSceneBufferType::CullViews, handle);
    }

    void RenderDeviceScene::RTUpdateObjectTransform(const RenderDeviceObjectHandle& handle, const glm::mat4& transform) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTUpdateObjectTransform");
        LUCY_ASSERT(Renderer::IsOnRenderThread());
        LUCY_ASSERT(m_Objects.IsValid(handle));

        RenderDeviceObjectData& object = m_Objects.Get(handle);

        if (object.Transform == transform)
            return;

        object.PreviousTransform = object.Transform;
        object.Transform = transform;
        object.TransformInversedTransposed = glm::transpose(glm::inverse(transform));

        RTEnqueueUpdate(m_Objects, RenderDeviceSceneBufferType::Objects, handle);
    }

    void RenderDeviceScene::RTRegisterMesh(const RenderDeviceObjectHandle& handle, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<Submesh>& submeshes) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTRegisterMesh");
        LUCY_ASSERT(Renderer::IsOnRenderThread());
        LUCY_ASSERT(m_Meshes.IsValid(handle));

        uint64_t requiredVertexCount = m_GlobalVertexCount + vertices.size();
        uint64_t requiredIndexCount = m_GlobalIndexCount + indices.size();

        /*RTEnsureGlobalBufferCapacity<Vertex>(
            m_GlobalVertexBuffer,
            requiredVertexCount,
            m_GlobalVertexCapacity,
            m_GlobalVertexBuffer->GetUsage()
        );

        RTEnsureGlobalBufferCapacity<uint32_t>(
            m_GlobalIndexBuffer,
            requiredIndexCount,
            m_GlobalIndexCapacity,
            m_GlobalIndexBuffer->GetUsage()
        );*/

        if (!vertices.empty()) {
            const auto& vertexBuffer = m_RenderDevice->AccessResource<RenderDeviceBuffer>(m_GlobalVertexBuffer);
            vertexBuffer->RTLoadToDevice(m_RenderDevice, vertices.data(), vertices.size() * sizeof(Vertex), m_GlobalVertexCount * sizeof(Vertex));
        }

        if (!indices.empty()) {
            const auto& indexBuffer = m_RenderDevice->AccessResource<RenderDeviceBuffer>(m_GlobalIndexBuffer);
            indexBuffer->RTLoadToDevice(m_RenderDevice, indices.data(), indices.size() * sizeof(uint32_t), m_GlobalIndexCount * sizeof(uint32_t));
        }

        using Index = RenderDeviceObjectHandle::IndexType;

        Index firstSubmeshIndex = INVALID_INDEX;
        Index registeredSubmeshCount = 0;

        const auto CalculateSubmeshBoundingSphere = [&](const Submesh& submesh) {
            if (submesh.VertexCount == 0)
                return glm::vec4(0.0f);

            glm::vec3 minimum{ std::numeric_limits<float>::max() };
            glm::vec3 maximum{ std::numeric_limits<float>::lowest() };

            for (uint32_t vertexIndex = 0; vertexIndex < submesh.VertexCount; vertexIndex++) {
                const glm::vec3 position = vertices[submesh.BaseVertexCount + vertexIndex].Position;

                minimum = glm::min(minimum, position);
                maximum = glm::max(maximum, position);
            }

            const glm::vec3 center = (minimum + maximum) * 0.5f;
            float radius = 0.0f;

            for (uint32_t vertexIndex = 0; vertexIndex < submesh.VertexCount; vertexIndex++) {
                const glm::vec3 position = vertices[submesh.BaseVertexCount + vertexIndex].Position;
                radius = glm::max(radius, glm::distance(center, position));
            }

            return glm::vec4(center, radius);
        };

        for (const Submesh& submesh : submeshes) {
            LUCY_ASSERT(!submesh.LODs.empty(), "Submesh requires at least one LOD.");

            const SubmeshLOD& baseLOD = submesh.LODs[0];

            RenderDeviceSubmeshData renderDeviceSubmesh{};
            renderDeviceSubmesh.Transform = submesh.Transform;
            renderDeviceSubmesh.TransformInversedTransposed = glm::transpose(glm::inverse(submesh.Transform));
            renderDeviceSubmesh.BoundingSphere = CalculateSubmeshBoundingSphere(submesh);
            renderDeviceSubmesh.Draw.x = m_GlobalIndexCount + submesh.BaseMeshletIndexCount + baseLOD.FirstMeshletIndex;
            renderDeviceSubmesh.Draw.y = baseLOD.MeshletIndexCount;
            renderDeviceSubmesh.Draw.z = static_cast<int32_t>(m_GlobalVertexCount + submesh.BaseVertexCount);
            renderDeviceSubmesh.Draw.w = submesh.MaterialID.Index;
            renderDeviceSubmesh.Meshlets.x = INVALID_INDEX;
            renderDeviceSubmesh.Meshlets.y = 0;
            renderDeviceSubmesh.LODs.x = INVALID_INDEX;
            renderDeviceSubmesh.LODs.y = 0;

            const RenderDeviceObjectHandle submeshHandle = RTRegisterSubmesh(renderDeviceSubmesh);

            if (firstSubmeshIndex == INVALID_INDEX)
                firstSubmeshIndex = submeshHandle.Index;
            else
                LUCY_ASSERT(submeshHandle.Index == firstSubmeshIndex + registeredSubmeshCount, "Submeshes must be contiguous.");

            Index firstSubmeshLODIndex = INVALID_INDEX;
            Index registeredSubmeshLODCount = 0;

            Index firstBaseLODMeshletIndex = INVALID_INDEX;
            Index registeredBaseLODMeshletCount = 0;

            for (uint32_t lodIndex = 0; lodIndex < submesh.LODs.size(); lodIndex++) {
                const SubmeshLOD& submeshLOD = submesh.LODs[lodIndex];

                Index firstLODMeshletIndex = INVALID_INDEX;
                Index registeredLODMeshletCount = 0;

                for (uint32_t meshletOffset = 0; meshletOffset < submeshLOD.MeshletCount; meshletOffset++) {
                    const Meshlet& meshlet = submesh.Meshlets[submeshLOD.FirstMeshlet + meshletOffset];

                    RenderDeviceMeshletData renderDeviceMeshlet{};
                    renderDeviceMeshlet.BoundingSphere = meshlet.BoundingSphere;
                    renderDeviceMeshlet.NormalCone = meshlet.NormalCone;
                    renderDeviceMeshlet.Draw.x = m_GlobalIndexCount + submesh.BaseMeshletIndexCount + meshlet.FirstIndex;
                    renderDeviceMeshlet.Draw.y = meshlet.IndexCount;
                    renderDeviceMeshlet.Draw.z = static_cast<int32_t>(m_GlobalVertexCount + submesh.BaseVertexCount);
                    renderDeviceMeshlet.Draw.w = submeshHandle.Index;

                    const RenderDeviceObjectHandle meshletHandle = RTRegisterMeshlet(renderDeviceMeshlet);

                    if (firstLODMeshletIndex == INVALID_INDEX)
                        firstLODMeshletIndex = meshletHandle.Index;
                    else
                        LUCY_ASSERT(meshletHandle.Index == firstLODMeshletIndex + registeredLODMeshletCount, "LOD meshlets must be contiguous.");

                    registeredLODMeshletCount++;
                }

                RenderDeviceMeshLODData renderDeviceMeshLOD{};
                renderDeviceMeshLOD.Meshlets.x = firstLODMeshletIndex;
                renderDeviceMeshLOD.Meshlets.y = registeredLODMeshletCount;
                renderDeviceMeshLOD.LODData.x = submeshLOD.MinimumProjectedRadius;

                const RenderDeviceObjectHandle meshLODHandle = RTRegisterMeshLOD(renderDeviceMeshLOD);

                if (firstSubmeshLODIndex == INVALID_INDEX)
                    firstSubmeshLODIndex = meshLODHandle.Index;
                else
                    LUCY_ASSERT(meshLODHandle.Index == firstSubmeshLODIndex + registeredSubmeshLODCount, "Submesh LODs must be contiguous.");

                if (lodIndex == 0) {
                    firstBaseLODMeshletIndex = firstLODMeshletIndex;
                    registeredBaseLODMeshletCount = registeredLODMeshletCount;
                }

                registeredSubmeshLODCount++;
            }

            RenderDeviceSubmeshData& registeredSubmesh = m_Submeshes.Get(submeshHandle);
            registeredSubmesh.Meshlets.x = firstBaseLODMeshletIndex;
            registeredSubmesh.Meshlets.y = registeredBaseLODMeshletCount;
            registeredSubmesh.LODs.x = firstSubmeshLODIndex;
            registeredSubmesh.LODs.y = registeredSubmeshLODCount;

            RTEnqueueUpdate(m_Submeshes, RenderDeviceSceneBufferType::Submeshes, submeshHandle);

            registeredSubmeshCount++;
        }

        const auto CalculateMeshBoundingSphere = [](const auto& vertices, const auto& submeshes) {
            if (vertices.empty())
                return glm::vec4(0.0f);

            glm::vec3 minimum{ std::numeric_limits<float>::max() };
            glm::vec3 maximum{ std::numeric_limits<float>::lowest() };

            for (const Submesh& submesh : submeshes) {
                for (uint32_t vertexIndex = 0; vertexIndex < submesh.VertexCount; vertexIndex++) {
                    const Vertex& vertex = vertices[submesh.BaseVertexCount + vertexIndex];
                    const glm::vec3 position = glm::vec3(submesh.Transform * glm::vec4(vertex.Position, 1.0f));

                    minimum = glm::min(minimum, position);
                    maximum = glm::max(maximum, position);
                }
            }

            const glm::vec3 center = (minimum + maximum) * 0.5f;
            float radius = 0.0f;

            for (const Submesh& submesh : submeshes) {
                for (uint32_t vertexIndex = 0; vertexIndex < submesh.VertexCount; vertexIndex++) {
                    const Vertex& vertex = vertices[submesh.BaseVertexCount + vertexIndex];
                    const glm::vec3 position = glm::vec3(submesh.Transform * glm::vec4(vertex.Position, 1.0f));

                    radius = glm::max(radius, glm::distance(center, position));
                }
            }

            return glm::vec4{ center, radius };
        };

        RenderDeviceMeshData& renderDeviceMesh = m_Meshes.Get(handle);
        renderDeviceMesh.BoundingSphere = CalculateMeshBoundingSphere(vertices, submeshes);
        renderDeviceMesh.Data.x = INVALID_INDEX;
        renderDeviceMesh.Data.y = 0;
        renderDeviceMesh.Data.z = firstSubmeshIndex;
        renderDeviceMesh.Data.w = registeredSubmeshCount;

        RTEnqueueUpdate(m_Meshes, RenderDeviceSceneBufferType::Meshes, handle);

        m_GlobalVertexCount = requiredVertexCount;
        m_GlobalIndexCount = requiredIndexCount;
    }
    
    void RenderDeviceScene::RTRegisterPBRMaterial(const RenderDeviceObjectHandle& handle, const RenderDevicePBRMaterialData& data) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTRegisterPBRMaterial");
        LUCY_ASSERT(Renderer::IsOnRenderThread());
        RTEnqueueUpdate(m_PBRMaterials, RenderDeviceSceneBufferType::Materials, handle);
    }

    RenderDeviceObjectHandle RenderDeviceScene::RTRegisterSubmesh(const RenderDeviceSubmeshData& data) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTRegisterSubmesh");
        LUCY_ASSERT(Renderer::IsOnRenderThread());

        const RenderDeviceObjectHandle& handle = m_Submeshes.Create(data);
        RTEnqueueUpdate(m_Submeshes, RenderDeviceSceneBufferType::Submeshes, handle);

        return handle;
    }

    RenderDeviceObjectHandle RenderDeviceScene::RTRegisterMeshlet(const RenderDeviceMeshletData& data) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTRegisterMeshlet");
        LUCY_ASSERT(Renderer::IsOnRenderThread());

        const RenderDeviceObjectHandle& handle = m_Meshlets.Create(data);
        RTEnqueueUpdate(m_Meshlets, RenderDeviceSceneBufferType::Meshlets, handle);

        return handle;
    }

    RenderDeviceObjectHandle RenderDeviceScene::RTRegisterMeshLOD(const RenderDeviceMeshLODData& data) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTRegisterMeshLOD");
        LUCY_ASSERT(Renderer::IsOnRenderThread());

        const RenderDeviceObjectHandle& handle = m_MeshLODs.Create(data);
        RTEnqueueUpdate(m_MeshLODs, RenderDeviceSceneBufferType::MeshLODs, handle);

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
            addresses.Objects = m_RenderDevice->AccessResource<RenderDeviceBuffer>(frameData.ObjectsBuffer)->GetDeviceAddress();
            addresses.Meshes = m_RenderDevice->AccessResource<RenderDeviceBuffer>(frameData.MeshBuffer)->GetDeviceAddress();
            addresses.Submeshes = m_RenderDevice->AccessResource<RenderDeviceBuffer>(frameData.SubmeshBuffer)->GetDeviceAddress();
            addresses.Meshlets = m_RenderDevice->AccessResource<RenderDeviceBuffer>(frameData.MeshletBuffer)->GetDeviceAddress();
            addresses.CullViews = m_RenderDevice->AccessResource<RenderDeviceBuffer>(frameData.CullViewsBuffer)->GetDeviceAddress();
            addresses.MeshLODs = m_RenderDevice->AccessResource<RenderDeviceBuffer>(frameData.MeshLODBuffer)->GetDeviceAddress();
            addresses.Vertices = m_RenderDevice->AccessResource<RenderDeviceBuffer>(m_GlobalVertexBuffer)->GetDeviceAddress();
            addresses.Indices = m_RenderDevice->AccessResource<RenderDeviceBuffer>(m_GlobalIndexBuffer)->GetDeviceAddress();

            constexpr size_t addressesOffset = offsetof(RenderDeviceSceneGlobalData, Addresses);

            const auto& globalsBuffer = m_RenderDevice->AccessResource<RenderDeviceBuffer>(frameData.GlobalsBuffer);
            globalsBuffer->RTLoadToDevice(m_RenderDevice, std::addressof(addresses), sizeof(addresses), addressesOffset);
        };

        bool materialBufferResized = RTEnsureDeviceAddressBufferCapacity<RenderDevicePBRMaterialData>(frameData.MaterialBuffer);
        bool meshBufferResized = RTEnsureDeviceAddressBufferCapacity<RenderDeviceMeshData>(frameData.MeshBuffer);
        bool submeshBufferResized = RTEnsureDeviceAddressBufferCapacity<RenderDeviceSubmeshData>(frameData.SubmeshBuffer);
        bool meshletBufferResized = RTEnsureDeviceAddressBufferCapacity<RenderDeviceMeshletData>(frameData.MeshletBuffer);
        bool meshLODBufferResized = RTEnsureDeviceAddressBufferCapacity<RenderDeviceMeshLODData>(frameData.MeshLODBuffer);
        bool cullViewsBufferResized = RTEnsureDeviceAddressBufferCapacity<RenderDeviceCullViewData>(frameData.CullViewsBuffer);

        if (materialBufferResized) {
            RTUploadEntirePool(m_PBRMaterials, frameData.MaterialBuffer);
            std::erase_if(frameData.PendingUpdates, [&](const RenderDeviceScenePendingUpdate& update) {
                return update.BufferType == RenderDeviceSceneBufferType::Materials;
            });
        }

        if (cullViewsBufferResized) {
            RTUploadEntirePool(m_CullViews, frameData.CullViewsBuffer);

            std::erase_if(frameData.PendingUpdates, [&](const RenderDeviceScenePendingUpdate& update) {
                return update.BufferType == RenderDeviceSceneBufferType::CullViews;
            });
        }

        if (meshBufferResized) {
            RTUploadEntirePool(m_Meshes, frameData.MeshBuffer);
            std::erase_if(frameData.PendingUpdates, [&](const RenderDeviceScenePendingUpdate& update) {
                return update.BufferType == RenderDeviceSceneBufferType::Meshes;
            });
        }

        if (submeshBufferResized) {
            RTUploadEntirePool(m_Submeshes, frameData.SubmeshBuffer);
            std::erase_if(frameData.PendingUpdates, [&](const RenderDeviceScenePendingUpdate& update) {
                return update.BufferType == RenderDeviceSceneBufferType::Submeshes;
            });
        }
        
        if (meshletBufferResized) {
            RTUploadEntirePool(m_Meshlets, frameData.MeshletBuffer);
            std::erase_if(frameData.PendingUpdates, [&](const RenderDeviceScenePendingUpdate& update) {
                return update.BufferType == RenderDeviceSceneBufferType::Meshlets;
            });
        }

        if (meshLODBufferResized) {
            RTUploadEntirePool(m_MeshLODs, frameData.MeshLODBuffer);

            std::erase_if(frameData.PendingUpdates, [&](const RenderDeviceScenePendingUpdate& update) {
                return update.BufferType == RenderDeviceSceneBufferType::MeshLODs;
            });
        }

        if (!frameData.Initialized) {
            UploadAllAddressesToGlobalBuffer();
            frameData.Initialized = true;
        }

        if (materialBufferResized || meshBufferResized || meshLODBufferResized || submeshBufferResized || meshletBufferResized || cullViewsBufferResized)
            UploadAllAddressesToGlobalBuffer();

        if (frameData.PendingUpdates.empty())
            return;

        RTApplyPendingUpdates(frameIndex);
    }

	void RenderDeviceScene::RTDestroy() {
        LUCY_ASSERT(Renderer::IsOnRenderThread());

        for (auto& frameData : m_FrameData) {
            Renderer::EnqueueResourceDestroy(frameData.GlobalsBuffer);
            Renderer::EnqueueResourceDestroy(frameData.ObjectsBuffer);
            Renderer::EnqueueResourceDestroy(frameData.MaterialBuffer);
            Renderer::EnqueueResourceDestroy(frameData.MeshBuffer);
            Renderer::EnqueueResourceDestroy(frameData.MeshLODBuffer);
            Renderer::EnqueueResourceDestroy(frameData.CullViewsBuffer);
            Renderer::EnqueueResourceDestroy(frameData.SubmeshBuffer);
            Renderer::EnqueueResourceDestroy(frameData.MeshletBuffer);
        }

        Renderer::EnqueueResourceDestroy(m_GlobalVertexBuffer);
        Renderer::EnqueueResourceDestroy(m_GlobalIndexBuffer);

        m_Globals.Clear();
        m_PBRMaterials.Clear();
        m_CullViews.Clear();
        m_Meshes.Clear();
        m_MeshLODs.Clear();
        m_Submeshes.Clear();
        m_Meshlets.Clear();

        m_GlobalVertexCount = 0;
        m_GlobalIndexCount = 0;
	}
}