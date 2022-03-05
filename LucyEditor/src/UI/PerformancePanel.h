#pragma once

#include "UI/Panel.h"

namespace Lucy {

	class PerformancePanel : public Panel {
	public:
		static PerformancePanel& GetInstance();

		void SetShow(bool show);
		virtual void Render();

	private:
		PerformancePanel() = default;
		virtual ~PerformancePanel() = default;

		bool m_Show;
	};
}

