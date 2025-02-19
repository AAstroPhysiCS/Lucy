#pragma once

#include "Renderer/RendererBackend.h"

#include "Pipeline/PipelineManager.h"
#include "Material/MaterialManager.h"

#include "Image/Image.h"
#include "Memory/Buffer/IndexBuffer.h"
#include "Renderer/Mesh.h"

#include "RenderGraph/RenderGraph.h"

namespace Lucy {

	class RenderPipeline;

	class RenderGraphPass;
	class RenderGraphResource;

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

		static void DirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size);
		static void SubmitImmediateCommand(std::function<void(VkCommandBuffer)>&& func);

		static void EnqueueToRenderThread(RenderCommandFunc&& func);
		static void EnqueueResourceDestroy(RenderResourceHandle& handle);
#pragma endregion RenderDevice
		static void InitializeImGui();
		static void RenderImGui();

		static bool IsValidRenderResource(RenderResourceHandle handle);

		static inline uint32_t GetCurrentImageIndex() { return s_Renderer->GetCurrentImageIndex(); }
		static inline uint32_t GetCurrentFrameIndex() { return s_Renderer->GetCurrentFrameIndex(); }
		static inline uint32_t GetMaxFramesInFlight() { return s_Renderer->GetMaxFramesInFlight(); }

		static inline const CommandQueueMetricsOutput& GetCommandQueueMetrics() { return s_Renderer->GetCommandQueueMetrics(); }

		static void RTReloadShader(const std::string& name);
		static inline const Ref<Shader>& GetShader(const std::string& name) { return s_Shaders[name]; }
		static inline const std::unordered_map<std::string, Ref<Shader>>& GetAllShaders() { return s_Shaders; }

		static inline Unique<PipelineManager>& GetPipelineManager() { return s_PipelineManager; }
		static inline Unique<MaterialManager>& GetMaterialManager() { return s_MaterialManager; }

		static inline RenderArchitecture GetRenderArchitecture() { return s_Config.RenderArchitecture; }

		static inline RenderResourceHandle GetBlankCubeImageHandle() { return s_BlankCubeHandle; }
		static inline Ref<Image> GetBlankCubeImage() { return GetRenderDevice()->AccessResource<Image>(s_BlankCubeHandle); }
		static inline const Ref<Mesh>& GetEnvCubeMesh() { return s_CubeMesh; }
		static inline uint32_t GetEnvCubeMeshIndexCount() { return (uint32_t)GetRenderDevice()->AccessResource<IndexBuffer>(s_CubeMesh->GetIndexBufferHandle())->GetSize(); }

		static RenderContextResultCodes WaitAndPresent();

		static void WaitForDevice();
		static bool IsOnRenderThread();

		static void OnEvent(Event& evt);
	private:
		static inline const Ref<RenderContext>& GetRenderContext() { return s_Renderer->GetRenderContext(); }
		static inline const Ref<RenderDevice>& GetRenderDevice() { return GetRenderContext()->GetRenderDevice(); }

		static void Init(RendererConfiguration config, const Ref<Window>& window);
		static void Destroy();

		static void SubmitToRender(RenderGraphPass& pass);

		static void OnWindowResize();
		static void OnViewportResize();
		static glm::vec3 OnMousePicking(const EntityPickedEvent& e);

		static void PushShader(Ref<Shader> shader);
		static void DestroyAllShaders();

		static inline Ref<RenderGraph> s_RenderGraph = nullptr;

		static inline std::unordered_map<std::string, RenderFrameHandles> s_RenderFrameHandleMap;

		static inline Ref<RendererBackend> s_Renderer = nullptr;
		static inline RendererConfiguration s_Config;
		
		static inline Unique<PipelineManager> s_PipelineManager;
		static inline Unique<MaterialManager> s_MaterialManager;

		static inline std::unordered_map<std::string, Ref<Shader>> s_Shaders;

		static inline RenderResourceHandle s_BlankCubeHandle = InvalidRenderResourceHandle;
		static inline Ref<Mesh> s_CubeMesh = nullptr;

		friend class Application; //for Init etc.
		friend class RenderGraph; //for CreateImage etc.

		friend class CustomShaderIncluder; //for ShaderIncluder
	};
}