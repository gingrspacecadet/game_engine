#include "shm_renderer.hpp"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

static int create_shm_file(off_t size) {
    const char *name = "/wl_shm_buffer";
    int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    shm_unlink(name); // we don't need it visible after opening

    if (fd < 0)
        return -1;

    if (ftruncate(fd, size) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

wl_buffer *create_shm_buffer(wl_shm *shm, int width, int height, uint32_t color) {
    const int stride = width * 4;
    const int size = stride * height;

    int fd = create_shm_file(size);
    if (fd < 0) {
        std::cerr << "Failed to create shared memory file!" << std::endl;
        return nullptr;
    }

    uint32_t *data = (uint32_t *)mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        return nullptr;
    }

    // Fill with color
    for (int i = 0; i < width * height; ++i)
        data[i] = color;

    wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
    wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);
    wl_shm_pool_destroy(pool);
    munmap(data, size);
    close(fd);

    return buffer;
}
