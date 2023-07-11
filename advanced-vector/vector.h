#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept { 
        buffer_ = other.buffer_;
        capacity_ = other.capacity_;
        other.buffer_ = nullptr;
        other.capacity_ = 0;
    }

    RawMemory& operator=(RawMemory&& rhs) noexcept {
        buffer_ = rhs.buffer_;
        capacity_ = rhs.capacity_;
        rhs.buffer_ = nullptr;
        rhs.capacity_ = 0;
        return *this;
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index <= capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:

    using iterator = T*;
    using const_iterator = const T*;
    
    iterator begin() noexcept {
        if (Capacity() == 0) {
            return nullptr;
        } else {
            return &data_[0];
        }
    }
    iterator end() noexcept {
        if (Capacity() == 0) {
            return nullptr;
        } else {
            return &data_[size_];
        }
    }

    const_iterator begin() const noexcept {
        if (Capacity() == 0) {
            return nullptr;
        } else {
            return const_cast<T*>(&data_[0]);
        }
    }
    const_iterator end() const noexcept {
        if (Capacity() == 0) {
            return nullptr;
        } else {
            return const_cast<T*>(&data_[size_]);
        }
    }

    const_iterator cbegin() const noexcept {
        return begin();
    }
    const_iterator cend() const noexcept {
        return end();
    }

    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size_);
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_) 
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), other.Size(), data_.GetAddress());
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            } else {
                if (Size() > rhs.Size()) {
                    for (size_t i = 0; i != rhs.Size(); ++i) {
                        data_[i] = rhs[i];
                    }
                    std::destroy_n(data_.GetAddress() + rhs.Size(), Size() - rhs.Size());
                } else {
                    for (size_t i = 0; i != Size(); ++i) {
                        data_[i] = rhs[i];
                    }
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + Size(), rhs.Size() - Size(), data_.GetAddress() + Size());
                }
            }
            size_ = rhs.size_;
        }
        return *this;
    }
    

    Vector(Vector&& other) noexcept {
        data_ = std::move(other.data_);
        size_ = other.size_;
        other.size_ = 0;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        data_ = std::move(rhs.data_);
        size_ = rhs.size_;
        rhs.size_ = 0;
        return *this;
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        } else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);

        data_.Swap(new_data);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    void Resize(size_t new_size) {
        if (new_size == size_) {
            return;
        }
        if (new_size < size_) {
            std::destroy_n(data_.GetAddress() + new_size, (size_ - new_size));            
        } else {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
        }
        size_ = new_size;
    }

    void PushBack(const T& value) {
        EmplaceShared(end(), value);
    }

    void PushBack(T&& value) {
        EmplaceShared(end(), std::forward<T>(value));
    }
    void PopBack() noexcept {
        std::destroy_n(data_.GetAddress() + (size_ - 1), 1);
        --size_;
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        return *(EmplaceShared(end(), std::forward<Args>(args)...));
    }

    iterator Erase(const_iterator pos) noexcept {
        pos->~T();
        std::move(const_cast<iterator>(pos) + 1, end(), const_cast<iterator>(pos));
        --size_;
        return begin() + std::distance(cbegin(), pos);
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        return EmplaceShared(pos, std::forward<Args>(args)...);
    }

    iterator Insert(const_iterator pos, const T& value) {
        return EmplaceShared(pos, value);
    }
    iterator Insert(const_iterator pos, T&& value) {
        return EmplaceShared(pos, std::forward<T>(value));
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;

    template <typename... Args>
    iterator EmplaceShared(const_iterator pos, Args&&... args) {
        iterator result;
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            result = new (new_data.GetAddress() + std::distance(cbegin(), pos)) T(std::forward<Args>(args)...);
            try {
                if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                    std::uninitialized_move_n(begin(), std::distance(cbegin(), pos), new_data.GetAddress());
                } else {
                    std::uninitialized_copy_n(cbegin(), std::distance(cbegin(), pos), new_data.GetAddress());
                }
            } catch (...) {
                result->~T();
                throw;
            }
            try {
                if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                    std::uninitialized_move_n(const_cast<iterator>(pos), std::distance(pos, cend()), new_data.GetAddress() + std::distance(cbegin(), pos) + 1);
                } else {
                    std::uninitialized_copy_n(pos, std::distance(pos, cend()), new_data.GetAddress() + std::distance(cbegin(), pos) + 1);
                }
            } catch (...) {
                std::destroy_n(new_data.GetAddress(), std::distance(cbegin(), pos));
                result->~T();
                throw;
            }
            std::destroy_n(begin(), size_);
            data_.Swap(new_data);
        } else {
            if (pos == cend()) {
                result = new (data_.GetAddress() + size_) T(std::forward<Args>(args)...);
            } else {
                result = const_cast<iterator>(pos);
                T temp(std::forward<Args>(args)...);
                if (size_ > 0) {
                    new (end()) T(std::move(*(end() - 1)));
                    if (size_ > 1) {
                        std::move_backward(result, end() - 1, end());
                    }
                }
                *result = std::move(temp);
            }          
        }
        ++size_;
        return result;
    }
};

