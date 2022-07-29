#pragma once

#include "Context/RenderContext.h"
#include "Device/RenderDevice.h"

namespace Lucy {

	class Window;
	class PushConstant;
	class DescriptorSet;

	class Scene;
	class Entity;

	//Vulkan error codes
	//Other API's will adapt it
	enum class PresentResult {
		SUCCESS = 0,
		NOT_READY = 1,
		TIMEOUT = 2,
		EVENT_SET = 3,
		EVENT_RESET = 4,
		INCOMPLETE = 5,
		ERROR_OUT_OF_HOST_MEMORY = -1,
		ERROR_OUT_OF_DEVICE_MEMORY = -2,
		ERROR_INITIALIZATION_FAILED = -3,
		ERROR_DEVICE_LOST = -4,
		ERROR_MEMORY_MAP_FAILED = -5,
		ERROR_LAYER_NOT_PRESENT = -6,
		ERROR_EXTENSION_NOT_PRESENT = -7,
		ERROR_FEATURE_NOT_PRESENT = -8,
		ERROR_INCOMPATIBLE_DRIVER = -9,
		ERROR_TOO_MANY_OBJECTS = -10,
		ERROR_FORMAT_NOT_SUPPORTED = -11,
		ERROR_SURFACE_LOST_KHR = -1000000000,
		SUBOPTIMAL_KHR = 1000001003,
		ERROR_OUT_OF_DATE_KHR = -1000001004,
		ERROR_INCOMPATIBLE_DISPLAY_KHR = -1000003001,
		ERROR_NATIVE_WINDOW_IN_USE_KHR = -1000000001,
		ERROR_VALIDATION_FAILED_EXT = -1000011001,
	};

	class RendererBase {
	public:
		static Ref<RendererBase> Create(RenderArchitecture arch, Ref<Window>& window);
		virtual ~RendererBase() = default;
		
		void Init();

		virtual void BeginScene(Ref<Scene>& scene) = 0;
		virtual void RenderScene() = 0;
		virtual PresentResult EndScene() = 0;
		virtual void WaitForDevice() = 0;

		virtual void Destroy(); //leaving it to the child

		inline const Ref<RenderDevice>& GetRenderDevice() const { return m_RenderDevice; }
		inline const Ref<RenderContext>& GetRenderContext() const { return m_RenderContext; }

		virtual void OnWindowResize() = 0;
		virtual void OnViewportResize() = 0;
		virtual Entity OnMousePicking(Ref<Scene>& scene, const Ref<Pipeline>& idPipeline) = 0;
	protected:
		RendererBase(RenderArchitecture arch, Ref<Window>& window);

		Ref<RenderContext> m_RenderContext = nullptr;
		Ref<RenderDevice> m_RenderDevice = nullptr;
	};
}