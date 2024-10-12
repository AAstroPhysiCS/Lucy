#pragma once

#include "Core/Base.h"
#include "Buffer.h"

#include "Renderer/Device/RenderResource.h"

namespace Lucy {

	class VertexBuffer : public FloatBuffer, public RenderResource {
	public:
		virtual ~VertexBuffer() = default;

		virtual void RTLoadToDevice() = 0;
	protected:
		VertexBuffer(size_t size) 
			: RenderResource("Vertex Buffer") {
			Resize(size); //internal std::vector allocation
		}
	};
}