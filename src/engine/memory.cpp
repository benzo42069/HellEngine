#include <engine/memory.h>

#include <stdexcept>

namespace engine {

FrameAllocator::FrameAllocator(const std::size_t capacityBytes)
    : buffer_(capacityBytes, 0) {}

void* FrameAllocator::allocate(const std::size_t size, const std::size_t alignment) {
    const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(buffer_.data());
    const std::uintptr_t current = base + offset_;
    const std::size_t mask = alignment - 1;
    const std::uintptr_t aligned = (current + mask) & ~static_cast<std::uintptr_t>(mask);
    const std::size_t next = static_cast<std::size_t>((aligned - base) + size);

    if (next > buffer_.size()) {
        throw std::bad_alloc();
    }

    offset_ = next;
    return reinterpret_cast<void*>(aligned);
}

void FrameAllocator::reset() {
    offset_ = 0;
}

} // namespace engine
