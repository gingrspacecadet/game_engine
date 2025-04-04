#include <wayland-client.h>
#include "protocols/xdg-shell-client-protocol.h"
#include <iostream>
#include <cstring> // For strcmp
#include <unistd.h>
#include "platform/shm_renderer.hpp" // Include ShmRenderer header
#include <chrono> // For frame timing
#include <bits/this_thread_sleep.h>

wl_compositor* compositor = nullptr;
xdg_wm_base* wm_base = nullptr;
wl_shm* shm = nullptr;

bool configured = false;

void xdg_surface_configure_handler(void* /*data*/, xdg_surface* surface, uint32_t serial) {
    xdg_surface_ack_configure(surface, serial);
    configured = true;
}

static const xdg_surface_listener xdgSurfaceListener = {
    .configure = xdg_surface_configure_handler
};

void registry_handler(void* data, wl_registry* registry, uint32_t id, const char* interface, uint32_t version) {
    if (strcmp(interface, "wl_compositor") == 0) {
        compositor = static_cast<wl_compositor*>(wl_registry_bind(registry, id, &wl_compositor_interface, 4));
        std::cout << "Bound wl_compositor interface." << std::endl;
    } else if (strcmp(interface, "xdg_wm_base") == 0) {
        wm_base = static_cast<xdg_wm_base*>(wl_registry_bind(registry, id, &xdg_wm_base_interface, 1));
        std::cout << "Bound xdg_wm_base interface." << std::endl;
    } else if (strcmp(interface, "wl_shm") == 0) {
        shm = static_cast<wl_shm*>(wl_registry_bind(registry, id, &wl_shm_interface, 1));
        std::cout << "Bound wl_shm interface." << std::endl;
    } else {
        std::cout << "Unknown interface: " << interface << std::endl;
    }
}

void registry_remover(void* /*data*/, wl_registry* /*registry*/, uint32_t /*id*/) {}

int main() {
    std::cout << "Connecting to Wayland display..." << std::endl;
    wl_display* display = wl_display_connect(nullptr);
    if (!display) {
        std::cerr << "Failed to connect to Wayland display!" << std::endl;
        return -1;
    }
    std::cout << "Connected to Wayland display." << std::endl;

    wl_registry* registry = wl_display_get_registry(display);
    std::cout << "Obtained Wayland registry." << std::endl;
    static const wl_registry_listener registryListener = {registry_handler, registry_remover};
    wl_registry_add_listener(registry, &registryListener, nullptr);
    wl_display_roundtrip(display);
    std::cout << "Completed Wayland roundtrip." << std::endl;

    if (!compositor || !wm_base) {
        std::cerr << "Required interfaces not found!" << std::endl;
        return -1;
    }
    std::cout << "Wayland compositor and wm_base found." << std::endl;

    wl_surface* surface = wl_compositor_create_surface(compositor);
    if (!surface) {
        std::cerr << "Failed to create Wayland surface!" << std::endl;
        return -1;
    }
    std::cout << "Created Wayland surface." << std::endl;

    xdg_surface* xdgSurf = xdg_wm_base_get_xdg_surface(wm_base, surface);
    if (!xdgSurf) {
        std::cerr << "Failed to create xdg_surface!" << std::endl;
        return -1;
    }
    std::cout << "Created xdg_surface." << std::endl;

    xdg_surface_add_listener(xdgSurf, &xdgSurfaceListener, nullptr);
    std::cout << "Added xdg_surface listener." << std::endl;

    xdg_toplevel* toplevel = xdg_surface_get_toplevel(xdgSurf);
    if (!toplevel) {
        std::cerr << "Failed to create xdg_toplevel!" << std::endl;
        return -1;
    }
    std::cout << "Created xdg_toplevel." << std::endl;

    xdg_toplevel_set_title(toplevel, "Test Window");
    std::cout << "Set xdg_toplevel title." << std::endl;
    xdg_toplevel_set_app_id(toplevel, "test.app");
    xdg_toplevel_set_min_size(toplevel, 800, 600);

    // Create SHM renderer for basic background
    ShmRenderer shmRenderer(display, surface);
    shmRenderer.draw_background(0xFF0000FF); // Draw a blue background

    wl_surface_commit(surface);
    wl_display_flush(display);
    std::cout << "Committed Wayland surface." << std::endl;

    // Wait for configure
    while (!configured) {
        std::cout << "Waiting for configure event..." << std::endl;
        wl_display_dispatch(display);
    }
    std::cout << "Configure event received." << std::endl;

    wl_surface_commit(surface);
    wl_display_flush(display);
    std::cout << "Re-committed Wayland surface after configure and flushed display." << std::endl;

    // Main loop
    using clock = std::chrono::high_resolution_clock;
    auto frame_duration = std::chrono::milliseconds(1000 / 60); // 60 FPS

    while (wl_display_dispatch(display) != -1) {
        auto frame_start = clock::now();

        std::cout << "Processing Wayland events..." << std::endl;
        shmRenderer.draw_background(0xFF0000FF); // Keep drawing the background
        wl_surface_commit(surface);
        wl_display_flush(display);

        auto frame_end = clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start);
        if (elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - elapsed);
        }
    }
    std::cout << "Exiting main loop." << std::endl;

    return 0;
}
