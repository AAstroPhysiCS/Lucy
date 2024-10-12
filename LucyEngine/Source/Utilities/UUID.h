#pragma once

namespace Lucy {

	static constexpr inline const char* DefaultUUID = "00000000-0000-0000-0000-000000000000";

	class UUID final {
	public:
		UUID();
		~UUID() = default;

		inline bool operator==(const UUID& other) const = default;
		inline bool operator==(const char* uuidAsString) const { return m_UUIDAsString == uuidAsString; }

		inline const std::string& GetID() const { return m_UUIDAsString; }
	private:
		std::string m_UUIDAsString = DefaultUUID;
	};

	using LucyID = uint64_t;

	template <typename T>
	inline static constexpr const T InvalidID = T(~0);

	template <typename TID = LucyID>
	class IDProvider final {
		static inline constexpr const size_t s_PoolResizeStep = 100;
	public:
		IDProvider() = default;
		~IDProvider() = default;

		TID RequestID();
		void ReturnID(TID id);
		TID Renew(TID oldId);
	private:
		void IncreasePool(size_t step);

		std::vector<TID> s_IDCount;
	};

	template<typename TID>
	TID IDProvider<TID>::RequestID() {
		//basically, init
		if (s_IDCount.empty())
			IncreasePool(s_PoolResizeStep);

		const auto& result = std::find(s_IDCount.begin(), s_IDCount.end(), 0);

		if (result == s_IDCount.end()) {
			IncreasePool(s_PoolResizeStep);
			return RequestID();
		}

		ptrdiff_t index = std::distance(s_IDCount.begin(), result);
		s_IDCount[index] = index + 1;

		return index;
	}

	template<typename TID>
	void IDProvider<TID>::ReturnID(TID id) {
		const auto& result = std::find(s_IDCount.begin(), s_IDCount.end(), id);

		if (result != s_IDCount.end()) {
			s_IDCount.erase(result);
			return;
		}

		LUCY_ASSERT(false, "IDProvider: {0} does not exist!", id);
	}

	template<typename TID>
	TID IDProvider<TID>::Renew(TID oldId) {
		ReturnID(oldId);
		return RequestID();
	}

	template<typename TID>
	void IDProvider<TID>::IncreasePool(size_t step) {
		s_IDCount.resize(s_IDCount.size() + step);
	}
}

template<>
struct std::hash<Lucy::UUID> {
	inline size_t operator()(const Lucy::UUID& uuid) const {
		return std::hash<std::string>{}(uuid.GetID());
	}
};

template <>
struct std::formatter<Lucy::UUID> {
	constexpr auto parse(const std::format_parse_context& ctx) const {
		return ctx.begin();
	}

	auto format(const Lucy::UUID& uuid, std::format_context& ctx) const {
		return std::format_to(ctx.out(), "{}", uuid.GetID());
	}
};