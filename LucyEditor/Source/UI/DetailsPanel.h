#pragma once

#include "Core/Panel.h"
#include "Scene/Entity.h"

#include "Renderer/Device/RenderDeviceResource.h"

namespace Lucy {

	class DetailsPanel : public Panel {
	public:
		static DetailsPanel& GetInstance();

		DetailsPanel();
		virtual ~DetailsPanel() = default;

		void OnDestroy() final override;
		void Render() final override;

		template <typename T>
		static void DrawComponentPanel(Entity& e, std::function<void(T&)> func) {
			if (e.HasComponent<T>())
				func(e.GetComponent<T>());
		}

		inline static RenderDeviceResourceHandle s_CheckerBoardTextureHandle{};
	};
}

