#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace engine {

class FrameAllocator {
  public:
    explicit FrameAllocator(std::size_t capacityBytes);

    void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t));
    void reset();

    [[nodiscard]] std::size_t used() const { return offset_; }
    [[nodiscard]] std::size_t capacity() const { return buffer_.size(); }

  private:
    std::vector<std::uint8_t> buffer_;
    std::size_t offset_ {0};
};

template <typename T>
class ObjectPool {
  public:
    explicit ObjectPool(std::size_t capacity)
        : storage_(std::make_unique<std::aligned_storage_t<sizeof(T), alignof(T)>[]>(capacity)),
          alive_(capacity, false) {
        freeList_.reserve(capacity);
        for (std::size_t i = 0; i < capacity; ++i) {
            freeList_.push_back(capacity - 1 - i);
        }
    }

    ~ObjectPool() {
        for (std::size_t i = 0; i < alive_.size(); ++i) {
            if (alive_[i]) {
                std::destroy_at(ptrAt(i));
            }
        }
    }

    template <typename... Args>
    T* create(Args&&... args) {
        if (freeList_.empty()) {
            return nullptr;
        }

        const std::size_t index = freeList_.back();
        freeList_.pop_back();

        T* object = ptrAt(index);
        std::construct_at(object, std::forward<Args>(args)...);
        alive_[index] = true;
        return object;
    }

    void destroy(T* object) {
        if (!object) {
            return;
        }

        const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(storage_.get());
        const std::uintptr_t current = reinterpret_cast<std::uintptr_t>(object);
        const std::size_t stride = sizeof(std::aligned_storage_t<sizeof(T), alignof(T)>);
        const std::size_t index = (current - base) / stride;

        if (index >= alive_.size() || !alive_[index]) {
            return;
        }

        std::destroy_at(object);
        alive_[index] = false;
        freeList_.push_back(index);
    }

    [[nodiscard]] std::size_t capacity() const { return alive_.size(); }
    [[nodiscard]] std::size_t aliveCount() const { return alive_.size() - freeList_.size(); }

  private:
    T* ptrAt(const std::size_t index) {
        return reinterpret_cast<T*>(&storage_[index]);
    }

    std::unique_ptr<std::aligned_storage_t<sizeof(T), alignof(T)>[]> storage_;
    std::vector<bool> alive_;
    std::vector<std::size_t> freeList_;
};

} // namespace engine
