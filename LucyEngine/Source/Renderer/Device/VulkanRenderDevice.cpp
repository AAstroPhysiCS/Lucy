#include "lypch.h"
#include "VulkanRenderDevice.h"
#include "RenderDeviceScene.h"

#include "Renderer/Context/VulkanContext.h"

#include "Renderer/Pipeline/VulkanGraphicsPipeline.h"
#include "Renderer/Pipeline/VulkanComputePipeline.h"

#include "Renderer/Descriptors/VulkanDescriptorSet.h"

#include "Renderer/ExecutionBatch.h"
#include "Renderer/RenderGraph/RenderGraphPass.h"

#include "Renderer/Commands/VulkanCommandPool.h"

#include "Renderer/Memory/Buffer/Vulkan/VulkanVertexBuffer.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanIndexBuffer.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanFrameBuffer.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanDeviceAddressBuffer.h"
#include "Renderer/Memory/VulkanAllocator.h"
#include "Renderer/Memory/VulkanRenderDeviceUploadManager.h"

#include "Renderer/Descriptors/DescriptorSetManager.h"

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

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		LUCY_VK_ASSERT(vkCreateFence(m_LogicalDevice, &fenceCreateInfo, nullptr, &m_ImmediateSubmitFence));

		m_DescriptorSetManager = Memory::CreateUnique<VulkanDescriptorSetManager>(this);
		m_UploadManager = Memory::CreateUnique<VulkanRenderDeviceUploadManager>(shared_from_this()->As<VulkanRenderDevice>());
		m_DeviceScene = Memory::CreateUnique<RenderDeviceScene>(this);
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

		//For the slang shader not to give errors
		VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR derivativeFeatures{};
		derivativeFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_KHR;
		derivativeFeatures.computeDerivativeGroupLinear = VK_TRUE;
		derivativeFeatures.computeDerivativeGroupQuads = VK_TRUE;

		//For layered rendering (cubemaps for example)
		VkPhysicalDeviceMultiviewFeatures multiViewFeatures{};
		multiViewFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
		multiViewFeatures.multiview = VK_TRUE;
		multiViewFeatures.pNext = &derivativeFeatures;

		VkPhysicalDeviceVulkan12Features vulkan12Features{};
		vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		//For layered rendering (cubemaps for example)
		vulkan12Features.shaderOutputLayer = VK_TRUE;
		//For bindless descriptor sets
		vulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
		vulkan12Features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
		vulkan12Features.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
		vulkan12Features.descriptorBindingVariableDescriptorCount = VK_TRUE;
		vulkan12Features.drawIndirectCount = VK_TRUE;
		vulkan12Features.bufferDeviceAddress = VK_TRUE;
		vulkan12Features.vulkanMemoryModel = VK_TRUE;
		vulkan12Features.vulkanMemoryModelDeviceScope = VK_TRUE;
		vulkan12Features.scalarBlockLayout = VK_TRUE;
		vulkan12Features.descriptorIndexing = VK_TRUE;
		vulkan12Features.runtimeDescriptorArray = VK_TRUE;
		vulkan12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
		vulkan12Features.timelineSemaphore = VK_TRUE;
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
		features.pNext = &vulkan13Features;

		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
		deviceCreateInfo.pNext = &features;

		deviceCreateInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();
		deviceCreateInfo.enabledExtensionCount = (uint32_t)m_DeviceExtensions.size();
		deviceCreateInfo.enabledLayerCount = 0;
		deviceCreateInfo.ppEnabledLayerNames = 0;

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

	void VulkanRenderDevice::SubmitWorkToGPU(const RenderCommandList& renderCommandList, VulkanSemaphore& waitSemaphore, VkPipelineStageFlags2 waitStage, 
		VulkanSemaphore& renderFinishedSemaphore, VulkanSemaphore& frameTimelineSemaphore, uint64_t signalValue) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::SubmitWorkToGPU");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		const auto& cmdPool = renderCommandList.GetPrimaryCommandPool();

		LUCY_ASSERT(cmdPool->GetState(frameIndex) == CommandBufferSlotState::Recorded);

		VkCommandBuffer cmd = (VkCommandBuffer)cmdPool->GetCommandBuffer(frameIndex);

		VkCommandBufferSubmitInfo cmdInfo{};
		cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		cmdInfo.commandBuffer = cmd;
		cmdInfo.deviceMask = 0;

		VkSemaphoreSubmitInfo waitInfo{};
		waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		waitInfo.semaphore = (VkSemaphore)waitSemaphore.GetHandle();
		waitInfo.value = 0;
		waitInfo.stageMask = waitStage;
		waitInfo.deviceIndex = 0;

		std::array<VkSemaphoreSubmitInfo, 2> signalInfos{};

		signalInfos[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalInfos[0].semaphore = (VkSemaphore)renderFinishedSemaphore.GetHandle();
		signalInfos[0].value = 0;
		signalInfos[0].stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		signalInfos[0].deviceIndex = 0;

		signalInfos[1].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalInfos[1].semaphore = (VkSemaphore)frameTimelineSemaphore.GetHandle();
		signalInfos[1].value = signalValue;
		signalInfos[1].stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
		signalInfos[1].deviceIndex = 0;

		VkSubmitInfo2 submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfo.waitSemaphoreInfoCount = 1;
		submitInfo.pWaitSemaphoreInfos = &waitInfo;
		submitInfo.commandBufferInfoCount = 1;
		submitInfo.pCommandBufferInfos = &cmdInfo;
		submitInfo.signalSemaphoreInfoCount = (uint32_t)signalInfos.size();
		submitInfo.pSignalSemaphoreInfos = signalInfos.data();

		LUCY_VK_ASSERT(vkQueueSubmit2(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));

		cmdPool->SetState(frameIndex, CommandBufferSlotState::Pending);
	}

	void VulkanRenderDevice::SubmitWorkToGPUAsBatch(const RenderCommandList& renderCommandList, const ExecutionBatch& batch) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::SubmitWorkToGPUAsBatch");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		const auto& vkBatch = batch.AsVulkanBatch();

		auto queueFamily = vkBatch.Passes[0]->GetTargetQueueFamily();
		const auto& waits = vkBatch.Waits;
		const auto& signals = vkBatch.Signals;

		const auto GetQueueHandle = [&](TargetQueueFamily family) -> VkQueue {
			switch (family) {
				case TargetQueueFamily::Graphics:
					return m_GraphicsQueue;
				case TargetQueueFamily::Compute:
					return m_ComputeQueue;
				case TargetQueueFamily::Transfer:
					return m_TransferQueue;
				default:
					LUCY_ASSERT(false, "Invalid queue family!");
					return VK_NULL_HANDLE;
			}
		}; 

		VkQueue queueHandle = GetQueueHandle(queueFamily);

		const auto& cmdPool = renderCommandList.GetPrimaryCommandPool();
		LUCY_ASSERT(cmdPool->GetState(frameIndex) == CommandBufferSlotState::Recorded, "Trying to submit a command buffer that is not recorded!");

		VkCommandBuffer cmd = static_cast<VkCommandBuffer>(cmdPool->GetCommandBuffer(frameIndex));

		VkCommandBufferSubmitInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		info.commandBuffer = cmd;
		info.deviceMask = 0;

		/*cmdInfos.reserve(submitCmds.size());
		for (VkCommandBuffer cmd : submitCmds) {
			VkCommandBufferSubmitInfo info{};
			info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
			info.commandBuffer = cmd;
			info.deviceMask = 0;
			cmdInfos.push_back(info);
		}*/

		std::vector<VkSemaphoreSubmitInfo> waitInfos;
		waitInfos.reserve(waits.size());
		for (const auto& wait : waits) {
			VkSemaphoreSubmitInfo& info = waitInfos.emplace_back();
			info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
			info.semaphore = static_cast<VkSemaphore>(wait.Semaphore.GetHandle());
			info.value = wait.Value;
			info.stageMask = wait.StageMask;
		}

		std::vector<VkSemaphoreSubmitInfo> signalInfos;
		signalInfos.reserve(signals.size());
		for (const auto& signal : signals) {
			VkSemaphoreSubmitInfo& info = signalInfos.emplace_back();
			info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
			info.semaphore = static_cast<VkSemaphore>(signal.Semaphore.GetHandle());
			info.value = signal.Value;
			info.stageMask = signal.StageMask;
		}

		VkSubmitInfo2 submitInfo{ VulkanAPI::QueueSubmitInfo2(1, &info,
			static_cast<uint32_t>(waitInfos.size()), waitInfos.data(), static_cast<uint32_t>(signalInfos.size()), signalInfos.data()) };

		LUCY_VK_ASSERT(vkQueueSubmit2(queueHandle, 1, &submitInfo, VK_NULL_HANDLE));
		cmdPool->SetState(frameIndex, CommandBufferSlotState::Pending);
	}

	void VulkanRenderDevice::SubmitWorkToGPUImmediate(VkQueue queueHandle, size_t commandBufferCount, void* commandBufferHandles) const {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::SubmitWorkToGPUImmediate");

		LUCY_ASSERT(queueHandle != VK_NULL_HANDLE, "Cannot submit work to a null queue!");
		LUCY_ASSERT(commandBufferCount > 0, "Cannot submit an empty command buffer list!");

		std::vector<VkCommandBufferSubmitInfo> commandBufferInfos;
		commandBufferInfos.reserve(commandBufferCount);

		VkCommandBuffer* cmdBuffers = static_cast<VkCommandBuffer*>(commandBufferHandles);
		for (size_t i = 0; i < commandBufferCount; ++i) {
			VkCommandBuffer commandBuffer = cmdBuffers[i];
			LUCY_ASSERT(commandBuffer != VK_NULL_HANDLE, "Cannot submit a null command buffer!");

			VkCommandBufferSubmitInfo& commandBufferInfo = commandBufferInfos.emplace_back();
			commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
			commandBufferInfo.commandBuffer = commandBuffer;
			commandBufferInfo.deviceMask = 0;
		}

		vkResetFences(m_LogicalDevice, 1, &m_ImmediateSubmitFence);

		VkSubmitInfo submitInfo = VulkanAPI::QueueSubmitInfo(commandBufferCount, (VkCommandBuffer*)&commandBufferHandles, 0, nullptr, nullptr, 0, nullptr);
		LUCY_VK_ASSERT(vkQueueSubmit(queueHandle, 1, &submitInfo, m_ImmediateSubmitFence));
		LUCY_VK_ASSERT(vkWaitForFences(m_LogicalDevice, 1, &m_ImmediateSubmitFence, VK_TRUE, UINT64_MAX));
	}

	void VulkanRenderDevice::PrintDeviceInfo() {
		LUCY_INFO(std::format("Selected Device: {0}", m_DeviceInfo.Name));
	}

	void VulkanRenderDevice::Destroy() {
		LUCY_PROFILE_DESTROY();
		auto& scene = GetScene();
		scene->RTDestroy();

		m_DescriptorSetManager->RTDestroy();
		m_UploadManager->Destroy(m_Allocator);
		m_Allocator.Destroy();
		vkDestroyFence(m_LogicalDevice, m_ImmediateSubmitFence, nullptr);
		vkDestroyDevice(m_LogicalDevice, nullptr);
	}

	void VulkanRenderDevice::BeginCommandBuffer(Ref<CommandPool> cmdPool) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BeginCommandBuffer");

		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		LUCY_ASSERT(cmdPool->GetState(frameIndex) == CommandBufferSlotState::Ready, "Current frame-slot command buffer is not ready!");

		auto cmdBufferBeginInfo = VulkanAPI::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		vkBeginCommandBuffer((VkCommandBuffer)cmdPool->GetCommandBuffer(frameIndex), &cmdBufferBeginInfo);

		cmdPool->SetState(frameIndex, CommandBufferSlotState::Recording);
	}

	void VulkanRenderDevice::EndCommandBuffer(Ref<CommandPool> cmdPool) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::EndCommandBuffer");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		LUCY_ASSERT(cmdPool->GetState(frameIndex) == CommandBufferSlotState::Recording, "Current frame-slot command buffer is not recording!");

		vkEndCommandBuffer((VkCommandBuffer)cmdPool->GetCommandBuffer(frameIndex));
		cmdPool->SetState(frameIndex, CommandBufferSlotState::Recorded);
	}

	void VulkanRenderDevice::FillBuffer(Ref<CommandPool> cmdPool, Ref<RenderDeviceBuffer> buffer, size_t offset, size_t size, uint32_t value) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::FillBuffer");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		vkCmdFillBuffer((VkCommandBuffer)cmdPool->GetCommandBuffer(frameIndex), buffer->As<VulkanDeviceAddressBuffer>()->GetVulkanBufferHandle(), offset, size, value);
	}

	void VulkanRenderDevice::CopyBuffer(Ref<CommandPool> cmdPool, Ref<RenderDeviceBuffer> srcBuffer, Ref<RenderDeviceBuffer> dstBuffer, size_t srcOffset, size_t dstOffset, size_t size) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::CopyBuffer");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(cmdPool->GetCommandBuffer(frameIndex));

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = srcOffset;
		copyRegion.dstOffset = dstOffset;
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer->As<VulkanDeviceAddressBuffer>()->GetVulkanBufferHandle(), 
			dstBuffer->As<VulkanDeviceAddressBuffer>()->GetVulkanBufferHandle(), 1, &copyRegion);
	}

	void VulkanRenderDevice::CopyBuffer(Ref<CommandPool> cmdPool, Ref<RenderDeviceBuffer> srcBuffer, Ref<RenderDeviceBuffer> dstBuffer, const std::vector<const void*>& regions) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::CopyBuffer");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(cmdPool->GetCommandBuffer(frameIndex));

		vkCmdCopyBuffer(commandBuffer, srcBuffer->As<VulkanDeviceAddressBuffer>()->GetVulkanBufferHandle(), 
			dstBuffer->As<VulkanDeviceAddressBuffer>()->GetVulkanBufferHandle(), regions.size(), reinterpret_cast<const VkBufferCopy*>(regions.data()));
	}

	void VulkanRenderDevice::BindBuffers(Ref<CommandPool> cmdPool, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindBuffers");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		VulkanVertexBindInfo vertexInfo;
		vertexInfo.CommandBuffer = (VkCommandBuffer)cmdPool->GetCommandBuffer(frameIndex);
		vertexBuffer->As<VulkanVertexBuffer>()->RTBind(vertexInfo);

		VulkanIndexBindInfo indexInfo;
		indexInfo.CommandBuffer = vertexInfo.CommandBuffer;
		indexBuffer->As<VulkanIndexBuffer>()->RTBind(indexInfo);
	}
	
	void VulkanRenderDevice::BindBuffers(Ref<CommandPool> cmdPool, Ref<RenderDeviceBuffer> indexBuffer) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindBuffers");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(cmdPool->GetCommandBuffer(frameIndex));

		auto vulkanIndexBuffer = indexBuffer->As<VulkanDeviceAddressBuffer>();
		vkCmdBindIndexBuffer(commandBuffer, vulkanIndexBuffer->GetVulkanBufferHandle(), 0, VK_INDEX_TYPE_UINT32);
	}

	//TODO: Clean this up
	RenderDeviceTextureHandle VulkanRenderDevice::BindGlobalImageHandleTo(const std::string& imageBufferName, const Ref<GraphicsPipeline>& pipeline, const Ref<Image>& image, uint32_t mip) {
		const auto& descriptorSetHandle = pipeline->As<VulkanGraphicsPipeline>()->GetDescriptorSetHandles()[VulkanDescriptorSetManager::TEXTURE_BINDLESS_TABLE_SET_INDEX];
		const auto& descriptorSet = AccessResource<VulkanDescriptorSet>(descriptorSetHandle);

		if (auto imageSampler = descriptorSet->GetVulkanImageSampler(imageBufferName)) {
			const auto& vulkanImage = image->As<VulkanImage>();
			const auto& imageHandle = image->GetMyHandle();
			VkImageView imageView = mip == static_cast<uint32_t>(-1) ? vulkanImage->GetImageView().GetVulkanHandle() : vulkanImage->GetImageView().GetMipViewVulkanHandle(mip);

			for (uint32_t index = 0; const auto& slot : imageSampler->Images) {
				if (!slot.Alive) {
					index++;
					continue;
				}

				if (slot.Data.ImageHandle == imageHandle && slot.Data.Mip == mip) {
					return RenderDeviceTextureHandle{
						.Index = index,
						.Generation = slot.Generation
					};
				}

				index++;
			}

			RenderDeviceTextureHandle handle = imageSampler->Images.Create(
				VulkanImageDescriptor{
					.ImageHandle = imageHandle,
					.Mip = mip,
					.ImageInfo = VulkanAPI::DescriptorImageInfo(
						vulkanImage->GetCurrentLayout(), 
						imageView, 
						Renderer::AccessResource<VulkanImageSampler>(vulkanImage->GetSamplerHandle())->GetVulkanHandle()
					)
				}
			);

			descriptorSet->RTUpdateImageSamplerDescriptors(this, imageBufferName, handle);

			return handle;
		}

		LUCY_ASSERT(false, "Graphics::BindGlobalImageHandleTo did not work for name: {0}", imageBufferName);
		return {};
	}
	
	//TODO: Clean this up
	RenderDeviceTextureHandle VulkanRenderDevice::BindGlobalImageHandleTo(const std::string& imageBufferName, const Ref<ComputePipeline>& pipeline, const Ref<Image>& image, uint32_t mip) {
		const auto& descriptorSetHandle = pipeline->As<VulkanComputePipeline>()->GetDescriptorSetHandles()[VulkanDescriptorSetManager::TEXTURE_BINDLESS_TABLE_SET_INDEX];
		const auto& descriptorSet = AccessResource<VulkanDescriptorSet>(descriptorSetHandle);

		if (auto imageSampler = descriptorSet->GetVulkanImageSampler(imageBufferName)) {
			const auto& vulkanImage = image->As<VulkanImage>();
			const auto& imageHandle = image->GetMyHandle();
			VkImageView imageView = mip == static_cast<uint32_t>(-1) ? vulkanImage->GetImageView().GetVulkanHandle() : vulkanImage->GetImageView().GetMipViewVulkanHandle(mip);

			for (uint32_t index = 0; const auto& slot : imageSampler->Images) {
				if (!slot.Alive) {
					index++;
					continue;
				}

				if (slot.Data.ImageHandle == imageHandle && slot.Data.Mip == mip) {
					return RenderDeviceTextureHandle{
						.Index = index,
						.Generation = slot.Generation
					};
				}

				index++;
			}

			RenderDeviceTextureHandle handle = imageSampler->Images.Create(
				VulkanImageDescriptor{
					.ImageHandle = imageHandle,
					.Mip = mip,
					.ImageInfo = VulkanAPI::DescriptorImageInfo(
						vulkanImage->GetCurrentLayout(),
						imageView,
						Renderer::AccessResource<VulkanImageSampler>(vulkanImage->GetSamplerHandle())->GetVulkanHandle()
					)
				}
			);

			descriptorSet->RTUpdateImageSamplerDescriptors(this, imageBufferName, handle);

			return handle;
		}

		LUCY_ASSERT(false, "Compute::BindGlobalImageHandleTo did not work for name: {0}", imageBufferName);
		return {};
	}
	
	void VulkanRenderDevice::BindPushConstant(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline, const PipelineConstant& pushConstant) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindPushConstant | Graphics");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		pushConstant.RTBind((VkCommandBuffer)cmdPool->GetCommandBuffer(frameIndex), pipeline->As<VulkanGraphicsPipeline>()->GetPipelineLayout());
	}

	void VulkanRenderDevice::BindPushConstant(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline, const PipelineConstant& pushConstant) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindPushConstant | Compute");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		pushConstant.RTBind((VkCommandBuffer)cmdPool->GetCommandBuffer(frameIndex), pipeline->As<VulkanComputePipeline>()->GetPipelineLayout());
	}

	void VulkanRenderDevice::BindPipeline(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindPipeline | Graphics");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		pipeline->RTBind(cmdPool->GetCommandBuffer(frameIndex));
	}

	void VulkanRenderDevice::BindPipeline(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindPipeline | Compute");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		pipeline->RTBind(cmdPool->GetCommandBuffer(frameIndex));
	}

	void VulkanRenderDevice::UpdateDescriptorSets(Ref<GraphicsPipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::UpdateDescriptorSets | Graphics");
		const auto& castedPipeline = pipeline->As<VulkanGraphicsPipeline>();
		const auto& descriptorSetHandles = castedPipeline->GetDescriptorSetHandles();

		for (auto handle : descriptorSetHandles) {
			Ref<VulkanDescriptorSet> vulkanSet = AccessResource<VulkanDescriptorSet>(handle);
			vulkanSet->RTUpdate(this);
		}
	}
	
	void VulkanRenderDevice::UpdateDescriptorSets(Ref<ComputePipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::UpdateDescriptorSets | Compute");
		const auto& castedPipeline = pipeline->As<VulkanComputePipeline>();
		const auto& descriptorSetHandles = castedPipeline->GetDescriptorSetHandles();

		for (auto handle : descriptorSetHandles) {
			Ref<VulkanDescriptorSet> vulkanSet = AccessResource<VulkanDescriptorSet>(handle);
			vulkanSet->RTUpdate(this);
		}
	}

	void VulkanRenderDevice::BindAllDescriptorSets(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindAllDescriptorSets | Graphics");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		const auto& castedPipeline = pipeline->As<VulkanGraphicsPipeline>();
		const auto& descriptorSetHandles = castedPipeline->GetDescriptorSetHandles();

		VulkanDescriptorSetBindInfo bindInfo;
		bindInfo.CommandBuffer = (VkCommandBuffer)cmdPool->GetCommandBuffer(frameIndex);
		bindInfo.PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		bindInfo.PipelineLayout = castedPipeline->GetPipelineLayout();

		for (auto handle : descriptorSetHandles) {
			Ref<VulkanDescriptorSet> vulkanSet = AccessResource<VulkanDescriptorSet>(handle);
			vulkanSet->RTBind(bindInfo);
		}
	}

	void VulkanRenderDevice::BindDescriptorSet(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline, uint32_t setIndex) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindDescriptorSet | Graphics");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		const auto& castedPipeline = pipeline->As<VulkanGraphicsPipeline>();
		const auto& descriptorSetHandles = castedPipeline->GetDescriptorSetHandles();

		VulkanDescriptorSetBindInfo bindInfo;
		bindInfo.CommandBuffer = (VkCommandBuffer)cmdPool->GetCommandBuffer(frameIndex);
		bindInfo.PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		bindInfo.PipelineLayout = castedPipeline->GetPipelineLayout();

		for (auto handle : descriptorSetHandles) {
			const auto& vulkanSet = AccessResource<VulkanDescriptorSet>(handle);
			if (vulkanSet->GetSetIndex() == setIndex) {
				vulkanSet->RTBind(bindInfo);
				break;
			}
		}
	}

	void VulkanRenderDevice::BindAllDescriptorSets(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindAllDescriptorSets | Compute");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		const auto& castedPipeline = pipeline->As<VulkanComputePipeline>();
		const auto& descriptorSetHandles = castedPipeline->GetDescriptorSetHandles();

		VulkanDescriptorSetBindInfo bindInfo;
		bindInfo.CommandBuffer = (VkCommandBuffer)cmdPool->GetCommandBuffer(frameIndex);
		bindInfo.PipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
		bindInfo.PipelineLayout = castedPipeline->GetPipelineLayout();

		for (auto handle : descriptorSetHandles) {
			const auto& vulkanSet = AccessResource<VulkanDescriptorSet>(handle);
			vulkanSet->RTBind(bindInfo);
		}
	}

	void VulkanRenderDevice::BindDescriptorSet(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline, uint32_t setIndex) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindDescriptorSet | Compute");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		const auto& castedPipeline = pipeline->As<VulkanComputePipeline>();
		const auto& descriptorSetHandles = castedPipeline->GetDescriptorSetHandles();

		VulkanDescriptorSetBindInfo bindInfo;
		bindInfo.CommandBuffer = (VkCommandBuffer)cmdPool->GetCommandBuffer(frameIndex);
		bindInfo.PipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
		bindInfo.PipelineLayout = castedPipeline->GetPipelineLayout();

		for (auto handle : descriptorSetHandles) {
			const auto& vulkanSet = AccessResource<VulkanDescriptorSet>(handle);
			if (vulkanSet->GetSetIndex() == setIndex) {
				vulkanSet->RTBind(bindInfo);
				break;
			}
		}
	}

	void VulkanRenderDevice::DrawIndexedIndirectCount(Ref<CommandPool> cmdPool, Ref<RenderDeviceBuffer> buffer, size_t offset, Ref<RenderDeviceBuffer> countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::DrawIndexedIndirectCount");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		vkCmdDrawIndexedIndirectCount((VkCommandBuffer)cmdPool->GetCommandBuffer(frameIndex), buffer->As<VulkanDeviceAddressBuffer>()->GetVulkanBufferHandle(), offset, 
			countBuffer->As<VulkanDeviceAddressBuffer>()->GetVulkanBufferHandle(), countBufferOffset, maxDrawCount, stride);
	}

	void VulkanRenderDevice::DrawIndexed(Ref<CommandPool> cmdPool, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::DrawIndexed");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		vkCmdDrawIndexed((VkCommandBuffer)cmdPool->GetCommandBuffer(frameIndex), indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void VulkanRenderDevice::DispatchCompute(Ref<CommandPool> cmdPool, Ref<ComputePipeline> computePipeline, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::DispatchCompute");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		computePipeline->As<VulkanComputePipeline>()->RTDispatch(cmdPool->GetCommandBuffer(frameIndex), groupCountX, groupCountY, groupCountZ);
	}

	void VulkanRenderDevice::DispatchComputeIndirect(Ref<CommandPool> cmdPool, Ref<RenderDeviceBuffer> buffer, size_t offset) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::DispatchComputeIndirect");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		vkCmdDispatchIndirect((VkCommandBuffer)cmdPool->GetCommandBuffer(frameIndex), buffer->As<VulkanDeviceAddressBuffer>()->GetVulkanBufferHandle(), offset);
	}

	void VulkanRenderDevice::BeginRenderPass(Ref<RenderPass> renderPass, Ref<FrameBuffer> frameBuffer, Ref<CommandPool> cmdPool) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BeginRenderPass");
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		Ref<VulkanRenderPass> vulkanRenderPass = renderPass->As<VulkanRenderPass>();
		Ref<VulkanFrameBuffer> vulkanFrameBuffer = frameBuffer->As<VulkanFrameBuffer>();

		VulkanRenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.CommandBuffer = (VkCommandBuffer)cmdPool->GetCommandBuffer(frameIndex);
		renderPassBeginInfo.Width = frameBuffer->GetWidth();
		renderPassBeginInfo.Height = frameBuffer->GetHeight();

		if (vulkanFrameBuffer->IsInFlight())
			renderPassBeginInfo.VulkanFrameBuffer = vulkanFrameBuffer->GetVulkanHandles()[frameIndex];
		else
			renderPassBeginInfo.VulkanFrameBuffer = vulkanFrameBuffer->GetVulkanHandles()[0];

		vulkanRenderPass->RTBegin(renderPassBeginInfo);
	}

	void VulkanRenderDevice::EndRenderPass(Ref<RenderPass> renderPass) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::EndRenderPass");
		renderPass->As<VulkanRenderPass>()->RTEnd();
	}

	void VulkanRenderDevice::BeginDebugMarker(Ref<CommandPool> cmdPool, const char* labelName) {
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
#if LUCY_DEBUG
		VkDebugUtilsLabelEXT labelInfo{};
		labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		labelInfo.pLabelName = labelName;
		VulkanExternalFuncLinkage::vkCmdBeginDebugUtilsLabelEXT((VkCommandBuffer)cmdPool->GetCommandBuffer(frameIndex), &labelInfo);
#endif
	}

	void VulkanRenderDevice::BeginDebugMarker(VkCommandBuffer commandBuffer, const char* labelName) {
#if LUCY_DEBUG
		VkDebugUtilsLabelEXT labelInfo{};
		labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		labelInfo.pLabelName = labelName;
		VulkanExternalFuncLinkage::vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &labelInfo);
#endif
	}

	void VulkanRenderDevice::EndDebugMarker(Ref<CommandPool> cmdPool) {
		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
#if LUCY_DEBUG
		VulkanExternalFuncLinkage::vkCmdEndDebugUtilsLabelEXT((VkCommandBuffer)cmdPool->GetCommandBuffer(frameIndex));
#endif
	}

	void VulkanRenderDevice::EndDebugMarker(VkCommandBuffer commandBuffer) {
#if LUCY_DEBUG
		VulkanExternalFuncLinkage::vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
	}

	void VulkanRenderDevice::RegisterShaderBindings(const Ref<Shader>& shader) {
		m_DescriptorSetManager->RegisterShaderBindings(shader);
	}

	void VulkanRenderDevice::SubmitImmediateCommand(const std::function<void(VkCommandBuffer)>& func, const Ref<VulkanTransientCommandPool>& cmdPool) {
		VkCommandBuffer commandBuffer = cmdPool->BeginSingleTimeCommand(m_LogicalDevice);
		func(commandBuffer);
		cmdPool->EndSingleTimeCommand();

		SubmitWorkToGPUImmediate(m_GraphicsQueue, 1, cmdPool->GetTransientCommandBuffer());
	}

	void VulkanRenderDevice::WaitForDevice() {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::WaitForDevice");
		LUCY_VK_ASSERT(vkDeviceWaitIdle(m_LogicalDevice));
	}

	void VulkanRenderDevice::WaitForQueue(TargetQueueFamily queueFamily) {
		switch (queueFamily) {
			using enum Lucy::TargetQueueFamily;
			case Graphics: {
				LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::WaitForQueue::Graphics");
				LUCY_VK_ASSERT(vkQueueWaitIdle(m_GraphicsQueue));
				break;
			}
			case Compute: {
				LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::WaitForQueue::Compute");
				LUCY_VK_ASSERT(vkQueueWaitIdle(m_ComputeQueue));
				break;
			}
			case Transfer: {
				LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::WaitForQueue::Transfer");
				LUCY_VK_ASSERT(vkQueueWaitIdle(m_TransferQueue));
				break;
			}
			default:
				LUCY_ASSERT(false);
		}
	}

	std::vector<RenderDeviceResourceHandle> VulkanRenderDevice::GetResourceBindingHandles(const Ref<Shader>& shader) const {
		return m_DescriptorSetManager->GetDescriptorSetHandles(shader);
	}
}