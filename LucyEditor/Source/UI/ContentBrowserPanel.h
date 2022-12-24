#pragma once

#include "Core/Panel.h"

namespace Lucy {

	class ContentBrowserPanel : public Panel {
	public:
		static ContentBrowserPanel& GetInstance();

		void Render() final override;
	private:
		ContentBrowserPanel() = default;
		virtual ~ContentBrowserPanel() = default;
	};
}