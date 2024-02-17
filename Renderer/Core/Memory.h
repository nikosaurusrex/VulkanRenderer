#ifndef MEMORY_H
#define MEMORY_H

#include "../Common.h"

template <typename T>
struct Magalloc {
    typedef T value_type;
    
    Magalloc() = default; 

    T *allocate(size_t n) {
        return static_cast<T *>(alloc(n * sizeof(T)));
    }

    void deallocate(T *ptr) {
        dealloc(static_cast<void *>(ptr));
    }

    void *alloc(size_t bytes) {
        void *ptr = malloc(bytes);

        if (!ptr) {
            LogFatal("Failed to allocate %d bytes of memory", bytes * sizeof(T));
        }
        return ptr;
    }

    void dealloc(void *ptr) {
        free(ptr);
    }
};

extern Magalloc<u8> galloc;

#endif