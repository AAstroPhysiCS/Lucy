#include "lypch.h"
#include "ShaderReflect.h"
#include "Shader.h"

#include "Renderer/Renderer.h"
#include "Core/FileSystem.h"

namespace Lucy {
	
	//TODO: Dynamic UBO and SSBO won't work, yet!
	void ShaderReflect::Info(const std::filesystem::path& path, const std::vector<uint32_t>& data, VkShaderStageFlags stageFlag) {
		//heap allocating it because of the compiler warning/stack size: "function uses X bytes of stack consider moving some data to heap"
		spirv_cross::CompilerGLSL* compiler = new spirv_cross::CompilerGLSL(data);

		spirv_cross::CompilerGLSL::Options options;
		if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan)
			options.vulkan_semantics = true; //default is false
		compiler->set_common_options(options);

		const spirv_cross::ShaderResources& resourcesShaderStage = compiler->get_shader_resources();

		auto Reflect = [this, path](spirv_cross::CompilerGLSL* compiler, spirv_cross::ShaderResources resource,
									ShaderStageInfo& stageInfo, VkShaderStageFlags stageFlag) {
			stageInfo.UniformCount = resource.uniform_buffers.size();
			stageInfo.SampledImagesCount = resource.sampled_images.size();
			stageInfo.StorageImageCount = resource.storage_images.size();
			stageInfo.PushConstantBufferCount = resource.push_constant_buffers.size();
			stageInfo.StageInputCount = resource.stage_inputs.size();
			stageInfo.StageOutputCount = resource.stage_outputs.size();
			stageInfo.StorageBufferCount = resource.storage_buffers.size();

			std::string shaderType = "Unknown";
			switch (stageFlag) {
				case VK_SHADER_STAGE_VERTEX_BIT:
					shaderType = "Vertex";
					break;
				case VK_SHADER_STAGE_FRAGMENT_BIT:
					shaderType = "Fragment";
					break;
				case VK_SHADER_STAGE_COMPUTE_BIT:
					shaderType = "Compute";
					break;
			}

			LUCY_INFO(std::format("------{0} Shader {1}------", shaderType, path.string()));
			LUCY_INFO(std::format("{0} uniform buffers", stageInfo.UniformCount));
			LUCY_INFO(std::format("{0} sampled images", stageInfo.SampledImagesCount));
			LUCY_INFO(std::format("{0} storage images", stageInfo.StorageImageCount));
			LUCY_INFO(std::format("{0} storage buffers", stageInfo.StorageBufferCount));
			LUCY_INFO(std::format("{0} push constant buffers", stageInfo.PushConstantBufferCount));
			LUCY_INFO(std::format("{0} stage inputs", stageInfo.StageInputCount));
			LUCY_INFO(std::format("{0} stage outputs", stageInfo.StageOutputCount));

			if (stageFlag == VK_SHADER_STAGE_VERTEX_BIT)
				ParseShaderInput(compiler, resource.stage_inputs);

			SearchFor(compiler, resource.uniform_buffers, stageFlag, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			SearchFor(compiler, resource.sampled_images, stageFlag, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			SearchFor(compiler, resource.storage_buffers, stageFlag, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			SearchFor(compiler, resource.storage_images, stageFlag, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

			SearchForPushConstants(compiler, resource, stageFlag);
		};

		Reflect(compiler, resourcesShaderStage, m_ShaderStageInfo, stageFlag);

		delete compiler;
	}

	void ShaderReflect::ParseShaderInput(spirv_cross::CompilerGLSL* compiler, const spirv_cross::SmallVector<spirv_cross::Resource>& shaderInputs) {
		for (const auto& shaderInput : shaderInputs) {
			auto id = shaderInput.id;

			VertexShaderLayoutElement element;
			element.Name = shaderInput.name;
			if (element.Name.empty() || element.Name.rfind("_", 0) == 0)
				element.Name = compiler->get_fallback_name(id);
			if (element.Name.empty() || element.Name.rfind("_", 0) == 0)
				element.Name = compiler->get_name(shaderInput.base_type_id);

			element.Location = compiler->get_decoration(id, spv::DecorationLocation);
			element.Type = *(ShaderMemberType*)&compiler->get_type_from_variable(id).basetype;
			element.ShaderDataSize = compiler->get_type_from_variable(id).vecsize;

			m_VertexShaderLayout.push_back(element);
		}
	}

	void ShaderReflect::ParseStructMemberRecursive(spirv_cross::CompilerGLSL* compiler, spirv_cross::SPIRType parentType, std::vector<ShaderMemberVariable>& out) {
		uint32_t index = 0;
		for (auto id : parentType.member_types) {
			auto& memberType = compiler->get_type(id);

			ShaderMemberVariable variable;
			variable.Name = compiler->get_fallback_member_name(index);
			if (variable.Name.empty() || variable.Name.rfind("_", 0) == 0)
				variable.Name = compiler->get_member_name(parentType.self, index);

			variable.Type = *(ShaderMemberType*)&memberType.basetype;
			variable.Offset = compiler->type_struct_member_offset(parentType, index);

			if (variable.Type == ShaderMemberType::Struct) {
				variable.Size = (uint32_t)compiler->get_declared_struct_size(memberType);
				//if the member is a struct or a block and if it has member variables
				ParseStructMemberRecursive(compiler, memberType, variable.Children);
			} else {
				variable.Size = (uint32_t)compiler->get_declared_struct_member_size(parentType, index);
			}

			out.push_back(variable);
			index++;
		}
	}

	void ShaderReflect::SearchFor(spirv_cross::CompilerGLSL* compiler, const spirv_cross::SmallVector<spirv_cross::Resource>& resource,
								  VkShaderStageFlags stageFlag, VkDescriptorType descriptorType) {
		using namespace spirv_cross;

		for (const auto& ub : resource) {
			const auto& type = compiler->get_type(ub.base_type_id);

			ShaderUniformBlock uniformBlock;
			uniformBlock.Name = compiler->get_block_fallback_name(ub.id);
			if (uniformBlock.Name.empty() || uniformBlock.Name.rfind("_", 0) == 0)
				uniformBlock.Name = compiler->get_fallback_name(ub.id);
			if (uniformBlock.Name.empty() || uniformBlock.Name.rfind("_", 0) == 0)
				uniformBlock.Name = compiler->get_name(ub.base_type_id);
			if (uniformBlock.Name.empty())
				uniformBlock.Name = ub.name;

			if (type.basetype == SPIRType::Struct)
				ParseStructMemberRecursive(compiler, type, uniformBlock.Members);

			uint32_t set = compiler->get_decoration(ub.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler->get_decoration(ub.id, spv::DecorationBinding);
			auto& arr = compiler->get_type(ub.type_id).array;
			size_t dimension = arr.size();
			size_t memberCount = type.member_types.size();

			if (dimension) {
				uniformBlock.ArraySize = arr[0];

				if (uniformBlock.ArraySize == 0) //meaning it is a dynamically allocated ubo/ssbo
					uniformBlock.DynamicallyAllocated = true;
			}

			//excluding samplers, since they dont support "get_declared_struct_size", because they don't have a block of member variables for example
			uint32_t bufferSize = 0;
			if (type.basetype != SPIRType::SampledImage && type.basetype != SPIRType::Sampler && type.basetype != SPIRType::Image) {
				bufferSize = (uint32_t)compiler->get_declared_struct_size(type);

				//if (bufferSize == 0 && descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) //means we are dealing with a dynamic ssbo
					//descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;

				//if (bufferSize == 0 && descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) //means we are dealing with a dynamic ubo
					//descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			}

			uniformBlock.Binding = binding;
			uniformBlock.BufferSize = bufferSize;
			uniformBlock.Type = ConvertDescriptorType(descriptorType);
			uniformBlock.StageFlag = stageFlag;

			if (!m_ShaderUniformBlockMap.contains(set)) {
				std::vector<ShaderUniformBlock> buffer;
				buffer.push_back(uniformBlock);
				m_ShaderUniformBlockMap.emplace(set, buffer);
			} else {
				const auto& it = m_ShaderUniformBlockMap.find(set);
				if (CheckIfAlreadyPresent(uniformBlock.Name, it->second))
					continue;
				it->second.push_back(uniformBlock);
			}

			LUCY_INFO(std::format("Name = '{0}'", uniformBlock.Name));
			LUCY_INFO(std::format("Set = {0}", set));
			LUCY_INFO(std::format("IsArray = {0}", (bool)dimension));

			if (dimension) {
				LUCY_INFO(std::format("Array Size = {0}", arr[0]));
				LUCY_INFO(std::format("Is Dynamically Allocated = {0}", uniformBlock.DynamicallyAllocated));
			}

			LUCY_INFO(std::format("Size = {0}", bufferSize));
			LUCY_INFO(std::format("Binding = {0}", binding));
			LUCY_INFO(std::format("Members = {0}", memberCount));
		}
	}

	void ShaderReflect::SearchForPushConstants(spirv_cross::CompilerGLSL* compiler, const spirv_cross::ShaderResources& resource, VkShaderStageFlags stageFlag) {
		for (const auto& ub : resource.push_constant_buffers) {

			ShaderUniformBlock uniformBlock;
			uniformBlock.Name = compiler->get_block_fallback_name(ub.id);
			if (uniformBlock.Name.empty() || uniformBlock.Name.rfind("_", 0) == 0)
				uniformBlock.Name = compiler->get_fallback_name(ub.id);
			if (uniformBlock.Name.empty() || uniformBlock.Name.rfind("_", 0) == 0)
				uniformBlock.Name = compiler->get_name(ub.base_type_id);
			if (uniformBlock.Name.empty())
				uniformBlock.Name = ub.name;

			if (CheckIfAlreadyPresent(uniformBlock.Name, m_ShaderPushConstants))
				return;

			const auto& type = compiler->get_type(ub.base_type_id);

			size_t memberCount = type.member_types.size();
			uint32_t bufferSize = (uint32_t)compiler->get_declared_struct_size(type);

			LUCY_INFO(std::format("Name = '{0}'", uniformBlock.Name));
			LUCY_INFO(std::format("Members = {0}", memberCount));
			LUCY_INFO(std::format("Size = {0}", bufferSize));

			uniformBlock.StageFlag = stageFlag;
			uniformBlock.BufferSize = bufferSize;

			m_ShaderPushConstants.push_back(uniformBlock);
		}
	}

	//if there are multiple occurences between the 2 shader stages (vertex and fragment), dont add a another one but combine them together
	bool ShaderReflect::CheckIfAlreadyPresent(std::string_view uniformBlockName, std::vector<ShaderUniformBlock>& buffer) {
		auto result = std::find_if(buffer.begin(), buffer.end(), [uniformBlockName](const ShaderUniformBlock& uniformBlock) {
			return uniformBlockName == uniformBlock.Name;
		});

		if (result != buffer.end()) {
			size_t index = result - buffer.begin();
			buffer[index].StageFlag = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			return true;
		}
		return false;
	}

	void ShaderReflect::DestroyCachedData() {
		m_ShaderPushConstants.clear();
		m_ShaderUniformBlockMap.clear();
		m_VertexShaderLayout.clear();
		m_ShaderStageInfo = {};
	}
}