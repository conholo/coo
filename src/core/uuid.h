#pragma once

#include <cstdint>
#include <functional>

class uuid
{
public:
    uuid();
    explicit uuid(uint64_t uuid);
    uuid(const uuid &) = default;

    explicit operator uint64_t() const { return m_uuid; }
	bool operator==(const uuid& other) const { return m_uuid == other.m_uuid; }
	bool operator!=(const uuid& other) const { return m_uuid != other.m_uuid; }

private:
    uint64_t m_uuid;
};

namespace std
{
    template<>
    struct hash<::uuid>
    {
        std::size_t operator()(const ::uuid &uuid) const

        noexcept
        {
            return static_cast<std::size_t>(static_cast<uint64_t>(uuid));
        }
    };
}
