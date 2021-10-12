#pragma once

#include "UI/Panel.h"

namespace Lucy {
	class TaskbarPanel : public Panel
	{
	public:
		void Render();

		static TaskbarPanel& GetInstance();
	};
}

