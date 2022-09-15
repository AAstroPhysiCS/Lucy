#pragma once

#include "Core/Panel.h"
#include "Scene/Entity.h"

namespace Lucy {

	class DetailsPanel : public Panel
	{
	public:
		static DetailsPanel& GetInstance();
	private:
		DetailsPanel();
		virtual ~DetailsPanel() = default;

		void OnDestroy() final override;
		void Render() final override;

		template <typename T>
		static void DrawComponentPanel(Entity& e, std::function<void(T&)> func) {
			if (e.HasComponent<T>())
				func(e.GetComponent<T>());
		}

		//relevant for DetailsPanel, since I cant render a nullptr vulkan descriptor set.
		inline static Ref<Image> s_CheckerBoardTexture = nullptr;
	};
}

