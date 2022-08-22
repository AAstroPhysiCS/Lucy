#include "lypch.h"
#include "VulkanContextDevice.h"

namespace Lucy {

	VulkanContextDevice& VulkanContextDevice::Get() {
		static VulkanContextDevice s_Instance;
		return s_Instance;
	}

	void VulkanContextDevice::Create(VkInstance instance, std::vector<const char*>& enabledValidationLayers, VkSurfaceKHR surface) {
		m_Surface = surface;

		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			LUCY_CRITICAL("No physical device found that supports vulkan!");
			LUCY_ASSERT(false);
		}

		std::vector<VkPhysicalDevice> availablePhysicalDevices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, availablePhysicalDevices.data());

		PickDeviceByRanking(availablePhysicalDevices);
		PrintDeviceInfo();
		CreateLogicalDevice(enabledValidationLayers);

		vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.GraphicsFamily, 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.PresentFamily, 0, &m_PresentQueue);
	}

	void VulkanContextDevice::Destroy() {
		LUCY_PROFILE_DESTROY();
		vkDestroyDevice(m_LogicalDevice, nullptr);
	}

	void VulkanContextDevice::PickDeviceByRanking(const std::vector<VkPhysicalDevice>& devices) {

		LUCY_INFO("----------Available Devices----------");

		for (const auto& device : devices) {
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(device, &properties);

			VkPhysicalDeviceFeatures features;
			vkGetPhysicalDeviceFeatures(device, &features);

			VulkanDeviceInfo deviceInfo = { properties.deviceName, properties.driverVersion, properties.apiVersion };
			LUCY_INFO(fmt::format("Device Name: {0}", deviceInfo.Name));
			LUCY_INFO(fmt::format("Device Driver Version: {0}", deviceInfo.DriverVersion));
			LUCY_INFO(fmt::format("Device API Version: {0}", deviceInfo.ApiVersion));
			LUCY_INFO("-------------------------------------");
			deviceInfo.MinUniformBufferAlignment = properties.limits.minUniformBufferOffsetAlignment;

			FindQueueFamilies(device);

			bool isDeviceRequirementsCovered = CheckDeviceExtensionSupport(device) && CheckDeviceFormatSupport(device);

			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
				&& features.multiViewport
				&& features.geometryShader
				&& features.samplerAnisotropy
				&& m_QueueFamilyIndices.IsComplete()
				&& isDeviceRequirementsCovered) {
				m_DeviceInfo = deviceInfo;
				m_PhysicalDevice = device;
				return;
			}
		}

		LUCY_CRITICAL("No suitable device found!");
		LUCY_ASSERT(false);
	}

	void VulkanContextDevice::CreateLogicalDevice(std::vector<const char*>& enabledValidationLayers) {

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { m_QueueFamilyIndices.GraphicsFamily, m_QueueFamilyIndices.PresentFamily };

		float priority = 1.0f;
		for (uint32_t family : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = family;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &priority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures2 features{};
		features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &features);

		// This should be already set to VK_TRUE, as we queried before.
		features.features.samplerAnisotropy = VK_TRUE;

		//for bindless descriptor sets
		VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
		indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;

		// This should be already set to VK_TRUE, as we queried before.
		indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
		indexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
		indexingFeatures.runtimeDescriptorArray = VK_TRUE;

		features.pNext = &indexingFeatures; //extending this structure

		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
		//deviceCreateInfo.pEnabledFeatures = &features;
		deviceCreateInfo.pNext = &features;

		deviceCreateInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();
		deviceCreateInfo.enabledExtensionCount = m_DeviceExtensions.size();

#ifdef LUCY_DEBUG
		deviceCreateInfo.enabledLayerCount = enabledValidationLayers.size();
		deviceCreateInfo.ppEnabledLayerNames = enabledValidationLayers.data();
#else
		deviceCreateInfo.enabledLayerCount = 0;
		deviceCreateInfo.ppEnabledLayerNames = 0;
#endif

		LUCY_VK_ASSERT(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_LogicalDevice));
	}

	void VulkanContextDevice::FindQueueFamilies(VkPhysicalDevice device) {
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		uint32_t i = 0;
		for (const auto& queueFamily : queueFamilies) {
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);

			if (presentSupport) {
				m_QueueFamilyIndices.PresentFamilyHasValue = true;
				m_QueueFamilyIndices.PresentFamily = i;
			}

			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
				m_QueueFamilyIndices.GraphicsFamilyHasValue = true;
				m_QueueFamilyIndices.GraphicsFamily = i;
				break;
			}
			i++;
		}
	}

	bool VulkanContextDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		if (!requiredExtensions.empty()) {
			LUCY_INFO("Not available extensions");
			uint32_t i = 0;
			for (const auto& notAvailableExtensions : requiredExtensions) {
				LUCY_INFO(fmt::format("Index: {0}, Name: {1}", (i++), notAvailableExtensions));
			}
			LUCY_ASSERT(false);
		}

		return requiredExtensions.empty();
	}

	bool VulkanContextDevice::CheckDeviceFormatSupport(VkPhysicalDevice device) {
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

	void VulkanContextDevice::PrintDeviceInfo() {
		LUCY_INFO(fmt::format("Selected Device: {0}", m_DeviceInfo.Name));
	}
}
