#pragma once

#include "RendererBackend.h"

#include "Shader/ShaderManager.h"

#include "RendererConfiguration.h"

#include "Pipeline/PipelineManager.h"
#include "Material/MaterialManager.h"

#include "Device/RenderDevice.h"
#include "Device/RenderDeviceHandles.h"

#include "RenderGraph/RenderGraphRegistry.h"

namespace Lucy {

	class MaterialManager;

	class RenderThread;
	class RenderPipeline;

	class Mesh;

	class RenderGraphPass;
	class RenderGraphResource;
	class RenderGraph;

	template <typename TRendererPass>
	concept IsRendererPass = requires(TRendererPass&& rendererPass, const Ref<RenderGraph>& renderGraph){
		{ rendererPass.AddPass(renderGraph) };
	};

	struct RenderFrameHandles {
		RenderDeviceResourceHandle RenderPassHandle{};
		RenderDeviceResourceHandle FrameBufferHandle{};
	};

	class Renderer final {
	private:
		Renderer() = delete;
		~Renderer() = delete;

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer(Renderer&&) = delete;
		Renderer& operator=(Renderer&&) = delete;
	public:
#pragma region RenderGraph
		static void ExecuteRenderGraph();
		static void CompileRenderGraph();
		static void Flush();

		static void ImportExternalRenderGraphResource(const RenderGraphResource& renderGraphResource, RenderDeviceResourceHandle renderResourceHandle, RGResourceData data = {});
		static void ImportExternalRenderGraphResource(const RenderGraphResource& renderGraphResource, 
			const std::vector<RenderDeviceResourceHandle>& renderResourceHandles, RGResourceData data = {});
		static void ImportExternalRenderGraphTransientResource(const RenderGraphResource& renderGraphResource, RenderDeviceResourceHandle renderResourceHandle);
	public:
		template <typename TRendererPass, typename ... TArgs> requires IsRendererPass<TRendererPass>
		static inline void AddRendererPass(TArgs ... args) {
			TRendererPass rendererPass(args...);
			rendererPass.AddPass(s_RenderGraph);
		}

		static Ref<Image> GetFrameBufferOutputOfPass(const char* name);
#pragma endregion RenderGraph

#pragma region RenderDevice
		template <typename TResource> requires IsRenderResource<TResource>
		static inline Ref<TResource> AccessResource(RenderDeviceResourceHandle handle) {
			if (!handle)
				return nullptr;
			auto& device = GetRenderDevice();
			return device->AccessResource<TResource>(handle);
		}

		static void SubmitImmediateCommand(std::function<void(VkCommandBuffer)>&& func);

		static void EnqueueToRenderCommandQueue(RenderCommandFunc&& func);
		static void EnqueueResourceDestroy(RenderDeviceResourceHandle& handle);
		static void EnqueueResourceDestroy(RenderDeletionFunc&& func);
		static void EnqueueResourceRecreate(RenderRecreateFunc&& func);
#pragma endregion RenderDevice
		static void InitializeImGui();

		static bool IsValidRenderResource(RenderDeviceResourceHandle handle);

		static inline uint32_t GetCurrentImageIndex() { return s_Backend->GetCurrentImageIndex(); }
		static inline uint32_t GetCurrentFrameIndex() { return s_Backend->GetCurrentFrameIndex(); }
		static inline uint32_t GetMaxFramesInFlight() { return s_Backend->GetMaxFramesInFlight(); }

		static inline const RenderCommandQueueMetricsOutput& GetCommandQueueMetrics() { return s_Backend->GetCommandQueueMetrics(); }

		static void ReloadShader(const std::string& name);
		static inline const ShaderLibrary& GetShaderLibrary() { return s_ShaderManager.GetShaderLibrary(); }

		static inline Unique<PipelineManager>& GetPipelineManager() { return s_PipelineManager; }
		static Unique<MaterialManager>& GetMaterialManager();

		static inline RenderArchitecture GetRenderArchitecture() { return s_Config.RenderArchitecture; }
		static inline RendererSettings& GetRendererSettings() { return s_Config.Settings; }

		static inline RenderDeviceResourceHandle GetBlankCubeImageHandle() { return s_BlankCubeHandle; }
		static Ref<Image> GetBlankCubeImage();
		static Ref<Image> GetBlankArrayImage();

		static const Unique<Mesh>& GetEnvCubeMesh() { return s_CubeMesh; }
		static uint32_t GetEnvCubeMeshIndexCount();

		static RenderContextResultCodes WaitAndPresent();

		static void WaitForDevice();
		static bool IsOnRenderThread();

		static void RTSetBackend(Ref<RendererBackend> backend);

		static void OnEvent(Event& evt);
	private:
		static inline const Ref<RenderContext>& GetRenderContext() { return s_Backend->GetRenderContext(); }
		static inline const Ref<RenderDevice>& GetRenderDevice() { return s_Backend->GetRenderDevice(); }

		static void Init(RendererConfiguration config, const Ref<Window>& window);
		static void Destroy();

		static void SubmitToRender(std::vector<ExecutionBatch>& batches);

		static void OnWindowResize();
		static void OnViewportResize();
		static glm::vec3 OnMousePicking(const EntityPickedEvent& e);

		static void DestroyAllShaders();

		static inline RendererConfiguration s_Config;
		static inline RenderThread* s_RenderThread = nullptr;
		static inline Ref<RendererBackend> s_Backend = nullptr;
		static inline Ref<RenderGraph> s_RenderGraph = nullptr;

		static inline ShaderManager s_ShaderManager;

		static inline std::unordered_map<std::string, RenderFrameHandles> s_RenderFrameHandleMap;

		static inline Unique<PipelineManager> s_PipelineManager = nullptr;
		static inline Unique<MaterialManager> s_MaterialManager = nullptr;

		static inline RenderDeviceResourceHandle s_BlankCubeHandle{};
		static inline RenderDeviceResourceHandle s_BlankArrayHandle{};
		static inline Unique<Mesh> s_CubeMesh = nullptr;

		friend class Application; //for Init etc.
		friend class MaterialManager; //for creating materials TODO: change this
	};
}