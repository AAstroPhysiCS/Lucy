#pragma once

#include "Image.h"

namespace Lucy {

	class OpenGLImage2D : public Image2D {
	public:
		OpenGLImage2D(const std::string& path, ImageCreateInfo& createInfo);
		OpenGLImage2D(ImageCreateInfo& createInfo);
		virtual ~OpenGLImage2D() = default;

		void Bind();
		void Unbind();
		void Destroy() override;

		inline uint16_t GetTarget() const { return m_Target; }
		inline uint32_t GetID() const { return m_Id; }
		inline uint32_t GetSlot() const { return m_Slot; }
	private:
		void SetParameters(const Ref<OpenGLRHIImageDesc>& imageDesc);

		uint16_t m_Target;
		uint32_t m_Id;
		uint32_t m_Slot;
	};
}

