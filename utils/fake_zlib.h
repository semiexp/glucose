#pragma once

#include <cstdlib>

using gzFile = void*;

inline size_t gzread(gzFile fp, void* buf, size_t buf_size) {
    abort();
}

inline gzFile gzopen(const char* filename, const char* mode) {
    abort();
}

inline gzFile gzdopen(int fd, const char* mode) {
    abort();
}

inline void gzclose(gzFile fp) {
    abort();
}
