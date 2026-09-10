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

	namespace VulkanExternalFuncLinkage {
		inline PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT{ nullptr };
		inline PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT{ nullptr };
		inline PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT{ nullptr }; //not used for now
		inline PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT{ nullptr };

		inline PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR{ nullptr };
		inline PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR{ nullptr };
		inline PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR{ nullptr };
		inline PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR{ nullptr };
		inline PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR{ nullptr };

		inline PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR{ nullptr };
		inline PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR{ nullptr };
		inline PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR{ nullptr };
	};

	class VulkanContext : public RenderContext {
	private:
		static inline constexpr uint32_t s_APIVersion = VK_API_VERSION_1_4;
	public:
		VulkanContext(const Ref<Window>& window);
		virtual ~VulkanContext() = default;

		VulkanContext(const VulkanContext&) = delete;
		VulkanContext& operator=(const VulkanContext&) = delete;
		VulkanContext(VulkanContext&&) = delete;
		VulkanContext& operator=(VulkanContext&&) = delete;
		
		void Init() final override;
		void Destroy() final override;
		void PrintInfo() final override;

		inline const std::vector<const char*>& GetValidationLayers() const { return m_ValidationLayers; }

		inline VkInstance GetVulkanInstance() const { return m_Instance; };
		static inline constexpr uint32_t GetAPIVersion() { return s_APIVersion; };
	private:
		void CheckValidationSupport() const;
		void LinkExternalFuncs();
		void DestroyDebugCallbacks();

		std::vector<const char*> m_ValidationLayers = {
			"VK_LAYER_KHRONOS_validation",
		};

		std::vector<const char*> m_InstanceExtensions = {
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		};

		VkInstance m_Instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger{};
	};
}