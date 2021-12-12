#pragma once

#include "RenderContext.h"
#include "vulkan/vulkan.h"

namespace Lucy {

	struct VulkanMessageCallback {
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
															VkDebugUtilsMessageTypeFlagsEXT messageType,
															const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
															void* pUserData);
	};

	class VulkanContext : public RenderContext {
	public:
		VulkanContext(RenderArchitecture type);
		virtual ~VulkanContext() = default;

		void Destroy();
		void PrintInfo();
	private:
		void Init(RenderArchitecture type);
		void CheckValidationSupport();
		void SetupMessageCallback();
		void DestroyMessageCallback();

		const std::vector<const char*> m_ValidationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger{};
	};
}
