#pragma once

#include "Core/Base.h"
#include "Buffer.h"

#include "Renderer/Device/RenderDeviceResource.h"

#include "Renderer/Mesh.h"

namespace Lucy {

	class VertexBuffer : public Buffer<Vertex>, public RenderDeviceResource {
	public:
		virtual ~VertexBuffer() = default;

		VertexBuffer(const VertexBuffer&) = delete;
		VertexBuffer& operator=(const VertexBuffer&) = delete;
		VertexBuffer(VertexBuffer&&) = delete;
		VertexBuffer& operator=(VertexBuffer&&) = delete;

		virtual void RTLoadToDevice() = 0;
	protected:
		VertexBuffer(size_t size) 
			: RenderDeviceResource("Vertex Buffer") {
			Resize(size); //internal std::vector allocation
		}
	};
}