#include "lypch.h"
#include "VulkanContext.h"

#include "Renderer/Memory/VulkanAllocator.h"
#include "Renderer/Context/VulkanContextDevice.h"
#include "Renderer/Context/VulkanSwapChain.h"

namespace Lucy {
	
	VulkanContext::VulkanContext(Ref<Window>& window)
		: RenderContext(window) {
		Init();
	}

	void VulkanContext::Destroy() {
		VulkanAllocator::Get().Destroy();
		VulkanSwapChain::Get().Destroy();
		VulkanContextDevice::Get().Destroy();
		m_Window->DestroyVulkanSurface(m_Instance);

		DestroyMessageCallback();
		vkDestroyInstance(m_Instance, nullptr);
	}

	void VulkanContext::PrintInfo() {
		LUCY_INFO("----------Enabled extensions----------");
		for (uint32_t i = 0; i < m_InstanceExtensions.size(); i++) {
			const char* extension = m_InstanceExtensions[i];
			LUCY_INFO(fmt::format("{0}: {1}", i, extension));
		}
	}

	void VulkanContext::Init() {
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "LucyEngine x64 Vulkan";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "LucyEngine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensionNames = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> instanceExtensions(glfwExtensionNames, glfwExtensionNames + glfwExtensionCount);
#ifdef LUCY_DEBUG
		CheckValidationSupport();
		createInfo.enabledLayerCount = m_ValidationLayers.size();
		createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

		VkDebugUtilsMessengerCreateInfoEXT debugForVkInstanceAndDestroy{};
		debugForVkInstanceAndDestroy.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugForVkInstanceAndDestroy.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugForVkInstanceAndDestroy.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugForVkInstanceAndDestroy.pfnUserCallback = VulkanMessageCallback::DebugCallback;
		createInfo.pNext = &debugForVkInstanceAndDestroy;

		instanceExtensions.insert(instanceExtensions.end(), m_InstanceExtensions.begin(), m_InstanceExtensions.end());
#else
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
#endif
		createInfo.enabledExtensionCount = instanceExtensions.size();
		createInfo.ppEnabledExtensionNames = instanceExtensions.data();

		LUCY_VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &m_Instance));

#ifdef LUCY_DEBUG
		LUCY_INFO("Vulkan successfully initialized");

		SetupMessageCallback();
#endif
		m_Window->InitVulkanSurface(m_Instance);
		VulkanContextDevice::Get().Create(m_Instance, m_ValidationLayers, m_Window->GetVulkanSurface());
		VulkanSwapChain::Get().Create(m_Window);
		VulkanAllocator::Get().Init(m_Instance);
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

		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
		if (func) {
			LUCY_VK_ASSERT(func(m_Instance, &createInfo, nullptr, &m_DebugMessenger));
			return;
		}
		LUCY_ASSERT(false);
	}

	void VulkanContext::DestroyMessageCallback() {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func) {
			func(m_Instance, m_DebugMessenger, nullptr);
		}
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanMessageCallback::DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {

		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			LUCY_WARN(fmt::format("Vulkan validation warning {0}", pCallbackData->pMessage));
		} else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			LUCY_CRITICAL(fmt::format("Vulkan validation error {0}", pCallbackData->pMessage));
		}

		return VK_FALSE;
	}

	void VulkanMessageCallback::ImGui_DebugCallback(VkResult result) {
		if (result != VK_SUCCESS) 
			LUCY_CRITICAL(fmt::format("Vulkan ImGui error {0}", RendererAPICodesToString(result)));
	}
}