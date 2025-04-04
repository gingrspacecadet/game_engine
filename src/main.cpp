#include <wayland-client.h>
#include "protocols/xdg-shell-client-protocol.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include "platform/shm_renderer.hpp"
#include <chrono>
#include <bits/this_thread_sleep.h>

// Global variables for Wayland objects
wl_compositor* compositor = nullptr;
xdg_wm_base* wm_base = nullptr;
wl_shm* shm = nullptr;
wl_seat* seat = nullptr;
wl_keyboard* keyboard = nullptr;
wl_pointer* pointer = nullptr;

bool configured = false;
bool running = true;

void xdg_surface_configure_handler(void* /*data*/, xdg_surface* surface, uint32_t serial) {
    xdg_surface_ack_configure(surface, serial);
    configured = true;
}

static const xdg_surface_listener xdgSurfaceListener = {
    .configure = xdg_surface_configure_handler
};

void handle_key_event(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
    std::cout << "Key event: key=" << key << ", state=" << (state == WL_KEYBOARD_KEY_STATE_PRESSED ? "PRESSED" : "RELEASED") << std::endl;

    if (key == 1 && state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        std::cout << "ESC pressed, exiting.\n";
        running = false;
    }
}

static const wl_pointer_listener pointer_listener = {
    .enter = [](void*, wl_pointer*, uint32_t, wl_surface*, wl_fixed_t, wl_fixed_t) {
        std::cout << "[DEBUG] Pointer entered a surface." << std::endl;
    },
    .leave = [](void*, wl_pointer*, uint32_t, wl_surface*) {
        std::cout << "[DEBUG] Pointer left a surface." << std::endl;
    },
    .motion = [](void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y) {
        std::cout << "[DEBUG] Pointer motion event: time=" << time << ", x=" << wl_fixed_to_double(x) << ", y=" << wl_fixed_to_double(y) << std::endl;
    },
    .button = [](void*data, wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
        std::cout << "[DEBUG] Pointer button event: serial=" << serial << ", time=" << time << ", button=" << button << ", state=" << state << std::endl;
    },
    .axis = [](void* data, wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {
        std::cout << "[DEBUG] Pointer axis event: time=" << time << ", axis=" << axis << ", value=" << wl_fixed_to_double(value) << std::endl;
    },
    .frame = [](void* data, wl_pointer* pointer) {
        std::cout << "[DEBUG] Pointer frame event received." << std::endl;
    },
    .axis_source = [](void* data, wl_pointer* pointer, uint32_t axis_source) {
        std::cout << "[DEBUG] Pointer axis source event: axis_source=" << axis_source << std::endl;
    },
    .axis_stop = [](void* data, wl_pointer* pointer, uint32_t time, uint32_t axis) {
        std::cout << "[DEBUG] Pointer axis stop event: time=" << time << ", axis=" << axis << std::endl;
    },
    .axis_discrete = [](void* data, wl_pointer* pointer, uint32_t axis, int32_t discrete) {
        std::cout << "[DEBUG] Pointer axis discrete event: axis=" << axis << ", discrete=" << discrete << std::endl;
    }
};

void handle_seat_capabilities(void* data, wl_seat* seat, uint32_t caps) {
    std::cout << "[DEBUG] Seat capabilities: caps=" << caps << std::endl;

    if (!seat) {
        std::cerr << "[ERROR] wl_seat is null!" << std::endl;
        return;
    }

    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
        std::cout << "[DEBUG] Keyboard capability detected." << std::endl;
        keyboard = wl_seat_get_keyboard(seat);
        static const wl_keyboard_listener keyboard_listener = {
            .keymap = [](void*, wl_keyboard*, uint32_t, int, uint32_t) {},
            .enter = [](void*, wl_keyboard*, uint32_t, wl_surface*, wl_array*) {},
            .leave = [](void*, wl_keyboard*, uint32_t, wl_surface*) {},
            .key = handle_key_event,
            .modifiers = [](void*, wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {},
            .repeat_info = [](void*, wl_keyboard*, int32_t, int32_t) {}
        };
        wl_keyboard_add_listener(keyboard, &keyboard_listener, nullptr);
    }

    if (caps & WL_SEAT_CAPABILITY_POINTER) {
        std::cout << "[DEBUG] Pointer capability detected." << std::endl;
        pointer = wl_seat_get_pointer(seat);
        if (pointer) {
            wl_pointer_add_listener(pointer, &pointer_listener, nullptr); // Add listener here
        } else {
            std::cerr << "[ERROR] Failed to get pointer from seat." << std::endl;
        }
    }
}

void handle_seat_name(void* data, wl_seat* seat, const char* name) {
    std::cout << "Seat name: " << name << std::endl;
}

static const wl_seat_listener seat_listener = {
    .capabilities = handle_seat_capabilities,
    .name = handle_seat_name
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
    } else if (strcmp(interface, "wl_seat") == 0) {
        seat = static_cast<wl_seat*>(wl_registry_bind(registry, id, &wl_seat_interface, version));
        wl_seat_add_listener(seat, &seat_listener, nullptr);
        std::cout << "Bound wl_seat interface." << std::endl;
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
    static const wl_registry_listener registryListener = { registry_handler, registry_remover };
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
    shmRenderer.draw_background(0xFF0000FF);

    wl_surface_commit(surface);
    wl_display_flush(display);
    std::cout << "Committed Wayland surface." << std::endl;

    while (!configured) {
        std::cout << "Waiting for configure event..." << std::endl;
        wl_display_dispatch(display);
    }
    std::cout << "Configure event received." << std::endl;

    wl_surface_commit(surface);
    wl_display_flush(display);
    std::cout << "Re-committed Wayland surface after configure and flushed display." << std::endl;

    using clock = std::chrono::high_resolution_clock;
    auto frame_duration = std::chrono::milliseconds(1000 / 60);

    while (running) {
        int ret = wl_display_dispatch(display);
        if (ret == -1) break;

        auto frame_start = clock::now();

        shmRenderer.draw_background(0xFF0000FF);
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
