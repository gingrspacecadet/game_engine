#pragma once

#include "platform/vulkan_context.hpp"

class Engine {
public:
    Engine(wl_display* display, wl_surface* surface);
    ~Engine();

    void run();

private:
    VulkanContext vkContext;
    void initialize();
    void main_loop();
};
