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

		uint32_t version;
		vkEnumerateInstanceVersion(&version);

		uint32_t major = VK_VERSION_MAJOR(version);
		uint32_t minor = VK_VERSION_MINOR(version);
		uint32_t patch = VK_VERSION_PATCH(version);

		LUCY_INFO("Vulkan successfully initialized: {0}.{1}.{2}", major, minor, patch);

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
		VulkanExternalFuncLinkage::vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)(vkGetInstanceProcAddr(m_Instance, "vkSetDebugUtilsObjectNameEXT"));
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

		const auto SeverityToString = [](VkDebugUtilsMessageSeverityFlagBitsEXT severity) -> std::string_view {
			if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)   return "Error";
			if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) return "Warning";
			if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)    return "Info";
			if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) return "Verbose";
			return "Unknown";
		};

		const auto ObjectTypeToString = [](VkObjectType type) -> const char* {
			switch (type) {
				case VK_OBJECT_TYPE_BUFFER: return "Buffer";
				case VK_OBJECT_TYPE_IMAGE: return "Image";
				case VK_OBJECT_TYPE_IMAGE_VIEW: return "ImageView";
				case VK_OBJECT_TYPE_SAMPLER: return "Sampler";
				case VK_OBJECT_TYPE_DESCRIPTOR_SET: return "DescriptorSet";
				case VK_OBJECT_TYPE_PIPELINE: return "Pipeline";
				case VK_OBJECT_TYPE_RENDER_PASS: return "RenderPass";
				case VK_OBJECT_TYPE_FRAMEBUFFER: return "Framebuffer";
				case VK_OBJECT_TYPE_COMMAND_BUFFER: return "CommandBuffer";
				case VK_OBJECT_TYPE_COMMAND_POOL: return "CommandPool";
				case VK_OBJECT_TYPE_QUEUE: return "Queue";
				case VK_OBJECT_TYPE_DEVICE: return "Device";
				case VK_OBJECT_TYPE_INSTANCE: return "Instance";
				case VK_OBJECT_TYPE_SWAPCHAIN_KHR: return "SwapchainKHR";
				default: return "Unknown";
			}
		};

		const auto MessageTypeToString = [](VkDebugUtilsMessageTypeFlagsEXT type) -> std::string {
			std::string result = "Unknown";

			if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
				result += "General|";
			if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
				result += "Validation|";
			if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
				result += "Performance|";

#ifdef VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT
			if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT)
				result += "DeviceAddressBinding|";
#endif

			if (!result.empty())
				result.pop_back();

			return result;
		};

		std::string message;
		message += std::format("[Vulkan {0}] [{1}]\nMessage: {2}\n", SeverityToString(messageSeverity), MessageTypeToString(messageType), pCallbackData ? pCallbackData->pMessage : "<null>");
		
		if (!pCallbackData)
			return VK_FALSE;

		message += "\tQueue Labels:\n";
		for (uint32_t i = 0; i < pCallbackData->queueLabelCount; ++i) {
			const VkDebugUtilsLabelEXT& label = pCallbackData->pQueueLabels[i];
			message += std::format("\t\t  - {}\n", label.pLabelName);
		}

		message += "\tCommand Buffer Labels:\n";
		for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; ++i) {
			const VkDebugUtilsLabelEXT& label = pCallbackData->pCmdBufLabels[i];
			message += std::format("\t\t  - {}\n", label.pLabelName);
		}

		message += "\tObjects:\n";
		for (uint32_t i = 0; i < pCallbackData->objectCount; ++i) {
			const VkDebugUtilsObjectNameInfoEXT& object = pCallbackData->pObjects[i];
			message += std::format("\t\t  - {} | Handle: {} | Name: {}\n", ObjectTypeToString(object.objectType), static_cast<uint64_t>(object.objectHandle), object.pObjectName == nullptr ? "<null>" : object.pObjectName);
		}

		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			LUCY_CRITICAL(message);
		} else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			LUCY_WARN(message);
		} else {
			LUCY_INFO(message);
		}

		return VK_FALSE;
	}

	void VulkanMessageCallback::ImGui_DebugCallback(VkResult result) {
		if (result != VK_SUCCESS) 
			LUCY_CRITICAL(std::format("Vulkan ImGui error {0}\n", RendererBackendCodesToString(result)));
	}
}