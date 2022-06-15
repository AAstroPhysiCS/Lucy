#pragma once

namespace Lucy {

	class UUID {
		UUID();
		~UUID() = default;

		std::string m_UUIDAsString = "0";
		friend struct UUIDComponent;
	public:
		bool operator==(const UUID& other) {
			return m_UUIDAsString == other.m_UUIDAsString;
		}
	};
}

