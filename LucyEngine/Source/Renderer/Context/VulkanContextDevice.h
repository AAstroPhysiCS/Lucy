#pragma once

#include "vulkan/vulkan.h"

namespace Lucy {

	struct VulkanDeviceInfo {
		std::string Name;
		uint32_t DriverVersion = 0;
		uint32_t ApiVersion = 0;
		uint32_t MinUniformBufferAlignment = 0;
		float TimestampPeriod = 0;
	};

	struct QueueFamilyIndices {
		uint32_t GraphicsFamily = UINT32_MAX;
		uint32_t PresentFamily = UINT32_MAX;
		uint32_t ComputeFamily = UINT32_MAX;
		bool GraphicsFamilyHasValue = false;
		bool PresentFamilyHasValue = false;
		bool ComputeFamilyHasValue = false;

		bool IsComplete() { return GraphicsFamilyHasValue && PresentFamilyHasValue && ComputeFamilyHasValue; }
	};

	class VulkanContextDevice {
	private:
		VulkanContextDevice() = default;
	public:
		~VulkanContextDevice() = default;
		static VulkanContextDevice& Get();

		inline VulkanDeviceInfo& GetDeviceInformation() { return m_DeviceInfo; }
		void Create(VkInstance instance, std::vector<const char*>& enabledValidationLayers, VkSurfaceKHR surface);
		void Destroy();

		inline VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		inline VkDevice GetLogicalDevice() const { return m_LogicalDevice; }
		inline QueueFamilyIndices GetQueueFamilies() const { return m_QueueFamilyIndices; }
		
		inline VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		inline VkQueue GetPresentQueue() const { return m_PresentQueue; }
		inline VkQueue GetComputeQueue() const { return m_ComputeQueue; }
		
		inline uint32_t GetMinUniformBufferOffsetAlignment() const { return m_DeviceInfo.MinUniformBufferAlignment; }
		inline float GetTimestampPeriod() const { return m_DeviceInfo.TimestampPeriod; }
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
		VkQueue m_ComputeQueue = VK_NULL_HANDLE;

		std::vector<const char*> m_DeviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		};

		std::vector<VkFormat> m_DeviceFormatSupportToCheck = {
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_FORMAT_R8G8B8A8_SNORM,
			VK_FORMAT_B8G8R8A8_SRGB,
			VK_FORMAT_B8G8R8A8_SNORM,
		};

		VulkanDeviceInfo m_DeviceInfo;
		QueueFamilyIndices m_QueueFamilyIndices;

		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
	};
}