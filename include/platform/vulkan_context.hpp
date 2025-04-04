#pragma once

#include <vulkan/vulkan.h>
#include <wayland-client.h>
#include <xdg-shell-client-protocol.h>
#include <string>
#include <vector>

class VulkanContext {
public:
    VulkanContext(wl_display* display, wl_surface* surface);
    ~VulkanContext();

    void draw_frame();
    void create_swapchain();
    void create_render_pass();       // Moved to public
    void create_framebuffers();      // Moved to public
    void create_command_pool();      // Moved to public
    void create_command_buffers();   // Moved to public
    void create_sync_objects();      // Moved to public
    void process_wayland_events();   // Move this method to the public section
    wl_display* get_display() const; // Add this method

    wl_compositor* waylandCompositor; // Ensure this is accessible

private:
    void init_instance();
    void pick_physical_device();
    void create_logical_device();
    void create_surface(wl_display* display, wl_surface* surface);
    void record_command_buffer(VkCommandBuffer cmdBuffer, uint32_t imageIndex);

    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSurfaceKHR vkSurface;

    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;

    VkRenderPass renderPass;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkFramebuffer> swapchainFramebuffers; // Add this member
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    wl_display* waylandDisplay; // Store Wayland display
};
