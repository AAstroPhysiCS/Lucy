#pragma once

#include "vulkan/vulkan.h"

namespace Lucy {

	struct VulkanDeviceInfo {
		std::string Name;
		uint32_t DriverVersion;
		uint32_t ApiVersion;
		uint32_t MinUniformBufferAlignment = 0;
	};

	struct QueueFamilyIndices {
		uint32_t GraphicsFamily;
		uint32_t PresentFamily;
		bool GraphicsFamilyHasValue = false;
		bool PresentFamilyHasValue = false;

		bool IsComplete() { return GraphicsFamilyHasValue && PresentFamilyHasValue; }
	};

	class VulkanDevice {
	private:
		VulkanDevice() = default;
	public:
		~VulkanDevice() = default;
		static VulkanDevice& Get();

		inline VulkanDeviceInfo& GetDeviceInformation() { return m_DeviceInfo; }
		void Create(VkInstance instance, std::vector<const char*>& enabledValidationLayers);
		void Destroy();

		inline VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		inline VkDevice GetLogicalDevice() const { return m_LogicalDevice; }
		inline QueueFamilyIndices GetQueueFamilies() const { return m_QueueFamilyIndices; }
		
		inline VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		inline VkQueue GetPresentQueue() const { return m_PresentQueue; }
		
		inline uint32_t GetMinUniformBufferOffsetAlignment() const { return m_DeviceInfo.MinUniformBufferAlignment; }
	private:
		void PickDeviceByRanking(const std::vector<VkPhysicalDevice>& devices);
		void CreateLogicalDevice(std::vector<const char*>& enabledValidationLayers);
		
		void FindQueueFamilies(VkPhysicalDevice device);
		
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
		bool CheckDeviceFormatSupport(VkPhysicalDevice device);
		void PrintDeviceInfo();

		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_LogicalDevice = VK_NULL_HANDLE;
		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_PresentQueue = VK_NULL_HANDLE;

		std::vector<const char*> m_DeviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		};

		std::vector<VkFormat> m_DeviceFormatSupportToCheck{
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_FORMAT_R8G8B8A8_SNORM,
			VK_FORMAT_B8G8R8A8_SRGB,
			VK_FORMAT_B8G8R8A8_SNORM,
		};

		VulkanDeviceInfo m_DeviceInfo;
		QueueFamilyIndices m_QueueFamilyIndices;
	};
}