#include "lypch.h"
#include "UUID.h"

#include <random>
#include <sstream>

#include "../Core/Base.h"

namespace Lucy {

	static std::random_device              rd;
	static std::mt19937                    gen(rd());
	static std::uniform_int_distribution<> dis(0, 15);
	static std::uniform_int_distribution<> dis2(8, 11);

	UUID::UUID() {
		std::stringstream ss;
		int i;
		ss << std::hex;
		for (i = 0; i < 8; i++) {
			ss << dis(gen);
		}
		ss << "-";
		for (i = 0; i < 4; i++) {
			ss << dis(gen);
		}
		ss << "-4";
		for (i = 0; i < 3; i++) {
			ss << dis(gen);
		}
		ss << "-";
		ss << dis2(gen);
		for (i = 0; i < 3; i++) {
			ss << dis(gen);
		}
		ss << "-";
		for (i = 0; i < 12; i++) {
			ss << dis(gen);
		};

		m_UUIDAsString = ss.str();
	}

	LucyID IDProvider::RequestID() {
		//basically, init
		if (s_IDCount.size() == 0)
			IncreasePool(s_PoolResizeStep);

		const auto& result = std::find(s_IDCount.begin(), s_IDCount.end(), 0);

		if (result == s_IDCount.end()) {
			IncreasePool(s_PoolResizeStep);
			return RequestID();
		}

		uint32_t index = std::distance(s_IDCount.begin(), result);
		s_IDCount[index] = index + 1;

		return index;
	}

	void IDProvider::ReturnID(LucyID id) {
		const auto& result = std::find(s_IDCount.begin(), s_IDCount.end(), id);

		if (result != s_IDCount.end()) {
			s_IDCount.erase(result);
			return;
		}

		LUCY_CRITICAL(fmt::format("Material ID: {0} does not exist!", id));
		LUCY_ASSERT(false);
	}

	LucyID IDProvider::Renew(LucyID oldId) {
		ReturnID(oldId);
		return RequestID();
	}

	void IDProvider::IncreasePool(size_t step) {
		s_IDCount.resize(s_IDCount.size() + step);
	}
}