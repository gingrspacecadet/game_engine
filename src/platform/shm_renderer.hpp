#pragma once
#include <wayland-client.h>

wl_buffer *create_shm_buffer(wl_shm *shm, int width, int height, uint32_t color);
