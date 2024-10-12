#pragma once

#include "Core/Base.h"
#include "Buffer.h"

#include "Renderer/Device/RenderResource.h"

namespace Lucy {

	class IndexBuffer : public IntBuffer, public RenderResource {
	public:
		virtual ~IndexBuffer() = default;

		virtual void RTLoadToDevice() = 0;
	protected:
		IndexBuffer(size_t size) 
			: RenderResource("Index Buffer") {
			Resize(size); //internal std::vector allocation
		}
	};
}

