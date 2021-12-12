#pragma once

namespace Lucy {

	class UUID {
		UUID();

		std::string m_UUIDAsString = "0";

		friend struct UUIDComponent;
	public:
		bool operator==(UUID& other) {
			return m_UUIDAsString == other.m_UUIDAsString;
		}
	};
}

