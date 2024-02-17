#include "Memory.h"

Magalloc<u8> galloc = Magalloc<u8>();

void* operator new(size_t size) {
    return galloc.alloc(size);
}

void operator delete(void* ptr) {
    galloc.dealloc(ptr);
}