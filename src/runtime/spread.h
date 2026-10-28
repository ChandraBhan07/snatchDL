#pragma once

#include <algorithm>
#include <random>
#include <utility>

namespace Spread
{
    inline int random(int min, int max, std::mt19937 &rng)
    {
        constexpr int regionCount = 5;
        constexpr int jitter = 5;

        int range = max - min + 1;
        int regionSize = range / regionCount;

        std::vector<std::pair<int, int>> regions;
        regions.reserve(regionCount);

        for (int i = 0; i < regionCount; ++i)
        {
            int regionMin = min + i * regionSize;

            int regionMax;

            if (i == regionCount - 1)
                regionMax = max;
            else
                regionMax = regionMin + regionSize - 1;

            regions.emplace_back(regionMin, regionMax);
        }

        std::shuffle(regions.begin(), regions.end(), rng);

        const auto &region = regions.front();

        std::uniform_int_distribution<int> valueDist(
            region.first,
            region.second);

        int value = valueDist(rng);

        std::uniform_int_distribution<int> jitterDist(-jitter, jitter);

        value += jitterDist(rng);

        value = std::clamp(value, min, max);

        return value;
    }

}
