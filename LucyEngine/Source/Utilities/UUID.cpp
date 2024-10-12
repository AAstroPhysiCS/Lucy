#include "lypch.h"
#include "UUID.h"
#include "Random.h"

namespace Lucy {

	static UniformRandom<uint64_t> s_RandomGen(0, 15);
	static UniformRandom<uint64_t> s_RandomGen2(8, 11);

	UUID::UUID() {
		std::stringstream ss;
		int i;
		ss << std::hex;
		for (i = 0; i < 8; i++) {
			ss << s_RandomGen.NextValue();
		}
		ss << "-";
		for (i = 0; i < 4; i++) {
			ss << s_RandomGen.NextValue();
		}
		ss << "-4";
		for (i = 0; i < 3; i++) {
			ss << s_RandomGen.NextValue();
		}
		ss << "-";
		ss << s_RandomGen2.NextValue();
		for (i = 0; i < 3; i++) {
			ss << s_RandomGen.NextValue();
		}
		ss << "-";
		for (i = 0; i < 12; i++) {
			ss << s_RandomGen.NextValue();
		};

		m_UUIDAsString = ss.str();
	}
}