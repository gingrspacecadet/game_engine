#include "engine.hpp"
#include <iostream> // For debugging/logging
#include <wayland-client.h> // For Wayland event polling

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

void Engine::main_loop() {
    while (true) {
        int dispatchResult = wl_display_dispatch(vkContext.get_display());
        if (dispatchResult == -1) {
            std::cerr << "[Engine] Wayland event dispatch failed. Exiting main loop.\n";
            break;
        }

        try {
            vkContext.draw_frame();
        } catch (const std::exception& e) {
            std::cerr << "[Engine] Error during frame rendering: " << e.what() << "\n";
            break;
        }
    }
}

void Engine::run() {
    // Placeholder for the main engine loop
    std::cout << "Engine is running..." << std::endl;
    initialize();
    main_loop();
}
