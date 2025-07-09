#include "lypch.h"
#include "VulkanRenderDevice.h"
#include "Renderer/Context/VulkanContext.h"

#include "Renderer/Pipeline/VulkanGraphicsPipeline.h"
#include "Renderer/Pipeline/VulkanComputePipeline.h"

#include "Renderer/Descriptors/VulkanDescriptorSet.h"

#include "Renderer/Commands/VulkanCommandPool.h"

#include "Renderer/Memory/Buffer/Vulkan/VulkanVertexBuffer.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanIndexBuffer.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanFrameBuffer.h"
#include "Renderer/Memory/VulkanAllocator.h"

#include "Renderer/Renderer.h"

#include "../Mesh.h"

namespace Lucy {

	void VulkanRenderDevice::Init(VkInstance instance, const std::vector<const char*>& enabledValidationLayers, VkSurfaceKHR surface, uint32_t apiVersion) {
		m_Surface = surface;

		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		LUCY_ASSERT(deviceCount != 0, "No physical device found that supports Vulkan!");

		std::vector<VkPhysicalDevice> availablePhysicalDevices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, availablePhysicalDevices.data());

		PickDeviceByRanking(availablePhysicalDevices);
		PrintDeviceInfo();
		CreateLogicalDevice(enabledValidationLayers);

		vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.GraphicsFamily, 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.PresentFamily, 0, &m_PresentQueue);
		vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.ComputeFamily, 0, &m_ComputeQueue);
		vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.TransferFamily, 0, &m_TransferQueue);

		m_Allocator.Init(instance, m_LogicalDevice, m_PhysicalDevice, apiVersion);

		m_ImmediateCommandFence = Memory::CreateUnique<Fence>(this);
	}

	void VulkanRenderDevice::PickDeviceByRanking(const std::vector<VkPhysicalDevice>& devices) {

		LUCY_INFO("----------Available Devices----------");
#if USE_INTEGRATED_GRAPHICS
		for (const auto& device : devices) {
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(device, &properties);

			VkPhysicalDeviceFeatures features;
			vkGetPhysicalDeviceFeatures(device, &features);

			if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
				continue;

			VulkanDeviceInfo deviceInfo = { properties.deviceName, properties.driverVersion, properties.apiVersion };
			LUCY_INFO(std::format("Device Name: {0}", deviceInfo.Name));
			LUCY_INFO(std::format("Device Driver Version: {0}", deviceInfo.DriverVersion));
			LUCY_INFO(std::format("Device API Version: {0}", deviceInfo.ApiVersion));
			LUCY_INFO("-------------------------------------");
			deviceInfo.MinUniformBufferAlignment = (uint32_t)properties.limits.minUniformBufferOffsetAlignment;
			deviceInfo.TimestampPeriod = properties.limits.timestampPeriod;

			FindQueueFamilies(device);

			bool isDeviceRequirementsCovered = CheckDeviceExtensionSupport(device) && CheckDeviceFormatSupport(device);

			if (features.multiViewport
				&& features.geometryShader
				&& features.samplerAnisotropy
				&& features.tessellationShader
				&& m_QueueFamilyIndices.IsComplete()
				&& isDeviceRequirementsCovered) {
				m_DeviceInfo = deviceInfo;
				m_PhysicalDevice = device;
				return;
			}
		}
#else
		for (const auto& device : devices) {
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(device, &properties);

			VkPhysicalDeviceFeatures features;
			vkGetPhysicalDeviceFeatures(device, &features);

			VulkanDeviceInfo deviceInfo = { properties.deviceName, properties.driverVersion, properties.apiVersion };
			LUCY_INFO(std::format("Device Name: {0}", deviceInfo.Name));
			LUCY_INFO(std::format("Device Driver Version: {0}", deviceInfo.DriverVersion));
			LUCY_INFO(std::format("Device API Version: {0}", deviceInfo.ApiVersion));
			LUCY_INFO("-------------------------------------");
			deviceInfo.MinUniformBufferAlignment = (uint32_t)properties.limits.minUniformBufferOffsetAlignment;
			deviceInfo.TimestampPeriod = properties.limits.timestampPeriod;

			FindQueueFamilies(device);

			bool isDeviceRequirementsCovered = CheckDeviceExtensionSupport(device) && CheckDeviceFormatSupport(device);

			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
				&& features.multiViewport
				&& features.geometryShader
				&& features.samplerAnisotropy
				&& features.tessellationShader
				&& m_QueueFamilyIndices.IsComplete()
				&& isDeviceRequirementsCovered) {
				m_DeviceInfo = deviceInfo;
				m_PhysicalDevice = device;
				return;
			}
		}
#endif

		LUCY_ASSERT(false, "No suitable device found!");
	}

	void VulkanRenderDevice::CreateLogicalDevice(const std::vector<const char*>& enabledValidationLayers) {
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { 
			m_QueueFamilyIndices.GraphicsFamily, 
			m_QueueFamilyIndices.ComputeFamily, 
			m_QueueFamilyIndices.TransferFamily, 
			m_QueueFamilyIndices.PresentFamily 
		};

		float priority = 1.0f;
		for (uint32_t family : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = family;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &priority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		//For layered rendering (cubemaps for example)
		VkPhysicalDeviceMultiviewFeatures multiViewFeatures{};
		multiViewFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
		multiViewFeatures.multiview = VK_TRUE;

		VkPhysicalDeviceVulkan12Features vulkan12Features{};
		vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		//For layered rendering (cubemaps for example)
		vulkan12Features.shaderOutputLayer = VK_TRUE;
		//For bindless descriptor sets
		vulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
		vulkan12Features.descriptorBindingVariableDescriptorCount = VK_TRUE;
		vulkan12Features.runtimeDescriptorArray = VK_TRUE;
		//for query pool reset
		vulkan12Features.hostQueryReset = VK_TRUE;
		vulkan12Features.pNext = &multiViewFeatures;

		//For compute shaders/pipeline
		VkPhysicalDeviceVulkan13Features vulkan13Features{};
		vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		vulkan13Features.maintenance4 = VK_TRUE;
		vulkan13Features.synchronization2 = VK_TRUE;
		vulkan13Features.pNext = &vulkan12Features;

		VkPhysicalDeviceFeatures2 features{};
		features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &features);
		features.features.samplerAnisotropy = VK_TRUE;
		features.features.geometryShader = VK_TRUE;
		features.features.multiViewport = VK_TRUE;
		features.pNext = &vulkan13Features; //extending this structure

		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
		//deviceCreateInfo.pEnabledFeatures = &features;
		deviceCreateInfo.pNext = &features;

		deviceCreateInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();
		deviceCreateInfo.enabledExtensionCount = (uint32_t)m_DeviceExtensions.size();

#ifdef LUCY_DEBUG
		deviceCreateInfo.enabledLayerCount = (uint32_t)enabledValidationLayers.size();
		deviceCreateInfo.ppEnabledLayerNames = enabledValidationLayers.data();
#else
		deviceCreateInfo.enabledLayerCount = 0;
		deviceCreateInfo.ppEnabledLayerNames = 0;
#endif

		LUCY_VK_ASSERT(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_LogicalDevice));
	}

	void VulkanRenderDevice::FindQueueFamilies(VkPhysicalDevice device) {
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; const auto& queueFamily : queueFamilies) {
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);

			VkBool32 graphicsBitSet = queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT;
			VkBool32 computeBitSet = queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT;
			VkBool32 transferBitSet = queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT;

			if (presentSupport) {
				m_QueueFamilyIndices.PresentFamilyHasValue = true;
				m_QueueFamilyIndices.PresentFamily = i;
			}

			if (graphicsBitSet) {
				m_QueueFamilyIndices.GraphicsFamilyHasValue = true;
				m_QueueFamilyIndices.GraphicsFamily = i;
			}

			if (computeBitSet && !graphicsBitSet) {
				m_QueueFamilyIndices.ComputeFamilyHasValue = true;
				m_QueueFamilyIndices.ComputeFamily = i;
			}

			if (transferBitSet && !graphicsBitSet && !computeBitSet) {
				m_QueueFamilyIndices.TransferFamilyHasValue = true;
				m_QueueFamilyIndices.TransferFamily = i;
			}
			i++;

			if (m_QueueFamilyIndices.GraphicsFamilyHasValue && m_QueueFamilyIndices.ComputeFamilyHasValue && m_QueueFamilyIndices.PresentFamilyHasValue && m_QueueFamilyIndices.TransferFamilyHasValue)
				break;
		}

		//If neither of the constraints are fullfilled... the gpu has probably 1 Queue family, which supports everything that it supports. (probably intel integrated gpu)
		if (!m_QueueFamilyIndices.ComputeFamilyHasValue) {
			m_QueueFamilyIndices.ComputeFamilyHasValue = true;
			m_QueueFamilyIndices.ComputeFamily = m_QueueFamilyIndices.GraphicsFamily;
		}

		if (!m_QueueFamilyIndices.TransferFamilyHasValue) {
			m_QueueFamilyIndices.TransferFamilyHasValue = true;
			m_QueueFamilyIndices.TransferFamily = m_QueueFamilyIndices.GraphicsFamily;
		}
	}

	bool VulkanRenderDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		if (!requiredExtensions.empty()) {
			LUCY_CRITICAL("Not available extensions");
			uint32_t i = 0;
			for (const auto& notAvailableExtensions : requiredExtensions) {
				LUCY_CRITICAL(std::format("Index: {0}, Name: {1}", (i++), notAvailableExtensions));
			}
			LUCY_ASSERT(false);
		}

		return requiredExtensions.empty();
	}

	bool VulkanRenderDevice::CheckDeviceFormatSupport(VkPhysicalDevice device) const {
		bool allFormatIsSupported = true;
		for (VkFormat formatToCheck : m_DeviceFormatSupportToCheck) {
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(device, formatToCheck, &properties);

			if (!(properties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
				allFormatIsSupported = false;
				break;
			}
		}

		return allFormatIsSupported;
	}

	void VulkanRenderDevice::SubmitWorkToGPU(VkQueue queueHandle, size_t commandBufferCount, VkCommandBuffer* commandBuffers, Fence* currentFrameFence, Semaphore* currentFrameWaitSemaphore, Semaphore* currentFrameSignalSemaphore) const {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::SubmitWorkToGPU");

		VkPipelineStageFlags imageWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		if (queueHandle == m_ComputeQueue) {
			imageWaitStages[0] = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		}

		bool noWaiting = currentFrameWaitSemaphore == nullptr;
		bool noSignaling = currentFrameSignalSemaphore == nullptr;

		bool noSyncNeeded = noWaiting && noSignaling;

		VkSubmitInfo submitInfo = VulkanAPI::QueueSubmitInfo((uint32_t)commandBufferCount, commandBuffers, noSyncNeeded ? 0 : 1, 
			noSyncNeeded ? nullptr : &currentFrameWaitSemaphore->GetSemaphore(), imageWaitStages, noSyncNeeded ? 0 : 1,
			noSyncNeeded ? nullptr : &currentFrameSignalSemaphore->GetSemaphore());
		LUCY_VK_ASSERT(vkQueueSubmit(queueHandle, 1, &submitInfo, currentFrameFence->GetFence()));
	}

	void VulkanRenderDevice::SubmitWorkToGPU(VkQueue queueHandle, VkCommandBuffer currentCommandBuffer, Fence* currentFrameFence, Semaphore* currentFrameWaitSemaphore, Semaphore* currentFrameSignalSemaphore) const {
		SubmitWorkToGPU(queueHandle, 1, &currentCommandBuffer, currentFrameFence, currentFrameWaitSemaphore, currentFrameSignalSemaphore);
	}

	void VulkanRenderDevice::SubmitWorkToGPU(VkQueue queueHandle, size_t commandBufferCount, void* commandBufferHandles) const {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::SubmitWorkToGPU");
		LUCY_ASSERT(commandBufferCount != 0);

		VkFence fenceHandle = m_ImmediateCommandFence->GetFence();
		vkResetFences(m_LogicalDevice, 1, &fenceHandle);

		VkSubmitInfo submitInfo = VulkanAPI::QueueSubmitInfo(commandBufferCount, (VkCommandBuffer*)&commandBufferHandles, 0, nullptr, nullptr, 0, nullptr);
		LUCY_VK_ASSERT(vkQueueSubmit(queueHandle, 1, &submitInfo, fenceHandle));
		LUCY_VK_ASSERT(vkWaitForFences(m_LogicalDevice, 1, &fenceHandle, VK_TRUE, UINT64_MAX));
	}

	void VulkanRenderDevice::SubmitWorkToGPU(TargetQueueFamily queueFamily, Ref<CommandPool> cmdPool,
											 Fence* currentFrameFence, Semaphore* currentFrameWaitSemaphore, Semaphore* currentFrameSignalSemaphore) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::SubmitWorkToGPU");
		LUCY_ASSERT(cmdPool);

		auto commandBuffer = (VkCommandBuffer)cmdPool->GetCurrentFrameCommandBuffer();
		switch (queueFamily) {
			using enum Lucy::TargetQueueFamily;
			case Graphics: {
				SubmitWorkToGPU(m_GraphicsQueue, commandBuffer, currentFrameFence, currentFrameWaitSemaphore, currentFrameSignalSemaphore);
				break;
			}
			case Compute: {
				SubmitWorkToGPU(m_ComputeQueue, commandBuffer, currentFrameFence, currentFrameWaitSemaphore, currentFrameSignalSemaphore);
				break;
			}
			case Transfer: {
				SubmitWorkToGPU(m_TransferQueue, commandBuffer, currentFrameFence, currentFrameWaitSemaphore, currentFrameSignalSemaphore);
				break;
			}
			default:
				LUCY_ASSERT(false);
		}
	}

	bool VulkanRenderDevice::SubmitWorkToGPU(TargetQueueFamily queueFamily, std::vector<Ref<CommandPool>>& cmdPools, 
											Fence* currentFrameFence, Semaphore* currentFrameWaitSemaphore, Semaphore* currentFrameSignalSemaphore) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::SubmitWorkToGPU");
		if (cmdPools.empty())
			return false;

		std::vector<VkCommandBuffer> cmdBufferHandles;
		cmdBufferHandles.resize(cmdPools.size(), VK_NULL_HANDLE);
		for (size_t i = 0; const Ref<CommandPool>& cmdPool : cmdPools)
			cmdBufferHandles[i++] = (VkCommandBuffer)cmdPool->GetCurrentFrameCommandBuffer();

		if (cmdBufferHandles.empty())
			return false;

		switch (queueFamily) {
			using enum Lucy::TargetQueueFamily;
			case Graphics: {
				SubmitWorkToGPU(m_GraphicsQueue, cmdBufferHandles.size(), cmdBufferHandles.data(), currentFrameFence, currentFrameWaitSemaphore, currentFrameSignalSemaphore);
				break;
			}
			case Compute: {
				SubmitWorkToGPU(m_ComputeQueue, cmdBufferHandles.size(), cmdBufferHandles.data(), currentFrameFence, currentFrameWaitSemaphore, currentFrameSignalSemaphore);
				break;
			}
			case Transfer: {
				SubmitWorkToGPU(m_TransferQueue, cmdBufferHandles.size(), cmdBufferHandles.data(), currentFrameFence, currentFrameWaitSemaphore, currentFrameSignalSemaphore);
				break;
			}
			default:
				LUCY_ASSERT(false);
		}

		return true;
	}

	void VulkanRenderDevice::SubmitWorkToGPU(TargetQueueFamily queueFamily, std::vector<Ref<CommandPool>>& cmdPools, 
											Fence* currentFrameFence, Semaphore* currentFrameWaitSemaphore) {
		SubmitWorkToGPU(queueFamily, cmdPools, currentFrameFence, currentFrameWaitSemaphore, nullptr);
	}

	void VulkanRenderDevice::PrintDeviceInfo() {
		LUCY_INFO(std::format("Selected Device: {0}", m_DeviceInfo.Name));
	}

	void VulkanRenderDevice::Destroy() {
		LUCY_PROFILE_DESTROY();
		m_ImmediateCommandFence->Destroy(shared_from_this()->As<RenderDevice>());

		m_Allocator.Destroy();
		vkDestroyDevice(m_LogicalDevice, nullptr);
	}

	void VulkanRenderDevice::BeginCommandBuffer(Ref<CommandPool> cmdPool) {
		auto cmdBufferBeginInfo = VulkanAPI::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		vkBeginCommandBuffer((VkCommandBuffer)cmdPool->GetCurrentFrameCommandBuffer(), &cmdBufferBeginInfo);
	}

	void VulkanRenderDevice::EndCommandBuffer(Ref<CommandPool> cmdPool) {
		vkEndCommandBuffer((VkCommandBuffer)cmdPool->GetCurrentFrameCommandBuffer());
	}

	void VulkanRenderDevice::BindBuffers(Ref<CommandPool> cmdPool, Ref<Mesh> mesh) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindBuffers");

		VulkanVertexBindInfo vertexInfo;
		vertexInfo.CommandBuffer = (VkCommandBuffer)cmdPool->GetCurrentFrameCommandBuffer();
		AccessResource<VulkanVertexBuffer>(mesh->GetVertexBufferHandle())->RTBind(vertexInfo);

		VulkanIndexBindInfo indexInfo;
		indexInfo.CommandBuffer = vertexInfo.CommandBuffer;
		AccessResource<VulkanIndexBuffer>(mesh->GetIndexBufferHandle())->RTBind(indexInfo);
	}

	void VulkanRenderDevice::BindBuffers(Ref<CommandPool> cmdPool, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindBuffers");

		VulkanVertexBindInfo vertexInfo;
		vertexInfo.CommandBuffer = (VkCommandBuffer)cmdPool->GetCurrentFrameCommandBuffer();
		vertexBuffer->As<VulkanVertexBuffer>()->RTBind(vertexInfo);

		VulkanIndexBindInfo indexInfo;
		indexInfo.CommandBuffer = vertexInfo.CommandBuffer;
		indexBuffer->As<VulkanIndexBuffer>()->RTBind(indexInfo);
	}

	void VulkanRenderDevice::BindPushConstant(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline, const VulkanPushConstant& pushConstant) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindPushConstant | Graphics");
		pushConstant.RTBind((VkCommandBuffer)cmdPool->GetCurrentFrameCommandBuffer(), pipeline->As<VulkanGraphicsPipeline>()->GetPipelineLayout());
	}

	void VulkanRenderDevice::BindPushConstant(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline, const VulkanPushConstant& pushConstant) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindPushConstant | Compute");
		pushConstant.RTBind((VkCommandBuffer)cmdPool->GetCurrentFrameCommandBuffer(), pipeline->As<VulkanComputePipeline>()->GetPipelineLayout());
	}

	void VulkanRenderDevice::BindPipeline(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindPipeline | Graphics");
		pipeline->RTBind(cmdPool->GetCurrentFrameCommandBuffer());
	}

	void VulkanRenderDevice::BindPipeline(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindPipeline | Compute");
		pipeline->RTBind(cmdPool->GetCurrentFrameCommandBuffer());
	}

	void VulkanRenderDevice::UpdateDescriptorSets(Ref<GraphicsPipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::UpdateDescriptorSets | Graphics");
		const auto& castedPipeline = pipeline->As<VulkanGraphicsPipeline>();
		
		for (auto handle : castedPipeline->GetShader()->GetDescriptorSetHandles()) {
			Ref<VulkanDescriptorSet> vulkanSet = AccessResource<VulkanDescriptorSet>(handle);
			vulkanSet->RTUpdate();
		}
	}
	
	void VulkanRenderDevice::UpdateDescriptorSets(Ref<ComputePipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::UpdateDescriptorSets | Compute");
		const auto& castedPipeline = pipeline->As<VulkanComputePipeline>();

		for (auto handle : castedPipeline->GetShader()->GetDescriptorSetHandles()) {
			Ref<VulkanDescriptorSet> vulkanSet = AccessResource<VulkanDescriptorSet>(handle);
			vulkanSet->RTUpdate();
		}
	}

	void VulkanRenderDevice::BindAllDescriptorSets(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindAllDescriptorSets | Graphics");
		const auto& castedPipeline = pipeline->As<VulkanGraphicsPipeline>();

		VulkanDescriptorSetBindInfo bindInfo;
		bindInfo.CommandBuffer = (VkCommandBuffer)cmdPool->GetCurrentFrameCommandBuffer();
		bindInfo.PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		bindInfo.PipelineLayout = castedPipeline->GetPipelineLayout();

		for (auto handle : castedPipeline->GetShader()->GetDescriptorSetHandles()) {
			Ref<VulkanDescriptorSet> vulkanSet = AccessResource<VulkanDescriptorSet>(handle);
			vulkanSet->RTBind(bindInfo);
		}
	}

	void VulkanRenderDevice::BindDescriptorSet(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline, uint32_t setIndex) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindDescriptorSet | Graphics");
		const auto& castedPipeline = pipeline->As<VulkanGraphicsPipeline>();

		VulkanDescriptorSetBindInfo bindInfo;
		bindInfo.CommandBuffer = (VkCommandBuffer)cmdPool->GetCurrentFrameCommandBuffer();
		bindInfo.PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		bindInfo.PipelineLayout = castedPipeline->GetPipelineLayout();

		for (auto handle : castedPipeline->GetShader()->GetDescriptorSetHandles()) {
			const auto& vulkanSet = AccessResource<VulkanDescriptorSet>(handle);
			if (vulkanSet->GetSetIndex() == setIndex) {
				vulkanSet->RTBind(bindInfo);
				break;
			}
		}
	}
	
	void VulkanRenderDevice::BindAllDescriptorSets(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindAllDescriptorSets | Compute");
		const auto& castedPipeline = pipeline->As<VulkanComputePipeline>();

		VulkanDescriptorSetBindInfo bindInfo;
		bindInfo.CommandBuffer = (VkCommandBuffer)cmdPool->GetCurrentFrameCommandBuffer();
		bindInfo.PipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
		bindInfo.PipelineLayout = castedPipeline->GetPipelineLayout();

		for (auto handle : castedPipeline->GetShader()->GetDescriptorSetHandles()) {
			const auto& vulkanSet = AccessResource<VulkanDescriptorSet>(handle);
			vulkanSet->RTBind(bindInfo);
		}
	}

	void VulkanRenderDevice::BindDescriptorSet(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline, uint32_t setIndex) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindDescriptorSet | Compute");
		const auto& castedPipeline = pipeline->As<VulkanComputePipeline>();

		VulkanDescriptorSetBindInfo bindInfo;
		bindInfo.CommandBuffer = (VkCommandBuffer)cmdPool->GetCurrentFrameCommandBuffer();
		bindInfo.PipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
		bindInfo.PipelineLayout = castedPipeline->GetPipelineLayout();

		for (auto handle : castedPipeline->GetShader()->GetDescriptorSetHandles()) {
			const auto& vulkanSet = AccessResource<VulkanDescriptorSet>(handle);
			if (vulkanSet->GetSetIndex() == setIndex) {
				vulkanSet->RTBind(bindInfo);
				break;
			}
		}
	}

	void VulkanRenderDevice::DrawIndexed(Ref<CommandPool> cmdPool, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
		vkCmdDrawIndexed((VkCommandBuffer)cmdPool->GetCurrentFrameCommandBuffer(), indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void VulkanRenderDevice::DispatchCompute(Ref<CommandPool> cmdPool, Ref<ComputePipeline> computePipeline, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
		computePipeline->As<VulkanComputePipeline>()->RTDispatch(cmdPool->GetCurrentFrameCommandBuffer(), groupCountX, groupCountY, groupCountZ);
	}

	void VulkanRenderDevice::BeginRenderPass(Ref<RenderPass> renderPass, Ref<FrameBuffer> frameBuffer, Ref<CommandPool> cmdPool) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BeginRenderPass");

		Ref<VulkanRenderPass> vulkanRenderPass = renderPass->As<VulkanRenderPass>();
		Ref<VulkanFrameBuffer> vulkanFrameBuffer = frameBuffer->As<VulkanFrameBuffer>();

		VulkanRenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.CommandBuffer = (VkCommandBuffer)cmdPool->GetCurrentFrameCommandBuffer();
		renderPassBeginInfo.Width = frameBuffer->GetWidth();
		renderPassBeginInfo.Height = frameBuffer->GetHeight();

		if (vulkanFrameBuffer->IsInFlight())
			renderPassBeginInfo.VulkanFrameBuffer = vulkanFrameBuffer->GetVulkanHandles()[Renderer::GetCurrentFrameIndex()];
		else
			renderPassBeginInfo.VulkanFrameBuffer = vulkanFrameBuffer->GetVulkanHandles()[0];

		vulkanRenderPass->RTBegin(renderPassBeginInfo);
	}

	void VulkanRenderDevice::EndRenderPass(Ref<RenderPass> renderPass) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::EndRenderPass");
		renderPass->As<VulkanRenderPass>()->RTEnd();
	}

	void VulkanRenderDevice::BeginDebugMarker(Ref<CommandPool> cmdPool, const char* labelName) {
#if LUCY_DEBUG
		VkDebugUtilsLabelEXT labelInfo{};
		labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		labelInfo.pLabelName = labelName;
		VulkanExternalFuncLinkage::vkCmdBeginDebugUtilsLabelEXT((VkCommandBuffer)cmdPool->GetCurrentFrameCommandBuffer(), &labelInfo);
#endif
	}

	void VulkanRenderDevice::EndDebugMarker(Ref<CommandPool> cmdPool) {
#if LUCY_DEBUG
		VulkanExternalFuncLinkage::vkCmdEndDebugUtilsLabelEXT((VkCommandBuffer)cmdPool->GetCurrentFrameCommandBuffer());
#endif
	}

	void VulkanRenderDevice::SubmitImmediateCommand(const std::function<void(VkCommandBuffer)>& func, const Ref<VulkanTransientCommandPool>& cmdPool) {
		VkCommandBuffer commandBuffer = cmdPool->BeginSingleTimeCommand(m_LogicalDevice);
		func(commandBuffer);
		cmdPool->EndSingleTimeCommand();

		SubmitWorkToGPU(m_GraphicsQueue, 1, cmdPool->GetTransientCommandBuffer());
	}

	void VulkanRenderDevice::WaitForDevice() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::WaitForDevice");
		LUCY_VK_ASSERT(vkDeviceWaitIdle(m_LogicalDevice));
	}

	void VulkanRenderDevice::WaitForQueue(TargetQueueFamily queueFamily) {
		switch (queueFamily) {
			using enum Lucy::TargetQueueFamily;
			case Graphics: {
				LUCY_VK_ASSERT(vkQueueWaitIdle(m_GraphicsQueue));
				break;
			}
			case Compute: {
				LUCY_VK_ASSERT(vkQueueWaitIdle(m_ComputeQueue));
				break;
			}
			case Transfer: {
				LUCY_VK_ASSERT(vkQueueWaitIdle(m_TransferQueue));
				break;
			}
			default:
				LUCY_ASSERT(false);
		}
	}
}