#pragma once
#include <cstddef>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <type_traits>

class Buffer {
public:
    Buffer() noexcept = default;
    explicit Buffer(std::size_t size) { Allocate(size); }
    Buffer(const void* data, std::size_t size) { Allocate(size); Write(data, size); }

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept
            : m_Data(std::move(other.m_Data)), m_Size(other.m_Size)
    {
        other.m_Size = 0;
    }

    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            m_Data = std::move(other.m_Data);
            m_Size = other.m_Size;
            other.m_Size = 0;
        }
        return *this;
    }

    ~Buffer() = default;

    static Buffer Copy(const Buffer& other) {
        Buffer buffer(other.m_Size);
        std::memcpy(buffer.Data(), other.Data(), other.m_Size);
        return buffer;
    }

    static Buffer Copy(const void* data, std::size_t size) {
        Buffer buffer(size);
        std::memcpy(buffer.Data(), data, size);
        return buffer;
    }

    void Allocate(std::size_t size) {
        m_Data.reset(new uint8_t[size]);
        m_Size = size;
    }

    void Release() noexcept
    {
        m_Data.reset();
        m_Size = 0;
    }

    void ZeroInitialize() const noexcept
    {
        if (m_Data)
        {
            std::memset(m_Data.get(), 0, m_Size);
        }
    }

    template<typename T>
    T& Read(std::size_t offset = 0)
    {
        if (offset + sizeof(T) > m_Size)
        {
            throw std::out_of_range("Buffer overflow in Read");
        }
        return *reinterpret_cast<T*>(m_Data.get() + offset);
    }

    template<typename T>
    const T& Read(std::size_t offset = 0) const
    {
        if (offset + sizeof(T) > m_Size)
        {
            throw std::out_of_range("Buffer overflow in Read");
        }
        return *reinterpret_cast<const T*>(m_Data.get() + offset);
    }

    std::unique_ptr<uint8_t[]> ReadBytes(std::size_t size, std::size_t offset) const
    {
        if (offset + size > m_Size)
        {
            throw std::out_of_range("Buffer overflow in ReadBytes");
        }
        auto buffer = std::make_unique<uint8_t[]>(size);
        std::memcpy(buffer.get(), m_Data.get() + offset, size);
        return buffer;
    }

    void Write(const void* data, std::size_t size, std::size_t offset = 0)
    {
        if (offset + size > m_Size)
        {
            throw std::out_of_range("Buffer overflow in Write");
        }
        std::memcpy(m_Data.get() + offset, data, size);
    }

    explicit operator bool() const noexcept { return static_cast<bool>(m_Data); }

    uint8_t& operator[](std::size_t index)
    {
        if (index >= m_Size)
        {
            throw std::out_of_range("Buffer index out of range");
        }
        return m_Data[index];
    }

    uint8_t operator[](std::size_t index) const
    {
        if (index >= m_Size) {
            throw std::out_of_range("Buffer index out of range");
        }
        return m_Data[index];
    }

    template<typename T>
    T* As() const noexcept
    {
        return reinterpret_cast<T*>(m_Data.get());
    }

    std::size_t GetSize() const noexcept { return m_Size; }
    uint8_t* Data() noexcept { return m_Data.get(); }
    const uint8_t* Data() const noexcept { return m_Data.get(); }

private:
    std::unique_ptr<uint8_t[]> m_Data;
    std::size_t m_Size = 0;
};