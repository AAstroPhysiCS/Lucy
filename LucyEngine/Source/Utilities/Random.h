#pragma once

#include <random>
#include <limits>

namespace Lucy {

    //using a struct here just to not have any ambiguousity in the constructor
	struct RandomSeed {
		std::uint64_t Value;
	};

    template <typename TType, template <typename> typename TDistribution>
    class Random final {
    public:
        using Distribution = TDistribution<TType>;

        Random(TType min = TType(0), TType max = std::numeric_limits<TType>::max())
            : m_Generator(CreateSeededGenerator()), m_Distribution(min, max) {}

        Random(RandomSeed seed, TType min = TType(0), TType max = std::numeric_limits<TType>::max())
            : m_Generator(seed.Value), m_Distribution(min, max) {}

        ~Random() = default;

        Random(const Random&) = delete;
        Random(Random&&) noexcept = delete;
        Random& operator=(const Random&) = delete;
        Random& operator=(Random&&) noexcept = delete;

        [[nodiscard]] TType NextValue() {
            return m_Distribution(m_Generator);
        }
    private:
        static std::mt19937_64 CreateSeededGenerator() {
            std::random_device randomDevice;
            std::seed_seq seedSeq{ randomDevice(), randomDevice() };
            return std::mt19937_64(seedSeq);
        }
    private:
        std::mt19937_64 m_Generator;
        Distribution m_Distribution;
    };

    template <typename TType>
    using UniformRandom = Random<TType, std::uniform_int_distribution>;

    template <typename TType>
    using UniformRealRandom = Random<TType, std::uniform_real_distribution>;
}