#pragma once
#include <wayland-client.h>

extern wl_shm* shm; // Declare the global wl_shm pointer as extern

wl_buffer *create_shm_buffer(wl_shm *shm, int width, int height, uint32_t color);

class ShmRenderer {
public:
    ShmRenderer(wl_display* display, wl_surface* surface);
    ~ShmRenderer();

    void draw_background(uint32_t color);

private:
    wl_display* display;
    wl_surface* surface;
    wl_shm* shm;
    wl_buffer* buffer;
    int width;
    int height;

    void create_buffer(uint32_t color);
};
