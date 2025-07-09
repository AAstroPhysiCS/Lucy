#pragma once
#include "shaderc/shaderc.hpp"

#include "ShaderReflect.h"
#include "Renderer/Descriptors/DescriptorSet.h"

#include "Renderer/Memory/Buffer/UniformBuffer.h"
#include "Renderer/Memory/Buffer/SharedStorageBuffer.h"
#include "Renderer/Memory/Buffer/PushConstant.h"
#include "Renderer/Renderer.h"

namespace Lucy {

	class VulkanDescriptorPool;

	class CustomShaderIncluder final : public shaderc::CompileOptions::IncluderInterface {
	public:
		CustomShaderIncluder(Ref<RenderDevice> renderDevice);
		virtual ~CustomShaderIncluder() = default;

		// Handles shaderc_include_resolver_fn callbacks.
		shaderc_include_result* GetInclude(const char* requested_source,
			shaderc_include_type type,
			const char* requesting_source,
			size_t include_depth) final override;

		// Handles shaderc_include_result_release_fn callbacks.
		void ReleaseInclude(shaderc_include_result* data) final override;
	private:
		Ref<RenderDevice> m_RenderDevice = nullptr;

		std::string* m_DataBuffer = nullptr;
	};

	class Shader : public MemoryTrackable {
	private:
		inline static std::filesystem::path s_ShaderFolder = "Assets/Shaders/";
		inline static std::filesystem::path s_CacheFolder = "Assets/Shaders/Cached/";
	public:
		static Ref<Shader> Create(const std::string& name, const std::filesystem::path& path, Ref<RenderDevice> device);

		Shader(const std::string& name, const std::filesystem::path& path);
		virtual ~Shader() = default;

		inline static std::filesystem::path& GetShaderFolder() { return s_ShaderFolder; }
		inline static std::filesystem::path& GetCacheFolder() { return s_CacheFolder; }

		inline std::filesystem::path GetPath() const { return m_Path; }
		inline const std::string& GetName() const { return m_Name; }

		inline const std::vector<RenderResourceHandle>& GetDescriptorSetHandles() const { return m_DescriptorSetHandles; }
		inline const std::vector<VulkanPushConstant>& GetPushConstants() const { return m_PushConstants; }
		inline const VertexShaderLayout& GetVertexShaderLayout() const { return m_Reflect.GetVertexShaderLayout(); }

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
#pragma region VulkanInternals
		VulkanPushConstant& GetPushConstants(const std::string& name);
#pragma endregion VulkanInternals

		virtual void RTLoad(const Ref<RenderDevice>& device, bool forceReloadFromDisk = false) = 0;
		virtual void RTDestroyResource(const Ref<RenderDevice>& device);

		uint32_t BindImageHandleTo(const std::string& imageBufferName, const Ref<Image>& image);
		bool HasImageHandleBoundTo(const std::string& imageBufferName) const;
		void RTLoadDescriptors(const Ref<RenderDevice>& device, const Ref<VulkanDescriptorPool>& descriptorPool);
	protected:
		void RunReflect(const std::vector<uint32_t>& data, int32_t flags = 0);
		std::vector<uint32_t> LoadSPIRVData(const std::filesystem::path& path, shaderc::Compiler& compiler, shaderc::CompileOptions& options,
						   shaderc_shader_kind kind);
		std::vector<uint32_t> LoadSPIRVDataFromCache(const std::filesystem::path& cachedFilePath);
	private:
		inline Ref<DescriptorSet> GetDescriptorSetFromHandle(RenderResourceHandle handle) const {
			return Renderer::AccessResource<DescriptorSet>(handle);
		}

		std::filesystem::path m_Path = "";
		std::string m_Name = "Unnamed";
		ShaderReflect m_Reflect;

		std::vector<RenderResourceHandle> m_DescriptorSetHandles;
#pragma region VulkanInternals
		std::vector<VulkanPushConstant> m_PushConstants;
#pragma endregion VulkanInternals
	};
}