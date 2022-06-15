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
		virtual ~VertexBuffer() = default;
		
		virtual void Bind(const VertexBindInfo& info) = 0;
		virtual void Unbind() = 0;
		virtual void LoadToGPU() = 0;
		virtual void DestroyHandle() = 0;

		static RefLucy<VertexBuffer> Create(uint32_t size);
		static RefLucy<VertexBuffer> Create();
	protected:
		VertexBuffer(uint32_t size);
		VertexBuffer() = default;
	};
}

