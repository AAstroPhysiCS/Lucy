#include "lypch.h"
#include "RenderGraphResource.h"

namespace Lucy {

	RenderGraphResource::RenderGraphResource(const std::string& name, size_t hash)
		: m_Name(name), m_Hash(hash) {
	}

	RenderGraphResource::RenderGraphResource(const std::string& name)
		: m_Name(name), m_Hash(std::hash<std::string>{}(m_Name)) {
	}
}