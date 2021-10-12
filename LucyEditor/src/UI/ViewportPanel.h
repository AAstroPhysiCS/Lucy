#pragma once

#include "UI/Panel.h"

namespace Lucy {

	class ViewportPanel : public Panel
	{
	public:

		static ViewportPanel& GetInstance();

		void Render();

	};

}

