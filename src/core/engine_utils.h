#pragma once
#include <functional>
#include <fstream>

#define BIND_FN(fn) [this](auto&& ... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace EngineUtils
{
    // from: https://stackoverflow.com/a/57595105
    template<typename T, typename... Rest>
    void HashCombine(std::size_t &seed, const T &v, const Rest &... rest)
    {
        seed ^= std::hash <T> {}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        (HashCombine(seed, rest), ...);
    };

    std::vector<char> ReadFile(const std::string &filepath);
}