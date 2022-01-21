#pragma once

#include "../../Core/Base.h"
#include "Buffer.h"

#include "vulkan/vulkan.h"

namespace Lucy {

	//Vulkan only
	struct IndexBindInfo {
		VkCommandBuffer CommandBuffer;
	};

	class IndexBuffer : public Buffer<uint32_t> {
	public:
		virtual void Bind(const IndexBindInfo& info) = 0;
		virtual void Unbind() = 0;
		virtual void AddData(const std::vector<uint32_t>& dataToAdd) = 0;
		virtual void Load() = 0;
		virtual void Destroy() = 0;

		static RefLucy<IndexBuffer> Create(uint32_t size);
		static RefLucy<IndexBuffer> Create();
	protected:
		IndexBuffer();
		IndexBuffer(uint32_t size);
		virtual ~IndexBuffer() = default;

		std::vector<uint32_t> m_Data;
		uint32_t m_Size;
	};
}

