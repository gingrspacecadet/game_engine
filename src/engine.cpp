#include "engine.hpp"
#include <iostream> // For debugging/logging
#include <wayland-client.h> // For Wayland event polling
#include <unistd.h> // For usleep

Engine::Engine(wl_display* display, wl_surface* surface)
    : vkContext(display, surface) { // Initialize VulkanContext with arguments
    std::cout << "Engine initialized with Wayland display and surface." << std::endl;
}

Engine::~Engine() {}

void Engine::initialize() {
    // Perform any necessary setup or resource loading here.
    std::cout << "[Engine] Initialization complete.\n";
    vkContext.create_swapchain();
    vkContext.create_render_pass();
    vkContext.create_framebuffers();
    vkContext.create_command_pool();
    vkContext.create_command_buffers();
    vkContext.create_sync_objects();
}

void Engine::render_frame() {
    try {
        vkContext.draw_frame(); // Render a frame using Vulkan
    } catch (const std::exception& e) {
        std::cerr << "[Engine] Error during frame rendering: " << e.what() << "\n";
        throw;
    }
}

void Engine::main_loop() {
    while (true) {
        int dispatchResult = wl_display_dispatch(vkContext.get_display());
        if (dispatchResult == -1) {
            perror("[Engine] Wayland event dispatch failed");
            break;
        }

        render_frame(); // Render a frame

        // Commit the surface after rendering
        wl_surface_commit(vkContext.get_surface());

        // Flush the Wayland display to ensure events are sent
        wl_display_flush(vkContext.get_display());

        // Optional: Sleep to limit frame rate (e.g., ~60fps)
        usleep(16000);
    }
}

void Engine::run() {
    // Placeholder for the main engine loop
    std::cout << "Engine is running..." << std::endl;
    initialize();
    main_loop();
}
