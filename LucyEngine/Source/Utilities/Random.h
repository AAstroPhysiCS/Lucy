#pragma once

#include <random>
#include <limits>

namespace Lucy {

	template <typename TType, template <typename> typename TDistribution>
	class Random final {
	public:
		template <typename TRangeMin, typename TRangeMax>
		Random(size_t seed, TRangeMin min = TType(0), TRangeMax max = std::numeric_limits<TType>::max())
			: s_Distribution(min, max), s_SeedSeq({ seed * std::random_device{}() }), s_Generator(s_SeedSeq) {
		}

		template <typename TRangeMin = TType, typename TRangeMax = TType>
		Random(TRangeMin min = TType(0), TRangeMax max = std::numeric_limits<TType>::max())
			: s_Distribution(min, max), s_Generator(std::random_device{}()) {
		}

		~Random() = default;

		inline [[nodiscard]] TType NextValue() { return s_Distribution(s_Generator); }
	private:
		std::mt19937 s_Generator;
		std::seed_seq s_SeedSeq;
		TDistribution<TType> s_Distribution;
	};

	template <typename TType>
	using UniformRandom = Random<TType, std::uniform_int_distribution>;

	template <typename TType>
	using UniformRealRandom = Random<TType, std::uniform_real_distribution>;
}