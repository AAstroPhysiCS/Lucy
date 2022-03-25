#pragma once

#include "Texture.h"

namespace Lucy {

	class OpenGLTexture2D : public Texture2D {
	public:
		OpenGLTexture2D(TextureSpecification& specs);
		virtual ~OpenGLTexture2D() = default;

		void Bind() override;
		void Unbind() override;
		void Destroy() override;

		inline uint16_t GetTarget() const { return m_Target; }
	private:
		uint16_t m_Target;
	};
}

