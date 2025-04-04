#include <wayland-client.h> // Include Wayland headers
#include "platform/vulkan_context.hpp" // Include VulkanContext

class Engine {
public:
    Engine(wl_display* display, wl_surface* surface); // Correct constructor declaration
    ~Engine(); // Add destructor declaration
    void run();
    VulkanContext vkContext; // Ensure this is accessible

private:
    void initialize(); // Add initialize method declaration
    void main_loop();  // Add main_loop method declaration
    void render_frame(); // Add render_frame method declaration
};
