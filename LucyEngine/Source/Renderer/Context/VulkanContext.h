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
		VulkanContext(Ref<Window>& window);
		virtual ~VulkanContext() = default;

		inline VkInstance GetVulkanInstance() { return m_Instance; };
	private:
		void Destroy() final override;
		void PrintInfo() final override;
		void Init() final override;
		
		void CheckValidationSupport();
		void SetupMessageCallback();
		void DestroyMessageCallback();

		std::vector<const char*> m_ValidationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		std::vector<const char*> m_InstanceExtensions = {
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};

		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger{};
		VkDebugReportCallbackEXT m_DebugReport{};
	};
}