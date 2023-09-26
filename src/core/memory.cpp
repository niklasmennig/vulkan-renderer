#include "memory.h"

size_t memory::align_up(size_t size, size_t alignment) {
    return (size + (alignment - 1)) & ~(alignment - 1);
}