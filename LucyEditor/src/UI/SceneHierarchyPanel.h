#pragma once

#include "UI/Panel.h"

namespace Lucy {
	class SceneHierarchyPanel : public Panel
	{
	public:
		static SceneHierarchyPanel& GetInstance();

	private:
		void Render();
	};
}

