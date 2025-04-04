#include "shm_renderer.hpp"
#include <wayland-client.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

ShmRenderer::ShmRenderer(wl_display* display, wl_surface* surface)
    : display(display), surface(surface), buffer(nullptr), width(800), height(600) {
    if (!::shm) {
        std::cerr << "[ShmRenderer] Global wl_shm pointer is null." << std::endl;
        throw std::runtime_error("Failed to bind wl_shm interface.");
    }
    shm = ::shm; // Use the global wl_shm pointer
    std::cout << "[ShmRenderer] wl_shm interface successfully assigned." << std::endl;
}

ShmRenderer::~ShmRenderer() {
    if (buffer) {
        wl_buffer_destroy(buffer);
    }
}

void ShmRenderer::create_buffer(uint32_t color) {
    int stride = width * 4;
    int size = stride * height;

    int fd = memfd_create("shm_buffer", MFD_CLOEXEC);
    if (fd < 0) {
        throw std::runtime_error("Failed to create shared memory file.");
    }

    if (ftruncate(fd, size) < 0) {
        close(fd);
        throw std::runtime_error("Failed to set size of shared memory file.");
    }

    void* data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        throw std::runtime_error("Failed to map shared memory.");
    }

    uint32_t* pixel = static_cast<uint32_t*>(data);
    for (int i = 0; i < width * height; ++i) {
        pixel[i] = color;
    }

    munmap(data, size);

    buffer = create_shm_buffer(shm, width, height, color);
    if (!buffer) {
        close(fd);
        throw std::runtime_error("Failed to create wl_buffer.");
    }

    close(fd);
}

void ShmRenderer::draw_background(uint32_t color) {
    if (!buffer) {
        create_buffer(color);
    }

    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_damage(surface, 0, 0, width, height);
}

wl_buffer* create_shm_buffer(wl_shm* shm, int width, int height, uint32_t color) {
    int stride = width * 4; // 4 bytes per pixel (RGBA)
    int size = stride * height;

    int fd = memfd_create("shm_buffer", MFD_CLOEXEC);
    if (fd < 0) {
        throw std::runtime_error("Failed to create shared memory file.");
    }

    if (ftruncate(fd, size) < 0) {
        close(fd);
        throw std::runtime_error("Failed to set size of shared memory file.");
    }

    void* data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        throw std::runtime_error("Failed to map shared memory.");
    }

    uint32_t* pixel = static_cast<uint32_t*>(data);
    for (int i = 0; i < width * height; ++i) {
        pixel[i] = color;
    }

    munmap(data, size);

    wl_shm_pool* pool = wl_shm_create_pool(shm, fd, size);
    wl_buffer* buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);

    close(fd);
    return buffer;
}
