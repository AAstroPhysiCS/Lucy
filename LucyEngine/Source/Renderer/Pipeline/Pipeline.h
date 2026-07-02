#pragma once

#include "Renderer/Device/RenderResource.h"

#include "Renderer/Descriptors/DescriptorSet.h"

#include "Renderer/Memory/Buffer/UniformBuffer.h"
#include "Renderer/Memory/Buffer/SharedStorageBuffer.h"
#include "Renderer/Memory/Buffer/PushConstant.h"
#include "Renderer/Renderer.h"

namespace Lucy {

	class Pipeline : public RenderResource {
	public:
		Pipeline(std::string_view name, Ref<Shader> shader)
			: RenderResource(name), m_Shader(shader) {
		}
		virtual ~Pipeline() = default;

		inline const Ref<Shader>& GetShader() const { return m_Shader; }
		PipelineConstant& GetPipelineConstants(const std::string& name);

		uint32_t BindImageHandleTo(const std::string& imageBufferName, const Ref<Image>& image, size_t mip = -1);
		bool HasImageHandleBoundTo(const std::string& imageBufferName);

		template <typename TUniformBuffer = UniformBuffer>
		inline Ref<TUniformBuffer> GetUniformBufferIfExists(const std::string& name) {
			for (auto handle : m_DescriptorSetHandles) {
				Ref<DescriptorSet> set = GetDescriptorSetFromHandle(handle);

				const auto& uniformBuffers = set->GetAllUniformBufferHandles();
				if (!uniformBuffers.contains(name))
					continue;
				return Renderer::AccessResource<UniformBuffer>(uniformBuffers.at(name))->As<TUniformBuffer>();
			}
			LUCY_ASSERT(false, "Could not find a suitable UBO for the given name: {0}", name);
			return nullptr;
		}

		template <typename TSharedStorageBuffer = SharedStorageBuffer>
		inline Ref<TSharedStorageBuffer> GetSharedStorageBufferIfExists(const std::string& name) {
			for (auto handle : m_DescriptorSetHandles) {
				Ref<DescriptorSet> set = GetDescriptorSetFromHandle(handle);

				const auto& ssbos = set->GetAllSharedStorageBufferHandles();
				if (!ssbos.contains(name))
					continue;
				return Renderer::AccessResource<SharedStorageBuffer>(ssbos.at(name))->As<TSharedStorageBuffer>();
			}
			LUCY_ASSERT(false, "Could not find a suitable SSBO for the given name: {0}", name);
			return nullptr;
		}

		inline std::vector<RenderResourceHandle>& GetDescriptorSetHandles() { return m_DescriptorSetHandles; }
	protected:
		virtual void RTLoadDescriptors(const Ref<RenderDevice>& vulkanDevice) = 0;
		virtual void RTDestroyResource() override;

		inline const std::vector<PipelineConstant>& GetPipelineConstants() const { return m_PushConstants; }
		inline Ref<DescriptorSet> GetDescriptorSetFromHandle(RenderResourceHandle handle) const { return Renderer::AccessResource<DescriptorSet>(handle); }

		void AddDescriptorSetHandle(RenderResourceHandle handle);
		void AddPushConstant(const ShaderVariable& pc);
	private:
		std::vector<RenderResourceHandle> m_DescriptorSetHandles;
		std::vector<PipelineConstant> m_PushConstants;

		Ref<Shader> m_Shader;
	};
}