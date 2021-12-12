#include "lypch.h"
#include "VulkanContext.h"

#include "GLFW/glfw3.h"

namespace Lucy {

	VulkanContext::VulkanContext(RenderArchitecture type)
		: RenderContext(type) {
		Init(type);
	}

	void VulkanContext::Destroy() {
		DestroyMessageCallback();
		vkDestroyInstance(m_Instance, nullptr);
		glfwTerminate();
	}

	void VulkanContext::PrintInfo() {
		Logger::Log(LoggerInfo::LUCY_INFO, "Vulkan version 1.2");
	}

	void VulkanContext::Init(RenderArchitecture type) {
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "LucyEngine x64 Vulkan";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "LucyEngine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensionNames = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

#ifdef LUCY_DEBUG
		CheckValidationSupport();
		createInfo.enabledLayerCount = m_ValidationLayers.size();
		createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

		std::vector<const char*> extensions(glfwExtensionNames, glfwExtensionNames + glfwExtensionCount);
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		createInfo.enabledExtensionCount = extensions.size();
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugForVkInstanceAndDestroy{};
		debugForVkInstanceAndDestroy.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugForVkInstanceAndDestroy.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugForVkInstanceAndDestroy.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugForVkInstanceAndDestroy.pfnUserCallback = VulkanMessageCallback::DebugCallback;
		createInfo.pNext = &debugForVkInstanceAndDestroy;
#else
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
		createInfo.enabledExtensionCount = 0;
		createInfo.ppEnabledExtensionNames = nullptr;
#endif
		
		VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
		LUCY_ASSERT_VULKAN(result);
		LUCY_INFO("Vulkan successfully initialized");

		SetupMessageCallback();
	}

	void VulkanContext::CheckValidationSupport() {
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
				LUCY_ASSERT(false);
		}
	}

	void VulkanContext::SetupMessageCallback() {
		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = VulkanMessageCallback::DebugCallback;
		createInfo.pUserData = nullptr; // Optional

		auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
		if (func) {
			LUCY_ASSERT_VULKAN(func(m_Instance, &createInfo, nullptr, &m_DebugMessenger));
			return;
		}
		LUCY_ASSERT(false);
	}

	void VulkanContext::DestroyMessageCallback() {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func) {
			func(m_Instance, m_DebugMessenger, nullptr);
		}
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanMessageCallback::DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {

		if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			LUCY_WARN(fmt::format("Vulkan validation warning {0}", pCallbackData->pMessage));
		} else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			LUCY_CRITICAL(fmt::format("Vulkan validation error {0}", pCallbackData->pMessage));
		}

		return VK_FALSE;
	}
}