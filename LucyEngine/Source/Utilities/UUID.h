#pragma once

#include "Random.h"

namespace Lucy {

	class UUID final {
	public:
		UUID() {
			std::stringstream ss;
			int32_t i;
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
		~UUID() = default;

		UUID(const UUID&) = default;
		UUID& operator=(const UUID&) = default;
		UUID(UUID&&) noexcept = default;
		UUID& operator=(UUID&&) noexcept = default;

		inline bool operator==(const UUID& other) const = default;
		inline bool operator==(const char* uuidAsString) const { return m_UUIDAsString == uuidAsString; }

		inline const std::string& GetID() const { return m_UUIDAsString; }
	private:
		static constexpr const char* DefaultUUID = "00000000-0000-0000-0000-000000000000";
	private:
		std::string m_UUIDAsString = DefaultUUID;

		static inline UniformRandom<uint64_t> s_RandomGen{ 0uLL, 15uLL };
		static inline UniformRandom<uint64_t> s_RandomGen2{ 8uLL, 11uLL };
	};

	using LucyID = uint64_t;

	template<typename T>
	concept Arithmetic = std::integral<T> || std::floating_point<T>;

	template<typename T> requires Arithmetic<T>
	inline constexpr T InvalidID = std::numeric_limits<T>::max();

	template<Arithmetic TID = LucyID, bool ReuseReturnedIDs = true>
	class IDProvider final {
	public:
		IDProvider() = default;
		~IDProvider() = default;

		IDProvider(const IDProvider&) = delete;
		IDProvider& operator=(const IDProvider&) = delete;
		IDProvider(IDProvider&&) noexcept = default;
		IDProvider& operator=(IDProvider&&) noexcept = default;

		[[nodiscard]] TID RequestID() {
			if constexpr (ReuseReturnedIDs) {
				if (!m_FreeList.empty()) {
					const TID id = m_FreeList.back();
					m_FreeList.pop_back();

					m_Alive[static_cast<size_t>(id)] = 1;
					return id;
				}
			}

			LUCY_ASSERT(m_NextID != InvalidID<TID>, "IDProvider overflow!");

			const TID id = m_NextID++;
			m_Alive.push_back(1);

			return id;
		}

		void ReturnID(TID id) {
			if constexpr (!ReuseReturnedIDs) {
				static_assert(ReuseReturnedIDs, "This IDProvider has ID returning disabled.");
			} else {
				const size_t index = static_cast<size_t>(id);

				LUCY_ASSERT(index < m_Alive.size(), "IDProvider: ID does not exist!");
				LUCY_ASSERT(m_Alive[index] != 0, "IDProvider: ID was already returned!");

				m_Alive[index] = 0;
				m_FreeList.push_back(id);
			}
		}

		[[nodiscard]] TID Renew(TID oldID) {
			ReturnID(oldID);
			return RequestID();
		}

		[[nodiscard]] bool IsAlive(TID id) const {
			const size_t index = static_cast<size_t>(id);
			return index < m_Alive.size() && m_Alive[index] != 0;
		}

		[[nodiscard]] size_t GetCapacity() const {
			return m_Alive.size();
		}

		void Reset() {
			m_NextID = 0;
			m_Alive.clear();
			m_FreeList.clear();
		}
	private:
		TID m_NextID = 0;
		std::vector<uint8_t> m_Alive;
		std::vector<TID> m_FreeList;
	};
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