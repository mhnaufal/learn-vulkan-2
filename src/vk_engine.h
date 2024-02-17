// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>

class VulkanEngine {
public:
	// sdl
	bool _isInitialized{ false };
	int _frameNumber {0};
	bool stop_rendering{ false };
	VkExtent2D _windowExtent{ 1700 , 900 };
	struct SDL_Window* _window{ nullptr };

	// vulkan instance & device
	VkInstance _instance{};
	VkDebugUtilsMessengerEXT _debug_messenger{};
	VkPhysicalDevice _chosen_device{};
	VkDevice _device{};
	VkSurfaceKHR _surface{};

	// vulkan swapchain
	VkSwapchainKHR _swapchain{};
	VkFormat _swap_chain_image_format{};
	std::vector<VkImage> _swap_chain_images{}; // actual image object
	std::vector<VkImageView> _swap_chain_image_views{}; // image wrapper, use for swapping the color
	VkExtent2D _swap_chain_extent{};

	static VulkanEngine& Get();

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();

private:
	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();
	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();
};
