#pragma once

#include "Core/Panel.h"

namespace Lucy {

	class DebugPanel : public Panel {
	public:
		static DebugPanel& GetInstance();
	private:
		void Render();

		DebugPanel() = default;
		virtual ~DebugPanel() = default;
	};
}

