#pragma once

#include "Renderer/RendererBackend.h"

#include "RenderGraph/RenderGraphPass.h"

#include "Pipeline/PipelineManager.h"
#include "Material/MaterialManager.h"

#include "Device/RenderDeviceResourceManager.h"
#include "Device/RenderDevice.h"
#include "Shader/ShaderManager.h"
#include "Image/Image.h"
#include "Memory/Buffer/IndexBuffer.h"
#include "Renderer/Mesh.h"

namespace Lucy {

	class RenderPipeline;

	class RenderGraphPass;
	class RenderGraphResource;

	class RenderGraph;

	template <typename TRendererPass>
	concept IsRendererPass = requires(TRendererPass&& rendererPass, const Ref<RenderGraph>& renderGraph){
		{ rendererPass.AddPass(renderGraph) };
	};

	struct RenderFrameHandles {
		RenderResourceHandle RenderPassHandle;
		RenderResourceHandle FrameBufferHandle;
	};

	class Renderer final {
	public:
		Renderer() = delete;
		~Renderer() = delete;

#pragma region RenderGraph
		static void ExecuteRenderGraph();
		static void CompileRenderGraph();
		static void Flush();

		static void ImportExternalRenderGraphResource(const RenderGraphResource& renderGraphResource, RenderResourceHandle renderResourceHandle);
		static void ImportExternalRenderGraphTransientResource(const RenderGraphResource& renderGraphResource, RenderResourceHandle renderResourceHandle);
	public:
		template <typename TRendererPass, typename ... TArgs> requires IsRendererPass<TRendererPass>
		static inline void AddRendererPass(TArgs ... args) {
			TRendererPass rendererPass(args...);
			rendererPass.AddPass(s_RenderGraph);
		}

		static Ref<Image> GetOutputOfPass(const char* name);
#pragma endregion RenderGraph

#pragma region RenderDevice
		template <typename TResource> requires IsRenderResource<TResource>
		static inline Ref<TResource> AccessResource(RenderResourceHandle handle) {
			if (handle == InvalidRenderResourceHandle)
				return nullptr;
			auto& device = GetRenderDevice();
			return device->AccessResource<TResource>(handle);
		}

		static void RTDirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size);
		static void SubmitImmediateCommand(std::function<void(VkCommandBuffer)>&& func);

		static void EnqueueToRenderCommandQueue(RenderCommandFunc&& func);
		static void EnqueueResourceDestroy(RenderResourceHandle& handle);
#pragma endregion RenderDevice
		static void InitializeImGui();

		static bool IsValidRenderResource(RenderResourceHandle handle);

		static inline uint32_t GetCurrentImageIndex() { return s_Backend->GetCurrentImageIndex(); }
		static inline uint32_t GetCurrentFrameIndex() { return s_Backend->GetCurrentFrameIndex(); }
		static inline uint32_t GetMaxFramesInFlight() { return s_Backend->GetMaxFramesInFlight(); }

		static inline const RenderCommandQueueMetricsOutput& GetCommandQueueMetrics() { return s_Backend->GetCommandQueueMetrics(); }

		static void ReloadShader(const std::string& name);
		static inline const ShaderLibrary& GetShaderLibrary() { return s_ShaderManager.GetShaderLibrary(); }

		static inline Unique<PipelineManager>& GetPipelineManager() { return s_PipelineManager; }
		static inline Unique<MaterialManager>& GetMaterialManager() { return s_MaterialManager; }

		static inline RenderArchitecture GetRenderArchitecture() { return s_Config.RenderArchitecture; }
		static inline RendererSettings& GetRendererSettings() { return s_Config.Settings; }

		static inline RenderResourceHandle GetBlankCubeImageHandle() { return s_BlankCubeHandle; }
		static inline Ref<Image> GetBlankCubeImage() { return GetRenderDevice()->AccessResource<Image>(s_BlankCubeHandle); }
		static inline Ref<Image> GetBlankArrayImage() { return GetRenderDevice()->AccessResource<Image>(s_BlankArrayHandle); }
		static inline const Ref<Mesh>& GetEnvCubeMesh() { return s_CubeMesh; }
		static inline uint32_t GetEnvCubeMeshIndexCount() { return (uint32_t)GetRenderDevice()->AccessResource<IndexBuffer>(s_CubeMesh->GetIndexBufferHandle())->GetSize(); }

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

		static inline Unique<PipelineManager> s_PipelineManager;
		static inline Unique<MaterialManager> s_MaterialManager;

		static inline RenderResourceHandle s_BlankCubeHandle = InvalidRenderResourceHandle;
		static inline RenderResourceHandle s_BlankArrayHandle = InvalidRenderResourceHandle;
		static inline Ref<Mesh> s_CubeMesh = nullptr;

		friend class Application; //for Init etc.
		friend class RenderGraph; //for CreateImage etc.
	};
}