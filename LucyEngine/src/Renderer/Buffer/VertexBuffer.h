#pragma once

#include "../../Core/Base.h"
#include "Buffer.h"

#include "vulkan/vulkan.h"

namespace Lucy {

	//Vulkan only
	struct VertexBindInfo {
		VkCommandBuffer CommandBuffer;
	};

	class VertexBuffer : public Buffer<float> {
	public:
		virtual void Bind(const VertexBindInfo& info) = 0;
		virtual void Unbind() = 0;
		virtual void AddData(const std::vector<float>& dataToAdd) = 0;
		virtual void Load() = 0;
		virtual void Destroy() = 0;

		static RefLucy<VertexBuffer> Create(uint32_t size);
		static RefLucy<VertexBuffer> Create();
	protected:
		VertexBuffer(uint32_t size);
		VertexBuffer();
		virtual ~VertexBuffer() = default;

		std::vector<float> m_Data;
		uint32_t m_Size;
	};
}

