#pragma once

#include "UI/Panel.h"

namespace Lucy {

	class PropertiesPanel : public Panel
	{
	public:
		static PropertiesPanel& GetInstance();

		void Render();
	private:
		void RenderTransformControl(const char* id, float& x, float& y, float& z, float defaultValue, float speed);
	};
}

