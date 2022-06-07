#pragma once

#include "UI/Panel.h"

namespace Lucy {

	class TaskbarPanel : public Panel {
	public:
		static TaskbarPanel& GetInstance();
	private:
		void Render();

		TaskbarPanel() = default;
		virtual ~TaskbarPanel() = default;
	};
}

