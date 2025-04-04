#include "engine.hpp"
#include "platform/vulkan_context.hpp"
#include "protocols/xdg-shell-client-protocol.h"
#include "platform/shm_renderer.hpp"
#include <wayland-client.h>
#include <iostream>
#include <unistd.h> // Include this for the close() function

// Global Wayland objects
wl_display *display = nullptr;
wl_registry *registry = nullptr;
wl_compositor *compositor = nullptr;
wl_surface *surface = nullptr;
wl_seat *seat = nullptr;
wl_pointer *pointer = nullptr;
wl_keyboard *keyboard = nullptr;

wl_shm *shm = nullptr;
xdg_wm_base *wm_base = nullptr;
xdg_surface *xdgSurf = nullptr;
xdg_toplevel *xdgTop = nullptr;

void registry_handler(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
    if (std::string(interface) == "wl_compositor") {
        compositor = static_cast<wl_compositor*>(
            wl_registry_bind(registry, id, &wl_compositor_interface, std::min(version, 4u))
        );
    } else if (std::string(interface) == "wl_shm") {
        shm = static_cast<wl_shm*>(
            wl_registry_bind(registry, id, &wl_shm_interface, std::min(version, 1u))
        );
    } else if (std::string(interface) == "xdg_wm_base") {
        wm_base = static_cast<xdg_wm_base*>(
            wl_registry_bind(registry, id, &xdg_wm_base_interface, std::min(version, 1u))
        );
    } else if (std::string(interface) == "wl_seat") {
        seat = static_cast<wl_seat*>(
            wl_registry_bind(registry, id, &wl_seat_interface, std::min(version, 5u))
        );
    }
}

void handle_ping(void *data, xdg_wm_base *wm, uint32_t serial) {
    xdg_wm_base_pong(wm, serial);
}

const xdg_wm_base_listener wm_base_listener = {
    .ping = handle_ping
};

void registry_remover(void *data, wl_registry *registry, uint32_t id) {}

void pointer_enter(void*, wl_pointer*, uint32_t, wl_surface*, wl_fixed_t, wl_fixed_t) {}
void pointer_leave(void*, wl_pointer*, uint32_t, wl_surface*) {}

void pointer_motion(void*, wl_pointer*, uint32_t time, wl_fixed_t x, wl_fixed_t y) {
    std::cout << "Mouse moved: (" << wl_fixed_to_double(x) << ", " << wl_fixed_to_double(y) << ")\n";
}

void pointer_button(void*, wl_pointer*, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
    std::cout << "Mouse button " << button << (state == WL_POINTER_BUTTON_STATE_PRESSED ? " pressed\n" : " released\n");
}

void pointer_axis(void*, wl_pointer*, uint32_t, uint32_t, wl_fixed_t) {}

static const wl_pointer_listener pointer_listener = {
    pointer_enter, pointer_leave, pointer_motion, pointer_button, pointer_axis
};

void keyboard_keymap(void *data, wl_keyboard *keyboard, uint32_t format, int fd, uint32_t size) {
    std::cout << "Keymap received: format=" << format << ", size=" << size << "\n";
    // Close the file descriptor as it's no longer needed
    close(fd);
}

void keyboard_enter(void*, wl_keyboard*, uint32_t, wl_surface*, wl_array*) {}
void keyboard_leave(void*, wl_keyboard*, uint32_t, wl_surface*) {}
void keyboard_key(void*, wl_keyboard*, uint32_t, uint32_t time, uint32_t key, uint32_t state) {
    std::cout << "Key " << key << (state == WL_KEYBOARD_KEY_STATE_PRESSED ? " pressed\n" : " released\n");
}
void keyboard_modifiers(void *data, wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
    std::cout << "Keyboard modifiers changed: "
              << "depressed=" << mods_depressed
              << ", latched=" << mods_latched
              << ", locked=" << mods_locked
              << ", group=" << group << "\n";
}

static const wl_keyboard_listener keyboard_listener = {
    keyboard_keymap, 
    keyboard_enter, 
    keyboard_leave, 
    keyboard_key, 
    keyboard_modifiers
};

int main() {
    // Wayland setup
    wl_display* display = wl_display_connect(nullptr);
    if (!display) {
        std::cerr << "Failed to connect to Wayland display!" << std::endl;
        return -1;
    }

    wl_registry* registry = wl_display_get_registry(display);
    static const wl_registry_listener registry_listener = {registry_handler, registry_remover};
    wl_registry_add_listener(registry, &registry_listener, nullptr);
    wl_display_roundtrip(display);

    if (!compositor || !wm_base) {
        std::cerr << "Required Wayland interfaces not available!" << std::endl;
        wl_display_disconnect(display);
        return -1;
    }

    wl_surface* surface = wl_compositor_create_surface(compositor);
    if (!surface) {
        std::cerr << "Failed to create surface!" << std::endl;
        wl_display_disconnect(display);
        return -1;
    }

    xdg_surface* xdgSurf = xdg_wm_base_get_xdg_surface(wm_base, surface);
    if (!xdgSurf) {
        std::cerr << "Failed to create xdg_surface!" << std::endl;
        wl_surface_destroy(surface);
        wl_display_disconnect(display);
        return -1;
    }

    xdg_toplevel* xdgTop = xdg_surface_get_toplevel(xdgSurf);
    if (!xdgTop) {
        std::cerr << "Failed to create xdg_toplevel!" << std::endl;
        xdg_surface_destroy(xdgSurf);
        wl_surface_destroy(surface);
        wl_display_disconnect(display);
        return -1;
    }

    wl_surface_commit(surface);
    wl_display_roundtrip(display);

    // Start the engine
    try {
        Engine engine(display, surface);
        while (true) {
            engine.vkContext.process_wayland_events();
            engine.run();
        }
    } catch (const std::exception& e) {
        std::cerr << "[Main] Exception: " << e.what() << std::endl;
    }

    // Cleanup
    xdg_toplevel_destroy(xdgTop);
    xdg_surface_destroy(xdgSurf);
    wl_surface_destroy(surface);

    if (pointer) wl_pointer_destroy(pointer);
    if (keyboard) wl_keyboard_destroy(keyboard);
    if (seat) wl_seat_destroy(seat);
    if (shm) wl_shm_destroy(shm);
    if (compositor) wl_compositor_destroy(compositor);
    if (wm_base) xdg_wm_base_destroy(wm_base);

    wl_display_roundtrip(display); // Ensure all requests are processed
    wl_display_disconnect(display);
    return 0;
}
