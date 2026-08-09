#pragma once

#include "Core/Base.h"
#include "Buffer.h"

#include "Renderer/Device/RenderDeviceResource.h"

namespace Lucy {

	class IndexBuffer : public IntBuffer, public RenderDeviceResource {
	public:
		virtual ~IndexBuffer() = default;

		IndexBuffer(const IndexBuffer&) = delete;
		IndexBuffer& operator=(const IndexBuffer&) = delete;
		IndexBuffer(IndexBuffer&&) = delete;
		IndexBuffer& operator=(IndexBuffer&&) = delete;

		virtual void RTLoadToDevice(RenderDevice* device) = 0;
	protected:
		IndexBuffer(size_t size) 
			: RenderDeviceResource("Index Buffer") {
			Resize(size); //internal std::vector allocation
		}
	};
}

