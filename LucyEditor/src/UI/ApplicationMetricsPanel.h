#pragma once

#include "Core/Panel.h"

namespace Lucy {

	class ApplicationMetricsPanel : public Panel {
	public:
		static ApplicationMetricsPanel& GetInstance();

		void SetShow(bool show);
	private:
		ApplicationMetricsPanel() = default;
		virtual ~ApplicationMetricsPanel() = default;
		
		void Render();

		bool m_Show;
	};
}

