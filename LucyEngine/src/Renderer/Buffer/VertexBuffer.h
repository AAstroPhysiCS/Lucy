#pragma once

#include "../../Core/Base.h"
#include "Buffer.h"

namespace Lucy {

	class VertexBuffer : public Buffer<float> {
	public:
		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual void AddData(std::vector<float>& dataToAdd) = 0;
		virtual void Load() = 0;
		virtual void Destroy() = 0;

		static RefLucy<VertexBuffer> Create(uint32_t size);
		static RefLucy<VertexBuffer> Create();
	protected:
		VertexBuffer(uint32_t size);
		VertexBuffer();
		virtual ~VertexBuffer() = default;

		std::vector<float> m_Data;
	};
}

