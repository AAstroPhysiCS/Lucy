#pragma once

#include "Core/Panel.h"

namespace Lucy {

	class ApplicationMetricsPanel : public Panel {
	public:
		static ApplicationMetricsPanel& GetInstance();

		void SetShow(bool show);
	private:
		bool m_Show = false;

		void Render();

		ApplicationMetricsPanel() = default;
		virtual ~ApplicationMetricsPanel() = default;
	};
}

