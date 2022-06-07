#pragma once

#include "UI/Panel.h"

namespace Lucy {

	class PerformancePanel : public Panel {
	public:
		static PerformancePanel& GetInstance();

		void SetShow(bool show);
	private:
		PerformancePanel() = default;
		virtual ~PerformancePanel() = default;
		
		void Render();

		bool m_Show;
	};
}

