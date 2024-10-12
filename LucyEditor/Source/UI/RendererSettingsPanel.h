#pragma once

#include "Core/Panel.h"

namespace Lucy {

	class RendererSettingsPanel final : public Panel {
	public:
		static RendererSettingsPanel& GetInstance();

		RendererSettingsPanel() = default;
		virtual ~RendererSettingsPanel() = default;

		void Render() final override;
	};
}