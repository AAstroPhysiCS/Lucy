#include "lypch.h"
#include "RenderContextResultCodes.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	const char* RendererBackendCodesToString(int32_t errorCode) {
		if (Renderer::GetRenderArchitecture() != RenderArchitecture::Vulkan)
			return "Rendering API not supported!";
		switch ((RenderContextResultCodes)errorCode) {
			case RenderContextResultCodes::SUCCESS:
				return "Command successfully completed";
			case RenderContextResultCodes::NOT_READY:
				return "A fence or query has not yet completed";
			case RenderContextResultCodes::TIMEOUT:
				return "A wait operation has not completed in the specified time";
			case RenderContextResultCodes::EVENT_SET:
				return "An event is signaled";
			case RenderContextResultCodes::EVENT_RESET:
				return "An event is unsignaled";
			case RenderContextResultCodes::INCOMPLETE:
				return "A return array was too small for the result";
			case RenderContextResultCodes::ERROR_OUT_OF_HOST_MEMORY:
				return "A host memory allocation has failed. Host out of memory.";
			case RenderContextResultCodes::ERROR_OUT_OF_DEVICE_MEMORY:
				return "A device memory allocation has failed. Device out of memory";
			case RenderContextResultCodes::ERROR_INITIALIZATION_FAILED:
				return "Initialization of an object could not be completed for implementation-specific reasons.";
			case RenderContextResultCodes::ERROR_DEVICE_LOST:
				return "The logical or physical device has been lost.";
			case RenderContextResultCodes::ERROR_MEMORY_MAP_FAILED:
				return "Mapping of a memory object has failed.";
			case RenderContextResultCodes::ERROR_LAYER_NOT_PRESENT:
				return "A requested layer is not present or could not be loaded.";
			case RenderContextResultCodes::ERROR_INCOMPATIBLE_DRIVER:
				return "The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation-specific reasons.";
			case RenderContextResultCodes::ERROR_TOO_MANY_OBJECTS:
				return "Too many objects of the type have already been created.";
			case RenderContextResultCodes::ERROR_FORMAT_NOT_SUPPORTED:
				return "A requested format is not supported on this device.";
			case RenderContextResultCodes::ERROR_SURFACE_LOST_KHR:
				return "A surface is no longer available.";
			case RenderContextResultCodes::SUBOPTIMAL_KHR:
				return "A swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully.";
			case RenderContextResultCodes::ERROR_OUT_OF_DATE_KHR:
				return "A surface has changed in such a way that it is no longer compatible with the swapchain, and further presentation requests using the swapchain will fail. \
					Applications must query the new surface properties and recreate their swapchain if they wish to continue presenting to the surface.";
			case RenderContextResultCodes::ERROR_INCOMPATIBLE_DISPLAY_KHR:
				return "The display used by a swapchain does not use the same presentable image layout, or is incompatible in a way that prevents sharing an image.";
			case RenderContextResultCodes::ERROR_NATIVE_WINDOW_IN_USE_KHR:
				return "The requested window is already in use by Vulkan or another API in a manner which prevents it from being used again.";
            case RenderContextResultCodes::ERROR_VALIDATION_FAILED_EXT:
                return "Validation layer failed.";
            case RenderContextResultCodes::ERROR_INVALID_SHADER_NV:
                return "The provided shader is invalid or incompatible.";
            case RenderContextResultCodes::ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
                return "The image usage is not supported by the platform or device.";
            case RenderContextResultCodes::ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
                return "The video picture layout is not supported.";
            case RenderContextResultCodes::ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
                return "The requested video profile operation is not supported.";
            case RenderContextResultCodes::ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
                return "The requested video profile format is not supported.";
            case RenderContextResultCodes::ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
                return "The requested video profile codec is not supported.";
            case RenderContextResultCodes::ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
                return "The requested video standard version is not supported.";
            case RenderContextResultCodes::ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
                return "The DRM format modifier plane layout is invalid.";
            case RenderContextResultCodes::ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
                return "Full-screen exclusive mode has been lost.";
            case RenderContextResultCodes::THREAD_IDLE_KHR:
                return "The thread is idle.";
            case RenderContextResultCodes::THREAD_DONE_KHR:
                return "The thread has finished its work.";
            case RenderContextResultCodes::OPERATION_DEFERRED_KHR:
                return "The operation has been deferred.";
            case RenderContextResultCodes::OPERATION_NOT_DEFERRED_KHR:
                return "The operation was not deferred.";
            case RenderContextResultCodes::ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
                return "Invalid video standard parameters.";
            case RenderContextResultCodes::ERROR_COMPRESSION_EXHAUSTED_EXT:
                return "Compression resources have been exhausted.";
            case RenderContextResultCodes::INCOMPATIBLE_SHADER_BINARY_EXT:
                return "The shader binary is incompatible.";
            case RenderContextResultCodes::PIPELINE_BINARY_MISSING_KHR:
                return "A required pipeline binary is missing.";
            case RenderContextResultCodes::ERROR_NOT_ENOUGH_SPACE_KHR:
                return "There is not enough space available for the operation.";
            case RenderContextResultCodes::ERROR_OUT_OF_POOL_MEMORY:
                return "Out of pool memory.";
            case RenderContextResultCodes::ERROR_INVALID_EXTERNAL_HANDLE:
                return "Invalid external handle.";
            case RenderContextResultCodes::ERROR_FRAGMENTATION:
                return "The operation could not be completed due to memory fragmentation.";
            case RenderContextResultCodes::ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
                return "The opaque capture address is invalid.";
            case RenderContextResultCodes::PIPELINE_COMPILE_REQUIRED:
                return "Pipeline compilation is required.";
            case RenderContextResultCodes::RESULT_MAX_ENUM:
                return "Maximum enum value reached.";
			default:
				return "Validation failed!";
		}
	}
}