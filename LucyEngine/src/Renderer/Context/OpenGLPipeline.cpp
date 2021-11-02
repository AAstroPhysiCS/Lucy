#include "OpenGLPipeline.h"

#include "glad/glad.h"

namespace Lucy {
	
	OpenGLPipeline::OpenGLPipeline(PipelineSpecification& specs)
		: Pipeline(specs)
	{
	}

	uint32_t OpenGLPipeline::GetTypeFromSize(ShaderDataSize size)
	{
		switch (size) {
			case ShaderDataSize::Int1:
			case ShaderDataSize::Float1: return 1; break;
			case ShaderDataSize::Int2:
			case ShaderDataSize::Float2: return 2; break;
			case ShaderDataSize::Int3:
			case ShaderDataSize::Float3: return 3; break;
			case ShaderDataSize::Int4:
			case ShaderDataSize::Float4: return 4; break;
			case ShaderDataSize::Mat4: return 4 * 4; break;
		}
	}

	uint32_t OpenGLPipeline::CalculateStride()
	{
		uint32_t stride = 0;
		for (auto [name, size] : m_Specs.VertexShaderLayout.ElementList) {
			stride += (uint32_t) size;
		}
		return stride;
	}

	void OpenGLPipeline::UploadVertexLayout(RefLucy<OpenGLVertexBuffer>& vertexBuffer)
	{
		vertexBuffer->Bind();
		
		uint32_t stride = CalculateStride();
		uint32_t offset = 0;
		uint32_t bufferIndex = 0;
		
		for (auto [name, size] : m_Specs.VertexShaderLayout.ElementList) {
			glEnableVertexAttribArray(bufferIndex);
			uint32_t apiSize = GetTypeFromSize(size);

			switch (size) {
				case ShaderDataSize::Float1:
				case ShaderDataSize::Float2:
				case ShaderDataSize::Float3:
				case ShaderDataSize::Float4:
				case ShaderDataSize::Mat4:
					glVertexAttribPointer(bufferIndex, apiSize, GL_FLOAT, GL_FALSE, stride * sizeof(float), (const void*) (offset * sizeof(float)));
					break;
				case ShaderDataSize::Int1:
				case ShaderDataSize::Int2:
				case ShaderDataSize::Int3:
				case ShaderDataSize::Int4:
					glVertexAttribIPointer(bufferIndex, apiSize, GL_INT, stride * sizeof(float), (const void*) (offset * sizeof(float)));
					break;
			}

			offset += apiSize;
			bufferIndex++;
		}

		vertexBuffer->Unbind();
	}
}
