#include "lypch.h"
#include "RenderDeviceScene.h"

#include "Renderer/Memory/VulkanAllocator.h"

#include "Renderer/Pipeline/RayTracingPipeline.h"

namespace Lucy {

    RenderDeviceScene::RenderDeviceScene(RenderDevice* device)
        : m_RenderDevice(device) {
        const uint32_t maxFramesInFlight = Renderer::GetMaxFramesInFlight();
        m_FrameData.resize(maxFramesInFlight);

        /*
        * I am allocating here a large chunk of memory... i guess so around 600mb... lets say 1gb... in the future, make it so that it can grow...
        */

        for (auto& frameData : m_FrameData) {
            frameData.GlobalsBuffer = device->CreateDeviceAddressBuffer({ "Globalsbuffer", sizeof(RenderDeviceSceneGlobalData), BufferUsage::None, MemoryUsage::GPUOnly });
            frameData.ObjectsBuffer = device->CreateDeviceAddressBuffer({ "ObjectBuffer", s_ObjectCapacity * sizeof(RenderDeviceObjectData), BufferUsage::None, MemoryUsage::GPUOnly });
            frameData.MaterialBuffer = device->CreateDeviceAddressBuffer({ "Materialbuffer", s_MaterialCapacity * sizeof(RenderDevicePBRMaterialData), BufferUsage::None, MemoryUsage::GPUOnly });
            frameData.MeshBuffer = device->CreateDeviceAddressBuffer({ "Meshbuffer", s_MeshCapacity * sizeof(RenderDeviceMeshData), BufferUsage::None, MemoryUsage::GPUOnly });
            frameData.MeshLODBuffer = device->CreateDeviceAddressBuffer({ "MeshLODBuffer", 4 * s_SubmeshCapacity * sizeof(RenderDeviceMeshLODData), BufferUsage::None, MemoryUsage::GPUOnly });
            frameData.SubmeshBuffer = device->CreateDeviceAddressBuffer({ "Submeshbuffer", s_SubmeshCapacity * sizeof(RenderDeviceSubmeshData), BufferUsage::None, MemoryUsage::GPUOnly });
            frameData.MeshletBuffer = device->CreateDeviceAddressBuffer({ "Meshletbuffer", s_MeshletCapacity * sizeof(RenderDeviceMeshletData), BufferUsage::None, MemoryUsage::GPUOnly });
            frameData.CullViewsBuffer = device->CreateDeviceAddressBuffer({ "CullViewsBuffer", 10 * sizeof(RenderDeviceCullViewData), BufferUsage::None, MemoryUsage::GPUOnly });
        }

        std::vector<RenderDeviceObjectData> emptyObjects(s_ObjectCapacity);
        for (auto& object : emptyObjects)
            object.Data = { INVALID_INDEX, static_cast<uint32_t>(RenderDeviceObjectFlags::None), 0, 0 };
        
        for (auto& frameData : m_FrameData) {
            const auto& buffer = m_RenderDevice->AccessResource<RenderDeviceBuffer>(frameData.ObjectsBuffer);
			buffer->RTLoadToDevice(m_RenderDevice, reinterpret_cast<const void*>(emptyObjects.data()), sizeof(RenderDeviceObjectData) * emptyObjects.size(), 0);
        }

        m_GlobalVertexBuffer = device->CreateDeviceAddressBuffer({ "GlobalVertexBuffer", s_GlobalVertexCapacity * sizeof(Vertex),
            BufferUsage::Storage | BufferUsage::TransferDestination | BufferUsage::TransferSource | BufferUsage::AccelerationStructureBuildInput, MemoryUsage::GPUOnly });

        m_GlobalIndexBuffer = device->CreateDeviceAddressBuffer({ "GlobalIndexBuffer", s_GlobalIndexCapacity * sizeof(uint32_t),
            BufferUsage::Storage | BufferUsage::Index | BufferUsage::TransferDestination | BufferUsage::TransferSource | BufferUsage::AccelerationStructureBuildInput, MemoryUsage::GPUOnly });

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

    RenderDeviceObjectHandle RenderDeviceScene::RTRegisterMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, std::vector<Submesh>& submeshes) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTRegisterMesh");
        const RenderDeviceObjectHandle& handle = m_Meshes.Create(RenderDeviceMeshData{}); //uninitialized
        RTRegisterMesh(handle, vertices, indices, submeshes);
        return handle;
    }

    RenderDeviceObjectHandle RenderDeviceScene::RTRegisterObject(const RenderDeviceObjectHandle& meshHandle, const glm::mat4& transform, RenderDeviceObjectFlags flags) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTRegisterObject");
        RenderDeviceObjectData data{};
        data.Transform = transform;
        data.TransformInversedTransposed = glm::transpose(glm::inverse(transform));
        data.PreviousTransform = transform;
        data.Data.x = meshHandle.Index;
        data.Data.y = static_cast<uint32_t>(flags) | static_cast<uint32_t>(RenderDeviceObjectFlags::Alive);

        const RenderDeviceObjectHandle& handle = m_Objects.Create(data);
        RTRegisterObject(handle);
        return handle;
    }

    RenderDeviceObjectHandle RenderDeviceScene::RegisterCullView(const RenderDeviceCullViewData& data) {
		LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RegisterCullView");
		const RenderDeviceObjectHandle& handle = m_CullViews.Create(data);
        Renderer::EnqueueToRenderCommandQueue([this, handle](const auto& device) {
            RTRegisterCullView(handle);
        });
		return handle;
    }

    void RenderDeviceScene::RTRegisterObject(const RenderDeviceObjectHandle& handle) {
        LUCY_PROFILE_NEW_EVENT("RenderDeviceScene::RTRegisterObject");
        LUCY_ASSERT(Renderer::IsOnRenderThread());
        RTEnqueueUpdate(m_Objects, RenderDeviceSceneBufferType::Objects, handle);

        for (auto& frameData : m_FrameData) {
            frameData.TopLevelAccelerationStructureDirty = true;
            frameData.TopLevelAccelerationStructureRebuild = true;
        }
    }

    RenderDeviceObjectHandle RenderDeviceScene::RegisterPBRMaterial(const RenderDevicePBRMaterialData& data) {
        const RenderDeviceObjectHandle& handle = m_PBRMaterials.Create(data);
        Renderer::EnqueueToRenderCommandQueue([this, handle, data](const auto& device) {
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
        if (currentData == data)
            return;
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

        for (auto& frameData : m_FrameData)
            frameData.TopLevelAccelerationStructureDirty = true;

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

        for (const Submesh& submesh : submeshes) {
            LUCY_ASSERT(!submesh.LODs.empty(), "Submesh requires at least one LOD.");

            const SubmeshLOD& baseLOD = submesh.LODs[0];

            RenderDeviceSubmeshData renderDeviceSubmesh{};
            renderDeviceSubmesh.Transform = submesh.Transform;
            renderDeviceSubmesh.TransformInversedTransposed = glm::transpose(glm::inverse(submesh.Transform));
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
                    renderDeviceMeshlet.AABBCenter = meshlet.AABBCenter;
                    renderDeviceMeshlet.AABBExtents = meshlet.AABBExtents;
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
                renderDeviceMeshLOD.LODData.x = submeshLOD.Error;

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

            //TODO: JUST CLEAN THIS UP... YOU HAVE MULTIPLE CALCS
            RenderDeviceSubmeshData& registeredSubmesh = m_Submeshes.Get(submeshHandle);
            registeredSubmesh.Meshlets.x = firstBaseLODMeshletIndex;
            registeredSubmesh.Meshlets.y = registeredBaseLODMeshletCount;
            registeredSubmesh.LODs.x = firstSubmeshLODIndex;
            registeredSubmesh.LODs.y = registeredSubmeshLODCount;

            glm::vec3 minimum{ std::numeric_limits<float>::max() };
            glm::vec3 maximum{ std::numeric_limits<float>::lowest() };

            for (const Meshlet& meshlet : submesh.Meshlets) {
                const glm::vec3 meshletMinimum = meshlet.AABBCenter - meshlet.AABBExtents;
                const glm::vec3 meshletMaximum = meshlet.AABBCenter + meshlet.AABBExtents;

                minimum = glm::min(minimum, meshletMinimum);
                maximum = glm::max(maximum, meshletMaximum);
            }

            registeredSubmesh.AABBCenter = (minimum + maximum) * 0.5f;
            registeredSubmesh.AABBExtents = (maximum - minimum) * 0.5f;

            float radius = 0.0f;
            for (uint32_t vertexIndex = 0; vertexIndex < submesh.VertexCount; vertexIndex++) {
                const glm::vec3& position = vertices[submesh.BaseVertexCount + vertexIndex].Position;
                radius = glm::max(radius, glm::distance(registeredSubmesh.AABBCenter, position));
            }
            registeredSubmesh.BoundingSphere = glm::vec4{ registeredSubmesh.AABBCenter, radius };

            RTEnqueueUpdate(m_Submeshes, RenderDeviceSceneBufferType::Submeshes, submeshHandle);

            registeredSubmeshCount++;
        }

        const auto CalculateMeshCenterAndExtents = [](const auto& vertices, const auto& submeshes) {
            if (vertices.empty())
                return std::make_pair(glm::vec3{}, glm::vec3{});

            glm::vec3 minimum{ std::numeric_limits<float>::max() };
            glm::vec3 maximum{ std::numeric_limits<float>::lowest() };

            for (const Submesh& submesh : submeshes) {
                for (uint32_t vertexIndex = 0; vertexIndex < submesh.VertexCount; vertexIndex++) {
                    const Vertex& vertex = vertices[submesh.BaseVertexCount + vertexIndex];

                    const glm::vec3 position = glm::vec3(
                        submesh.Transform * glm::vec4(vertex.Position, 1.0f)
                    );

                    minimum = glm::min(minimum, position);
                    maximum = glm::max(maximum, position);
                }
            }

            const glm::vec3 center = (minimum + maximum) * 0.5f;
            const glm::vec3 extents = (maximum - minimum) * 0.5f;

            return std::make_pair(center, extents);
        };

        const auto [center, extents] = CalculateMeshCenterAndExtents(vertices, submeshes);

        RenderDeviceMeshData& renderDeviceMesh = m_Meshes.Get(handle);
        renderDeviceMesh.AABBCenter = center;
        renderDeviceMesh.AABBExtents = extents;
        renderDeviceMesh.Data.x = INVALID_INDEX;
        renderDeviceMesh.Data.y = 0;
        renderDeviceMesh.Data.z = firstSubmeshIndex;
        renderDeviceMesh.Data.w = registeredSubmeshCount;

        RTEnqueueUpdate(m_Meshes, RenderDeviceSceneBufferType::Meshes, handle);
        //sometimes we are registering meshes with no submeshes (like the cube mesh for skybox)
        if (!submeshes.empty())
		    RTCreateMeshBottomLevelAccelerationStructure(handle, submeshes);

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

    void RenderDeviceScene::RTCreateMeshBottomLevelAccelerationStructure(const RenderDeviceObjectHandle& meshHandle, const std::vector<Submesh>& submeshes) {
        LUCY_ASSERT(Renderer::IsOnRenderThread());

        const auto& vertexBuffer = m_RenderDevice->AccessResource<RenderDeviceBuffer>(m_GlobalVertexBuffer);
        const auto& indexBuffer = m_RenderDevice->AccessResource<RenderDeviceBuffer>(m_GlobalIndexBuffer);

        BLAccelerationStructureCreateInfo createInfo {
			.Geometries = {},
        };
        createInfo.Geometries.reserve(submeshes.size());

        for (const Submesh& submesh : submeshes) {
            LUCY_ASSERT(!submesh.LODs.empty());
            const SubmeshLOD& baseLOD = submesh.LODs[0];

            AccelerationStructureGeometry& geometry = createInfo.Geometries.emplace_back();
            geometry.VertexAddress = vertexBuffer->GetDeviceAddress() + (m_GlobalVertexCount + submesh.BaseVertexCount) * sizeof(Vertex) + offsetof(Vertex, Position);
            geometry.IndexAddress = indexBuffer->GetDeviceAddress() + (m_GlobalIndexCount + submesh.BaseMeshletIndexCount + baseLOD.FirstMeshletIndex) * sizeof(uint32_t);
            geometry.Transform = submesh.Transform;
            geometry.VertexStride = sizeof(Vertex);
            geometry.VertexCount = submesh.VertexCount;
            geometry.PrimitiveCount = baseLOD.MeshletIndexCount / 3;
        }

        if (meshHandle.Index >= m_MeshBLAccelerationStructures.size())
            m_MeshBLAccelerationStructures.resize(meshHandle.Index + 1);
        m_MeshBLAccelerationStructures[meshHandle.Index] = m_RenderDevice->CreateBLAccelerationStructure(createInfo);
    }

    void RenderDeviceScene::RTSyncTopLevelAccelerationStructure(uint32_t frameIndex) {
        auto& frameData = m_FrameData[frameIndex];
        if (!frameData.TopLevelAccelerationStructureDirty)
            return;
        
        std::vector<AccelerationStructureInstance> instances;
        ForEachAlive(m_Objects, [&](RenderDeviceObjectHandle handle, const RenderDeviceObjectData& object) {
            uint32_t meshIndex = object.Data.x;
			//guard bcs sometimes meshes do not have any submesh... so we dont have a BLAS for them... so lets skip for tlas as well
            if (meshIndex >= m_MeshBLAccelerationStructures.size())
                return;
            const auto& blas = m_RenderDevice->AccessResource<AccelerationStructure>(m_MeshBLAccelerationStructures[meshIndex]);

            AccelerationStructureInstance& instance = instances.emplace_back();
            instance.BottomLevelAccelerationStructureAddress = blas->GetDeviceAddress();
            instance.Transform = object.Transform;
            instance.CustomIndex = static_cast<uint32_t>(handle.Index);
            instance.ShaderBindingTableRecordOffset = 0;
            instance.Mask = 0xFF;
        });

        if (instances.empty()) {
            frameData.TopLevelAccelerationStructureDirty = false;
            frameData.TopLevelAccelerationStructureRebuild = false;
            return;
        }

        TLAccelerationStructureCreateInfo createInfo{ .Instances = std::move(instances) };

        if (!frameData.TopLevelAccelerationStructure || frameData.TopLevelAccelerationStructureRebuild) {
            if (frameData.TopLevelAccelerationStructure)
                m_RenderDevice->RTDestroyResource(frameData.TopLevelAccelerationStructure);

            frameData.TopLevelAccelerationStructure = m_RenderDevice->CreateTLAccelerationStructure(createInfo);
        } else {
			m_RenderDevice->AccessResource<AccelerationStructure>(frameData.TopLevelAccelerationStructure)->RTUpdate(m_RenderDevice, createInfo);
        }

        frameData.TopLevelAccelerationStructureDirty = false;
        frameData.TopLevelAccelerationStructureRebuild = false;
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

        if (!frameData.PendingUpdates.empty())
            RTApplyPendingUpdates(frameIndex);

		RTSyncTopLevelAccelerationStructure(frameIndex);
    }

	void RenderDeviceScene::RTDestroy() {
        LUCY_ASSERT(Renderer::IsOnRenderThread());

		for (auto& accelerationStructure : m_MeshBLAccelerationStructures)
			Renderer::EnqueueResourceDestroy(accelerationStructure);

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
        m_MeshBLAccelerationStructures.clear();

        m_GlobalVertexCount = 0;
        m_GlobalIndexCount = 0;
	}
}