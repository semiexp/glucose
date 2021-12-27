#pragma once

#include <cstdio>

using gzFile = FILE*;

inline size_t gzread(gzFile fp, void* buf, size_t buf_size) {
    return fread(buf, 1, buf_size, fp);
}

inline gzFile gzopen(const char* filename, const char* mode) {
    return fopen(filename, mode);
}

inline gzFile gzdopen(int fd, const char* mode) {
    return fdopen(fd, mode);
}

inline void gzclose(gzFile fp) {
    fclose(fp);
}
