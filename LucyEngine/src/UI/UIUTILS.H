#pragma once

namespace Lucy {

	class UIUtils {
	public:
		static void TextCenter(const std::string& text, float xPadding = 0, uint32_t indent = 0);
		static void TextCenter(const char const* text, float xPadding = 0, uint32_t indent = 0);
		static void TextCenterTable(const char const* text, float xPadding = 0, float yPadding = 0, uint32_t indent = 0);
		static void TransformControl(const char* id, float& x, float& y, float& z, float defaultValue, float speed);
	private:
		UIUtils() = default;
	};
}

