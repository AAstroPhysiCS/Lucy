#pragma once

#include "RenderContext.h"

#include "vulkan/vulkan.h"

namespace Lucy {

	struct VulkanMessageCallback {
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
															VkDebugUtilsMessageTypeFlagsEXT messageType,
															const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
															void* pUserData);
		static void ImGui_DebugCallback(VkResult result);
	};

	class VulkanContext : public RenderContext {
	public:
		VulkanContext();
		virtual ~VulkanContext() = default;

		void Destroy() override;
		void PrintInfo() override;
		inline VkInstance GetVulkanInstance() { return m_Instance; };
	private:
		void Init() override;
		void CheckValidationSupport();
		void SetupMessageCallback();
		void DestroyMessageCallback();

		std::vector<const char*> m_ValidationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		std::vector<const char*> m_InstanceExtensions = {
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
			VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
		};

		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger{};
		VkDebugReportCallbackEXT m_DebugReport{};
	};
}