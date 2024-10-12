#pragma once

#include <string_view>

namespace Lucy {

	class RenderGraphResource final {
	public:
		RenderGraphResource(const std::string& name, size_t hash);
		RenderGraphResource(const std::string& name);

		inline const std::string& GetName() const { return m_Name; }

		inline bool operator==(const RenderGraphResource& other) const { return m_Hash == other.m_Hash; }
		inline auto operator<=>(const RenderGraphResource& other) const = default;
	private:
		std::string m_Name;
		size_t m_Hash;
	};

#define RGResource(ResourceName) RenderGraphResource(#ResourceName)

	static inline auto UndefinedRenderGraphResource = RGResource(Undefined);
}

template <>
struct std::hash<Lucy::RenderGraphResource> {
	inline size_t operator()(const Lucy::RenderGraphResource& resource) const {
		return std::hash<std::string>{}(resource.GetName());
	}
};