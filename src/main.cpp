#include <wayland-client.h>
#include "protocols/xdg-shell-client-protocol.h"
#include <iostream>
#include <cstring> // For strcmp
#include <unistd.h>

wl_compositor* compositor = nullptr;
xdg_wm_base* wm_base = nullptr;

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
    } else if (strcmp(interface, "xdg_wm_base") == 0) {
        wm_base = static_cast<xdg_wm_base*>(wl_registry_bind(registry, id, &xdg_wm_base_interface, 1));
    }
}

void registry_remover(void* /*data*/, wl_registry* /*registry*/, uint32_t /*id*/) {}

int main() {
    wl_display* display = wl_display_connect(nullptr);
    if (!display) {
        std::cerr << "Failed to connect to Wayland display!" << std::endl;
        return -1;
    }

    wl_registry* registry = wl_display_get_registry(display);
    static const wl_registry_listener registryListener = {registry_handler, registry_remover};
    wl_registry_add_listener(registry, &registryListener, nullptr);
    wl_display_roundtrip(display);

    if (!compositor || !wm_base) {
        std::cerr << "Required interfaces not found!" << std::endl;
        return -1;
    }

    wl_surface* surface = wl_compositor_create_surface(compositor);
    xdg_surface* xdgSurf = xdg_wm_base_get_xdg_surface(wm_base, surface);
    xdg_surface_add_listener(xdgSurf, &xdgSurfaceListener, nullptr);
    xdg_toplevel* toplevel = xdg_surface_get_toplevel(xdgSurf);

    xdg_toplevel_set_title(toplevel, "Test Window");
    xdg_toplevel_set_app_id(toplevel, "test.app");
    xdg_toplevel_set_min_size(toplevel, 800, 600);

    wl_surface_commit(surface);

    // Wait for configure
    while (!configured)
        wl_display_dispatch(display);

    wl_surface_commit(surface);

    // Main loop
    while (wl_display_dispatch(display) != -1) {}

    return 0;
}
