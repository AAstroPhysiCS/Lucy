#include "lypch.h"
#include "VulkanContext.h"

#include "Renderer/Context/VulkanSwapChain.h"

#include "Renderer/Device/VulkanRenderDevice.h"

namespace Lucy {
	
	VulkanContext::VulkanContext(const Ref<Window>& window)
		: RenderContext(window) {
	}

	void VulkanContext::Destroy() {
		const auto& window = GetWindow();
		window->DestroyVulkanSurface(m_Instance);

		DestroyDebugCallbacks();
		vkDestroyInstance(m_Instance, nullptr);
	}

	void VulkanContext::PrintInfo() {
		LUCY_INFO("----------Enabled extensions----------");
		for (uint32_t i = 0; i < m_InstanceExtensions.size(); i++) {
			const char* extension = m_InstanceExtensions[i];
			LUCY_INFO(std::format("{0}: {1}", i, extension));
		}
	}

	void VulkanContext::Init() {
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "LucyEngine x64 Vulkan";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "LucyEngine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = s_APIVersion;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

#if LUCY_DEBUG
		VkValidationFeaturesEXT validationFeatures{};
		validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
		validationFeatures.enabledValidationFeatureCount = 4;
		VkValidationFeatureEnableEXT enables[] = { VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT, 
			VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT, VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT, 
			VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT };
		validationFeatures.pEnabledValidationFeatures = enables;

		createInfo.pNext = &validationFeatures;
#endif

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensionNames = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> instanceExtensions(glfwExtensionNames, glfwExtensionNames + glfwExtensionCount);
#ifdef LUCY_DEBUG
		CheckValidationSupport();
		createInfo.enabledLayerCount = (uint32_t)m_ValidationLayers.size();
		createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

		VkDebugUtilsMessengerCreateInfoEXT debugForVkInstanceAndDestroy{};
		debugForVkInstanceAndDestroy.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugForVkInstanceAndDestroy.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
		debugForVkInstanceAndDestroy.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugForVkInstanceAndDestroy.pfnUserCallback = VulkanMessageCallback::DebugCallback;
		debugForVkInstanceAndDestroy.pUserData = nullptr; // Optional

		createInfo.pNext = &debugForVkInstanceAndDestroy;

		instanceExtensions.insert(instanceExtensions.end(), m_InstanceExtensions.begin(), m_InstanceExtensions.end());
#else
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
#endif
		createInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		createInfo.ppEnabledExtensionNames = instanceExtensions.data();

		LUCY_VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &m_Instance));
		LUCY_INFO("Vulkan successfully initialized");

#ifdef LUCY_DEBUG
		if (auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"))
			LUCY_VK_ASSERT(func(m_Instance, &debugForVkInstanceAndDestroy, nullptr, &m_DebugMessenger));

		SetupDebugLabels();
#endif
		const Ref<Window>& window = GetWindow();
		window->InitVulkanSurface(m_Instance);
	}

	void VulkanContext::CheckValidationSupport() const {
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const VkLayerProperties& prop : availableLayers) {
			bool notFound = false;
			for (const char* names : m_ValidationLayers) {
				if (prop.layerName != names) {
					notFound = true;
					break;
				}
			}
			if (!notFound)
				LUCY_ASSERT(false, "Vulkan validation support isn't being supported!");
		}
	}

	void VulkanContext::SetupDebugLabels() {
		VulkanExternalFuncLinkage::vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)(vkGetInstanceProcAddr(m_Instance, "vkCmdBeginDebugUtilsLabelEXT"));
		VulkanExternalFuncLinkage::vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)(vkGetInstanceProcAddr(m_Instance, "vkCmdEndDebugUtilsLabelEXT"));
		VulkanExternalFuncLinkage::vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)(vkGetInstanceProcAddr(m_Instance, "vkCmdInsertDebugUtilsLabelEXT"));
	}

	void VulkanContext::DestroyDebugCallbacks() {
		if (auto destroyDebugUtils = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"))
			destroyDebugUtils(m_Instance, m_DebugMessenger, nullptr);
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanMessageCallback::DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {

		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			LUCY_WARN(std::format("Vulkan validation warning {0}", pCallbackData->pMessage));
		} else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			LUCY_CRITICAL(std::format("Vulkan validation error {0}\n", pCallbackData->pMessage));
		} else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
			LUCY_INFO(std::format("Vulkan validation verbose {0}", pCallbackData->pMessage));
		} else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
			LUCY_INFO(std::format("Vulkan validation info {0}", pCallbackData->pMessage));
		} else {
			LUCY_INFO(std::format("Unknown vulkan validation {0}", pCallbackData->pMessage));
		}

		return VK_FALSE;
	}

	void VulkanMessageCallback::ImGui_DebugCallback(VkResult result) {
		if (result != VK_SUCCESS) 
			LUCY_CRITICAL(std::format("Vulkan ImGui error {0}\n", RendererBackendCodesToString(result)));
	}
}