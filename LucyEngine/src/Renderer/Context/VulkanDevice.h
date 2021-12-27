#pragma once

#include "vulkan/vulkan.hpp"

namespace Lucy {

	struct VulkanDeviceInfo {
		std::string Name;
		uint32_t DriverVersion;
		uint32_t ApiVersion;
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

		inline VkPhysicalDevice GetPhysicalDevice() { return m_PhysicalDevice; }
		inline VkDevice GetLogicalDevice() { return m_LogicalDevice; }
		inline QueueFamilyIndices GetQueueFamilies() { return m_QueueFamilyIndices; }
		inline VkQueue GetGraphicsQueue() { return m_GraphicsQueue; }
		inline VkQueue GetPresentQueue() { return m_PresentQueue; }
	private:
		void PickDeviceByRanking(std::vector<VkPhysicalDevice>& devices);
		void CreateLogicalDevice(std::vector<const char*>& enabledValidationLayers);
		void FindQueueFamilies(VkPhysicalDevice device);
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
		void PrintDeviceInfo();

		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_LogicalDevice = VK_NULL_HANDLE;
		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_PresentQueue = VK_NULL_HANDLE;

		std::vector<const char*> m_DeviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		VulkanDeviceInfo m_DeviceInfo;
		QueueFamilyIndices m_QueueFamilyIndices;
	};
}