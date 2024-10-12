#pragma once

#include "RenderContext.h"
#include "VulkanSwapChain.h"

namespace Lucy {

	struct VulkanMessageCallback {
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
															VkDebugUtilsMessageTypeFlagsEXT messageType,
															const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
															void* pUserData);
		static void ImGui_DebugCallback(VkResult result);
	};

	class VulkanExternalFuncLinkage final {
		VulkanExternalFuncLinkage() = delete;
		~VulkanExternalFuncLinkage() = delete;

		inline static PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT{ nullptr };
		inline static PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT{ nullptr };
		inline static PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT{ nullptr }; //not used for now

		friend class VulkanRenderDevice;
		friend class VulkanContext;
	};

	class VulkanContext : public RenderContext {
	public:
		VulkanContext(const Ref<Window>& window);
		virtual ~VulkanContext() = default;

		inline VulkanSwapChain& GetSwapChain() { return m_SwapChain; }
		inline VkInstance GetVulkanInstance() const { return m_Instance; };
	private:
		void Destroy() final override;
		void PrintInfo() final override;
		void Init() final override;
		
		void CheckValidationSupport() const;
		void SetupDebugLabels();
		void DestroyDebugCallbacks();

		std::vector<const char*> m_ValidationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		std::vector<const char*> m_InstanceExtensions = {
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		};

		VulkanSwapChain m_SwapChain;

		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger{};
	};
}