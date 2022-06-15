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
		virtual ~IndexBuffer() = default;

		virtual void Bind(const IndexBindInfo& info) = 0;
		virtual void Unbind() = 0;
		virtual void LoadToGPU() = 0;
		virtual void DestroyHandle() = 0;

		static RefLucy<IndexBuffer> Create(uint32_t size);
		static RefLucy<IndexBuffer> Create();
	protected:
		IndexBuffer(uint32_t size);
		IndexBuffer() = default;
	};
}

