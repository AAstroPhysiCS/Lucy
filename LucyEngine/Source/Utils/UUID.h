#pragma once

namespace Lucy {

	class UUID final {
	public:
		bool operator==(const UUID& other) {
			return m_UUIDAsString == other.m_UUIDAsString;
		}
	private:
		UUID();
		~UUID() = default;

		std::string m_UUIDAsString = "0";
		friend struct UUIDComponent;
	};

	typedef uint64_t LucyID;

	struct IDProvider final {
	private:
		static constexpr size_t s_PoolResizeStep = 100;
	public:
		IDProvider() = default;
		~IDProvider() = default;

		LucyID RequestID();
		void ReturnID(LucyID id);
		LucyID Renew(LucyID oldId);
	private:
		void IncreasePool(size_t step);

		std::vector<LucyID> s_IDCount;
	};
}