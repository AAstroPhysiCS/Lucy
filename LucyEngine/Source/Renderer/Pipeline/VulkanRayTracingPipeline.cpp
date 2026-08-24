#include "lypch.h"
#include "VulkanRayTracingPipeline.h"

#include "Renderer/Renderer.h"
#include "Renderer/Context/VulkanContext.h"
#include "Renderer/Device/VulkanRenderDevice.h"

#include "Renderer/Shader/VulkanRayTracingShader.h"
#include "Renderer/Descriptors/VulkanDescriptorSet.h"

#include "Renderer/Memory/Buffer/Vulkan/VulkanDeviceAddressBuffer.h"

#include "vulkan/vulkan.h"

namespace Lucy {

	//GLM is column-major, but Vulkan expects row-major matrices
	//i could just transpose the matrix but again it expects a 3x4 matrix so i have to do this anyway
	static inline VkTransformMatrixKHR TransformToVulkanMatrix(const glm::mat4& transform) {
		VkTransformMatrixKHR vkTransform{};
		for (uint32_t i = 0; i < 3; i++) {
			for (uint32_t j = 0; j < 4; j++) {
				vkTransform.matrix[i][j] = transform[j][i];
			}
		}
		return vkTransform;
	};

	VulkanAccelerationStructure::VulkanAccelerationStructure(const BLAccelerationStructureCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device)
		: AccelerationStructure(createInfo) {
		RTCreateBottomLevel(device);
	}

	VulkanAccelerationStructure::VulkanAccelerationStructure(const TLAccelerationStructureCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device)
		: AccelerationStructure(createInfo) {
		RTCreateTopLevel(device);
	}

	void VulkanAccelerationStructure::RTUpdate(RenderDevice* device, const TLAccelerationStructureCreateInfo& createInfo) {
		LUCY_ASSERT(Renderer::IsOnRenderThread());
		LUCY_ASSERT(m_AccelerationStructure != VK_NULL_HANDLE);

		auto* vulkanDevice = reinterpret_cast<VulkanRenderDevice*>(device);

		std::vector<VkAccelerationStructureInstanceKHR> vulkanInstances = CreateVulkanInstances(createInfo);

		const auto& instanceBuffer = vulkanDevice->AccessResource<RenderDeviceBuffer>(m_InstanceBufferHandle);
		LUCY_ASSERT(sizeof(VkAccelerationStructureInstanceKHR) * vulkanInstances.size() == instanceBuffer->GetSize(), "TLAS instance count cannot change during an update");

		instanceBuffer->RTLoadToDevice(vulkanDevice, vulkanInstances.data(), sizeof(VkAccelerationStructureInstanceKHR) * vulkanInstances.size());

		vulkanDevice->GetUploadManager()->SyncFrame(Renderer::GetCurrentFrameIndex());

		VkAccelerationStructureGeometryInstancesDataKHR instanceData{};
		instanceData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		instanceData.arrayOfPointers = VK_FALSE;
		instanceData.data.deviceAddress = instanceBuffer->GetDeviceAddress();

		VkAccelerationStructureGeometryKHR geometry{};
		geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		geometry.geometry.instances = instanceData;

		VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
		buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
		buildInfo.srcAccelerationStructure = m_AccelerationStructure;
		buildInfo.dstAccelerationStructure = m_AccelerationStructure;
		buildInfo.geometryCount = 1;
		buildInfo.pGeometries = &geometry;

		const auto& scratchBuffer = vulkanDevice->AccessResource<RenderDeviceBuffer>(m_ScratchBufferHandle);
		buildInfo.scratchData.deviceAddress = scratchBuffer->GetDeviceAddress();

		VkAccelerationStructureBuildRangeInfoKHR range{};
		range.primitiveCount = static_cast<uint32_t>(vulkanInstances.size());
		range.primitiveOffset = 0;
		range.firstVertex = 0;
		range.transformOffset = 0;

		const VkAccelerationStructureBuildRangeInfoKHR* rangePointer = &range;

		vulkanDevice->SubmitImmediateCommand([=](VkCommandBuffer commandBuffer) {
			VulkanExternalFuncLinkage::vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildInfo, &rangePointer);
		});
	}

	void VulkanAccelerationStructure::RTCreateBottomLevel(const Ref<VulkanRenderDevice>& device) {
		LUCY_ASSERT(Renderer::IsOnRenderThread());
		const auto& createInfo = GetBLCreateInfo();

		std::vector<VkTransformMatrixKHR> transforms;
		transforms.reserve(createInfo.Geometries.size());
		for (const auto& geometry : createInfo.Geometries)
			transforms.emplace_back(TransformToVulkanMatrix(geometry.Transform));

		RenderDeviceResourceHandle transformBufferHandle = device->CreateDeviceAddressBuffer({ 
			.DebugName = GetDebugName() + " TransformBuffer",
			.Size = transforms.size() * sizeof(VkTransformMatrixKHR), 
			.Usage = BufferUsage::AccelerationStructureBuildInput, 
			.MemoryUsage = MemoryUsage::GPUOnly 
		});

		const auto& transformBuffer = device->AccessResource<RenderDeviceBuffer>(transformBufferHandle);
		transformBuffer->RTLoadToDevice(device.get(), transforms.data(), transforms.size() * sizeof(VkTransformMatrixKHR));

		//upload it
		device->GetUploadManager()->SyncFrame(Renderer::GetCurrentFrameIndex());

		std::vector<VkAccelerationStructureGeometryKHR> geometries;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges;
		std::vector<uint32_t> primitiveCounts;

		geometries.resize(createInfo.Geometries.size());
		ranges.resize(createInfo.Geometries.size());
		primitiveCounts.resize(createInfo.Geometries.size());

		for (uint32_t i = 0; i < createInfo.Geometries.size(); i++) {
			const auto& source = createInfo.Geometries[i];
			LUCY_ASSERT(source.VertexAddress != 0);
			LUCY_ASSERT(source.IndexAddress != 0);
			LUCY_ASSERT(source.VertexCount > 0);
			LUCY_ASSERT(source.PrimitiveCount > 0);

			VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
			triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
			triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
			triangles.vertexData.deviceAddress = source.VertexAddress;
			triangles.vertexStride = source.VertexStride;
			triangles.maxVertex = source.VertexCount - 1;

			triangles.indexType = VK_INDEX_TYPE_UINT32;
			triangles.indexData.deviceAddress = source.IndexAddress;

			triangles.transformData.deviceAddress = transformBuffer->GetDeviceAddress() + i * sizeof(VkTransformMatrixKHR);

			VkAccelerationStructureGeometryKHR geometry{};
			geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
			geometry.geometry.triangles = triangles;
			geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

			geometries[i] = geometry;

			VkAccelerationStructureBuildRangeInfoKHR range{};
			range.primitiveCount = source.PrimitiveCount;
			range.primitiveOffset = 0;
			range.firstVertex = 0;
			range.transformOffset = 0;

			ranges[i] = range;
			primitiveCounts[i] = source.PrimitiveCount;
		}

		VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
		buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;

		buildInfo.geometryCount = static_cast<uint32_t>(geometries.size());
		buildInfo.pGeometries = geometries.data();

		VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
		sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		VulkanExternalFuncLinkage::vkGetAccelerationStructureBuildSizesKHR(device->GetLogicalDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, 
			&buildInfo, primitiveCounts.data(), &sizeInfo);

		m_StorageBufferHandle = device->CreateDeviceAddressBuffer({
			.DebugName = GetDebugName() + " Storage",
			.Size = sizeInfo.accelerationStructureSize,
			.Usage = BufferUsage::AccelerationStructureStorage,
			.MemoryUsage = MemoryUsage::GPUOnly
		});

		const auto& storageBuffer = device->AccessResource<VulkanDeviceAddressBuffer>(m_StorageBufferHandle);

		VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
		accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreateInfo.buffer = storageBuffer->GetVulkanBufferHandle();
		accelerationStructureCreateInfo.offset = 0;
		accelerationStructureCreateInfo.size = sizeInfo.accelerationStructureSize;
		accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

		LUCY_VK_ASSERT(VulkanExternalFuncLinkage::vkCreateAccelerationStructureKHR(device->GetLogicalDevice(), &accelerationStructureCreateInfo, nullptr, &m_AccelerationStructure));

		RenderDeviceResourceHandle scratchBufferHandle = device->CreateDeviceAddressBuffer({
			.DebugName = GetDebugName() + " Scratch",
			.Size = sizeInfo.buildScratchSize,
			.Usage = BufferUsage::Storage,
			.MemoryUsage = MemoryUsage::GPUOnly
		});

		const auto& scratchBuffer = device->AccessResource<VulkanDeviceAddressBuffer>(scratchBufferHandle);
		buildInfo.dstAccelerationStructure = m_AccelerationStructure;
		buildInfo.scratchData.deviceAddress = scratchBuffer->GetDeviceAddress();

		std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> rangePointers;
		rangePointers.reserve(ranges.size());
		for (const auto& range : ranges)
			rangePointers.emplace_back(&range);

		device->SubmitImmediateCommand([=](VkCommandBuffer commandBuffer) {
			VulkanExternalFuncLinkage::vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildInfo, rangePointers.data());
		});

		VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
		addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		addressInfo.accelerationStructure = m_AccelerationStructure;

		m_DeviceAddress = VulkanExternalFuncLinkage::vkGetAccelerationStructureDeviceAddressKHR(device->GetLogicalDevice(), &addressInfo);
		LUCY_ASSERT(m_DeviceAddress != 0, "Failed to query BLAS device address");

		device->RTDestroyResource(scratchBufferHandle);
		device->RTDestroyResource(transformBufferHandle);
	}

	void VulkanAccelerationStructure::RTCreateTopLevel(const Ref<VulkanRenderDevice>& device) {
		LUCY_ASSERT(Renderer::IsOnRenderThread());
		const auto& createInfo = GetTLCreateInfo();

		std::vector<VkAccelerationStructureInstanceKHR> vulkanInstances = CreateVulkanInstances(createInfo);

		m_InstanceBufferHandle = device->CreateDeviceAddressBuffer({
			.DebugName = GetDebugName() + " InstanceBuffer",
			.Size = createInfo.Instances.size() * sizeof(VkAccelerationStructureInstanceKHR),
			.Usage = BufferUsage::AccelerationStructureBuildInput,
			.MemoryUsage = MemoryUsage::GPUOnly
		});

		const auto& instanceBuffer = device->AccessResource<RenderDeviceBuffer>(m_InstanceBufferHandle);
		instanceBuffer->RTLoadToDevice(device.get(), vulkanInstances.data(), sizeof(VkAccelerationStructureInstanceKHR) * vulkanInstances.size());

		device->GetUploadManager()->SyncFrame(Renderer::GetCurrentFrameIndex());

		VkAccelerationStructureGeometryInstancesDataKHR instanceData{};
		instanceData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		instanceData.arrayOfPointers = VK_FALSE;
		instanceData.data.deviceAddress = instanceBuffer->GetDeviceAddress();

		VkAccelerationStructureGeometryKHR geometry{};
		geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		geometry.geometry.instances = instanceData;

		VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
		buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		buildInfo.geometryCount = 1;
		buildInfo.pGeometries = &geometry;
		buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

		VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
		sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		uint32_t primitiveCount = static_cast<uint32_t>(createInfo.Instances.size());
		VulkanExternalFuncLinkage::vkGetAccelerationStructureBuildSizesKHR(device->GetLogicalDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &primitiveCount, &sizeInfo);

		m_StorageBufferHandle = device->CreateDeviceAddressBuffer({
			.DebugName = GetDebugName() + " Storage",
			.Size = sizeInfo.accelerationStructureSize,
			.Usage = BufferUsage::AccelerationStructureStorage,
			.MemoryUsage = MemoryUsage::GPUOnly
		});

		const auto& storageBuffer = device->AccessResource<VulkanDeviceAddressBuffer>(m_StorageBufferHandle);

		VkAccelerationStructureCreateInfoKHR accCreateInfo{};
		accCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accCreateInfo.buffer = storageBuffer->GetVulkanBufferHandle();
		accCreateInfo.offset = 0;
		accCreateInfo.size = sizeInfo.accelerationStructureSize;
		accCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

		LUCY_VK_ASSERT(VulkanExternalFuncLinkage::vkCreateAccelerationStructureKHR(device->GetLogicalDevice(), &accCreateInfo, nullptr, &m_AccelerationStructure));

		buildInfo.dstAccelerationStructure = m_AccelerationStructure;

		VkDeviceSize requiredScratchSize = std::max(sizeInfo.updateScratchSize, sizeInfo.buildScratchSize);

		m_ScratchBufferHandle = device->CreateDeviceAddressBuffer({
			.DebugName = GetDebugName() + " Scratch",
			.Size = requiredScratchSize,
			.Usage = BufferUsage::Storage,
			.MemoryUsage = MemoryUsage::GPUOnly
		});

		const auto& scratchBuffer = device->AccessResource<RenderDeviceBuffer>(m_ScratchBufferHandle);
		buildInfo.scratchData.deviceAddress = scratchBuffer->GetDeviceAddress();

		VkAccelerationStructureBuildRangeInfoKHR range{};
		range.primitiveCount = primitiveCount;
		range.primitiveOffset = 0;
		range.firstVertex = 0;
		range.transformOffset = 0;

		const VkAccelerationStructureBuildRangeInfoKHR* rangePointer = &range;
		device->SubmitImmediateCommand([=](VkCommandBuffer commandBuffer) {
			VulkanExternalFuncLinkage::vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildInfo, &rangePointer);
		});

		VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
		addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		addressInfo.accelerationStructure = m_AccelerationStructure;

		m_DeviceAddress = VulkanExternalFuncLinkage::vkGetAccelerationStructureDeviceAddressKHR(device->GetLogicalDevice(), &addressInfo);
		LUCY_ASSERT(m_DeviceAddress != 0, "Failed to query BLAS device address");
	}

	void VulkanAccelerationStructure::RTDestroyResource(RenderDevice* device) {
		auto* vulkanDevice = reinterpret_cast<VulkanRenderDevice*>(device);

		VulkanExternalFuncLinkage::vkDestroyAccelerationStructureKHR(vulkanDevice->GetLogicalDevice(), m_AccelerationStructure, nullptr);
		m_AccelerationStructure = VK_NULL_HANDLE;
		
		device->RTDestroyResource(m_StorageBufferHandle);
		device->RTDestroyResource(m_InstanceBufferHandle);
		device->RTDestroyResource(m_ScratchBufferHandle);

		m_StorageBufferHandle = {};
		m_InstanceBufferHandle = {};
		m_ScratchBufferHandle = {};
		m_DeviceAddress = 0;
	}

	std::vector<VkAccelerationStructureInstanceKHR> VulkanAccelerationStructure::CreateVulkanInstances(const TLAccelerationStructureCreateInfo& createInfo) {
		std::vector<VkAccelerationStructureInstanceKHR> vulkanInstances;
		vulkanInstances.reserve(createInfo.Instances.size());

		for (const auto& instance : createInfo.Instances) {
			VkAccelerationStructureInstanceKHR& result = vulkanInstances.emplace_back();
			result.transform = TransformToVulkanMatrix(instance.Transform);
			result.instanceCustomIndex = instance.CustomIndex;
			result.mask = instance.Mask;
			result.instanceShaderBindingTableRecordOffset = instance.ShaderBindingTableRecordOffset;
			result.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR | VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
			result.accelerationStructureReference = instance.BottomLevelAccelerationStructureAddress;
		}

		return vulkanInstances;
	}

	VulkanRayTracingPipeline::VulkanRayTracingPipeline(const RayTracingPipelineCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device) 
		: RayTracingPipeline(createInfo) {
		Renderer::EnqueueToRenderCommandQueue([this](const auto& device) {
			Create(device->As<VulkanRenderDevice>());
		});
	}

	void VulkanRayTracingPipeline::Create(const Ref<VulkanRenderDevice>& vulkanDevice) {
		LUCY_ASSERT(Renderer::IsOnRenderThread());

		VkDevice logicalDevice = vulkanDevice->GetLogicalDevice();

		const auto& rayGenShader = GetRayGenShader()->As<VulkanRayTracingShader>();
		const auto& missShader = GetMissShader()->As<VulkanRayTracingShader>();
		const auto& closestHitShader = GetClosestHitShader()->As<VulkanRayTracingShader>();

		std::vector<VkPipelineShaderStageCreateInfo> stages;
		stages.reserve(GetAnyHitShader() ? 4 : 3);

		stages.emplace_back(rayGenShader->GetShaderInfo());
		stages.emplace_back(missShader->GetShaderInfo());
		stages.emplace_back(closestHitShader->GetShaderInfo());

		uint32_t anyHitStageIndex = VK_SHADER_UNUSED_KHR;
		if (GetAnyHitShader()) {
			anyHitStageIndex = static_cast<uint32_t>(stages.size());

			const auto& anyHitShader = GetAnyHitShader()->As<VulkanRayTracingShader>();
			stages.emplace_back(anyHitShader->GetShaderInfo());
		}

		std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;
		shaderGroups.reserve(3);

		VkRayTracingShaderGroupCreateInfoKHR rayGenGroup{};
		rayGenGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		rayGenGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		rayGenGroup.generalShader = 0;
		rayGenGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		rayGenGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		rayGenGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

		shaderGroups.emplace_back(rayGenGroup);

		VkRayTracingShaderGroupCreateInfoKHR missGroup{};
		missGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		missGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		missGroup.generalShader = 1;
		missGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		missGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		missGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

		shaderGroups.emplace_back(missGroup);

		VkRayTracingShaderGroupCreateInfoKHR hitGroup{};
		hitGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		hitGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		hitGroup.generalShader = VK_SHADER_UNUSED_KHR;
		hitGroup.closestHitShader = 2;
		hitGroup.anyHitShader = anyHitStageIndex;
		hitGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

		shaderGroups.emplace_back(hitGroup);

		m_ShaderGroupCount = static_cast<uint32_t>(shaderGroups.size());

		const auto AddShaderDescriptorSets = [&](const Ref<Shader>& shader) {
			const auto descriptorSetHandles = vulkanDevice->GetResourceBindingHandles(shader);
			for (const auto& handle : descriptorSetHandles) {
				if (std::ranges::find(m_DescriptorSetHandles, handle) == m_DescriptorSetHandles.end())
					m_DescriptorSetHandles.emplace_back(handle);
			}
		};

		AddShaderDescriptorSets(GetRayGenShader());
		AddShaderDescriptorSets(GetMissShader());
		AddShaderDescriptorSets(GetClosestHitShader());
		AddShaderDescriptorSets(GetAnyHitShader());

		const auto& rayGenPushConstants = GetRayGenShader()->GetShaderPushConstants();
		for (auto& pushConstant : rayGenPushConstants)
			AddPushConstant(pushConstant);
		
		const auto& pushConstants = GetPipelineConstants();
		std::vector<VkPushConstantRange> pushConstantRanges;
		pushConstantRanges.reserve(pushConstants.size());
		for (const PipelineConstant& pushConstant : pushConstants)
			pushConstantRanges.emplace_back(pushConstant.GetHandle());

		uint32_t highestSetIndex = 0;

		for (const auto& handle : m_DescriptorSetHandles) {
			const auto& descriptorSet = vulkanDevice->AccessResource<VulkanDescriptorSet>(handle);
			highestSetIndex = std::max(highestSetIndex, descriptorSet->GetSetIndex());
		}

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts(highestSetIndex + 1, VK_NULL_HANDLE);
		for (const auto& handle : m_DescriptorSetHandles) {
			const auto& descriptorSet = vulkanDevice->AccessResource<VulkanDescriptorSet>(handle);
			descriptorSetLayouts[descriptorSet->GetSetIndex()] = descriptorSet->GetDescriptorSetLayout();
		}

		//bcs vulkan requires all descriptor set layouts to be present in the pipeline layout, we need to create empty descriptor set layouts for any missing ones
		for (uint32_t setIndex = 0; setIndex < descriptorSetLayouts.size(); setIndex++) {
			if (descriptorSetLayouts[setIndex] != VK_NULL_HANDLE)
				continue;

			VkDescriptorSetLayoutCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

			VkDescriptorSetLayout emptyLayout = VK_NULL_HANDLE;

			LUCY_VK_ASSERT(vkCreateDescriptorSetLayout(logicalDevice, &createInfo, nullptr, &emptyLayout));

			descriptorSetLayouts[setIndex] = emptyLayout;
			m_EmptyDescriptorSetLayouts.emplace_back(emptyLayout);
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = VulkanAPI::PipelineLayoutCreateInfo(static_cast<uint32_t>(descriptorSetLayouts.size()), descriptorSetLayouts.data(), static_cast<uint32_t>(pushConstantRanges.size()), pushConstantRanges.data());

		LUCY_VK_ASSERT(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &m_PipelineLayoutHandle));
		VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		pipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
		pipelineInfo.pStages = stages.data();
		pipelineInfo.groupCount = static_cast<uint32_t>(shaderGroups.size());
		pipelineInfo.pGroups = shaderGroups.data();
		pipelineInfo.maxPipelineRayRecursionDepth = 1;
		pipelineInfo.layout = m_PipelineLayoutHandle;

		LUCY_VK_ASSERT(VulkanExternalFuncLinkage::vkCreateRayTracingPipelinesKHR(logicalDevice, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_PipelineHandle));
		CreateShaderBindingTable(vulkanDevice);

		LUCY_INFO("Vulkan ray tracing pipeline '{0}' created successfully!", GetDebugName());
#ifdef LUCY_DEBUG
		std::string objectName = std::format("{0} Ray Tracing Pipeline", GetDebugName());

		VkDebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_PIPELINE;
		nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_PipelineHandle);
		nameInfo.pObjectName = objectName.c_str();

		VulkanExternalFuncLinkage::vkSetDebugUtilsObjectNameEXT(logicalDevice, &nameInfo);
#endif
	}

	void VulkanRayTracingPipeline::RTRecreate(Ref<Shader> shader) {
		
	}

	void VulkanRayTracingPipeline::RTTrace(void* commandBufferHandle, uint32_t width, uint32_t height, uint32_t depth) {
		VulkanExternalFuncLinkage::vkCmdTraceRaysKHR(static_cast<VkCommandBuffer>(commandBufferHandle), &m_RayGenRegion, &m_MissRegion, &m_HitRegion, &m_CallableRegion, width, height, depth);
	}

	void VulkanRayTracingPipeline::RTBind(void* commandBufferHandle) {
		vkCmdBindPipeline(static_cast<VkCommandBuffer>(commandBufferHandle), VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_PipelineHandle);
	}

	void VulkanRayTracingPipeline::RTUpdateAccelerationStructure(RenderDevice* device, const std::string& name, const Ref<AccelerationStructure>& accelerationStructure) {
		for (const auto& handle : m_DescriptorSetHandles) {
			const auto& descriptorSet = device->AccessResource<VulkanDescriptorSet>(handle);
			if (!descriptorSet->HasAccelerationStructureBinding(name))
				continue;
			descriptorSet->RTUpdateAccelerationStructure(device, name, accelerationStructure);
			return;
		}
		LUCY_ASSERT(false, "Acceleration structure binding '{0}' does not exist", name);
	}

	void VulkanRayTracingPipeline::RTDestroyResource(RenderDevice* device) {
		Pipeline::RTDestroyResource(device);

		const auto& vulkanDevice = reinterpret_cast<VulkanRenderDevice*>(device);

		if (m_PipelineLayoutHandle != VK_NULL_HANDLE)
			vkDestroyPipelineLayout(vulkanDevice->GetLogicalDevice(), m_PipelineLayoutHandle, nullptr);

		for (VkDescriptorSetLayout layout : m_EmptyDescriptorSetLayouts)
			vkDestroyDescriptorSetLayout(vulkanDevice->GetLogicalDevice(), layout, nullptr);
		m_EmptyDescriptorSetLayouts.clear();

		if (m_PipelineHandle != VK_NULL_HANDLE)
			vkDestroyPipeline(vulkanDevice->GetLogicalDevice(), m_PipelineHandle, nullptr);

		if (m_ShaderBindingTableHandle)
			device->RTDestroyResource(m_ShaderBindingTableHandle);

		m_PipelineLayoutHandle = VK_NULL_HANDLE;
		m_PipelineHandle = VK_NULL_HANDLE;

		m_ShaderBindingTableHandle = {};

		m_RayGenRegion = {};
		m_MissRegion = {};
		m_HitRegion = {};
		m_CallableRegion = {};
	}

	void VulkanRayTracingPipeline::CreateShaderBindingTable(const Ref<VulkanRenderDevice>& vulkanDevice) {
		LUCY_ASSERT(Renderer::IsOnRenderThread());
		LUCY_ASSERT(m_PipelineHandle != VK_NULL_HANDLE);
		LUCY_ASSERT(m_ShaderGroupCount == 3);

		const auto AlignUp = [](VkDeviceSize value, VkDeviceSize alignment) {
			return (value + alignment - 1) & ~(alignment - 1);
		};

		const auto& deviceInfo = vulkanDevice->GetDeviceInformation();

		uint32_t handleSize = deviceInfo.ShaderGroupHandleSize;
		uint32_t handleAlignment = deviceInfo.ShaderGroupHandleAlignment;
		uint32_t baseAlignment = deviceInfo.ShaderGroupBaseAlignment;

		VkDeviceSize handleSizeAligned = AlignUp(handleSize, handleAlignment);

		VkDeviceSize rayGenOffset = 0;
		VkDeviceSize missOffset = AlignUp(rayGenOffset + handleSizeAligned, baseAlignment);
		VkDeviceSize hitOffset = AlignUp(missOffset + handleSizeAligned, baseAlignment);

		VkDeviceSize bufferSize = hitOffset + handleSizeAligned;
		std::vector<uint8_t> shaderGroupHandles(static_cast<size_t>(handleSize) * m_ShaderGroupCount);

		LUCY_VK_ASSERT(VulkanExternalFuncLinkage::vkGetRayTracingShaderGroupHandlesKHR(vulkanDevice->GetLogicalDevice(), m_PipelineHandle, 0, m_ShaderGroupCount, shaderGroupHandles.size(), shaderGroupHandles.data()));

		m_ShaderBindingTableHandle = vulkanDevice->CreateDeviceAddressBuffer({
			.DebugName = GetDebugName() + " ShaderBindingTable",
			.Size = bufferSize,
			.Usage = BufferUsage::ShaderBindingTable,
			.MemoryUsage = MemoryUsage::CPUToGPU
		});

		const auto& shaderBindingTable = vulkanDevice->AccessResource<VulkanDeviceAddressBuffer>(m_ShaderBindingTableHandle);
		LUCY_ASSERT(shaderBindingTable->GetMappedData());

		uint8_t* mappedData = static_cast<uint8_t*>(shaderBindingTable->GetMappedData());
		memset(mappedData, 0, bufferSize);
		memcpy(mappedData + rayGenOffset, shaderGroupHandles.data() + handleSize * 0, handleSize);
		memcpy(mappedData + missOffset, shaderGroupHandles.data() + handleSize * 1, handleSize);
		memcpy(mappedData + hitOffset, shaderGroupHandles.data() + handleSize * 2, handleSize);

		const VkDeviceAddress shaderBindingTableAddress = shaderBindingTable->GetDeviceAddress();
		m_RayGenRegion.deviceAddress = shaderBindingTableAddress + rayGenOffset;
		m_RayGenRegion.stride = handleSizeAligned;
		m_RayGenRegion.size = handleSizeAligned;

		m_MissRegion.deviceAddress = shaderBindingTableAddress + missOffset;
		m_MissRegion.stride = handleSizeAligned;
		m_MissRegion.size = handleSizeAligned;

		m_HitRegion.deviceAddress = shaderBindingTableAddress + hitOffset;
		m_HitRegion.stride = handleSizeAligned;
		m_HitRegion.size = handleSizeAligned;

		m_CallableRegion = {};
	}
}