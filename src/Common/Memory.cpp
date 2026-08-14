#include <cstdlib>
#include <new>

// The ESP32 no-RTTI libstdc++ nothrow overload delegates to throwing new.
// At very low heap, allocating std::bad_alloc can itself fail and abort before
// callers such as AsyncTCP get the nullptr they are explicitly prepared for.
void *operator new(std::size_t size, const std::nothrow_t &) noexcept
{
    return std::malloc(size == 0 ? 1 : size);
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept
{
    return std::malloc(size == 0 ? 1 : size);
}
