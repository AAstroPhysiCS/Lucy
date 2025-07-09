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
	private:
		inline static constexpr uint32_t s_APIVersion = VK_API_VERSION_1_3;
	public:
		VulkanContext(const Ref<Window>& window);
		virtual ~VulkanContext() = default;
		
		void Init() final override;
		void Destroy() final override;
		void PrintInfo() final override;

		inline const std::vector<const char*>& GetValidationLayers() const { return m_ValidationLayers; }

		inline VkInstance GetVulkanInstance() const { return m_Instance; };
		inline static constexpr uint32_t GetAPIVersion() { return s_APIVersion; };
	private:
		void CheckValidationSupport() const;
		void SetupDebugLabels();
		void DestroyDebugCallbacks();

		std::vector<const char*> m_ValidationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		std::vector<const char*> m_InstanceExtensions = {
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		};

		VkInstance m_Instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger{};
	};
}