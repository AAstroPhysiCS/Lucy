#pragma once

#include "Core/Panel.h"

namespace Lucy {

	class TaskbarPanel : public Panel {
	public:
		static TaskbarPanel& GetInstance();
	private:
		TaskbarPanel() = default;
		virtual ~TaskbarPanel() = default;

		void Render();
	};
}

