//> includes
#include "VkBootstrap.h"
#include "vk_engine.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_initializers.h>
#include <vk_types.h>

#include <chrono>
#include <thread>

VulkanEngine* loadedEngine = nullptr;
constexpr bool bUseValidationLayers = false;

VulkanEngine& VulkanEngine::Get() { return *loadedEngine; }

/***********/
/* FLOW: SDL -> Instance -> Surface -> Device -> Swapchain */
/***********/

void VulkanEngine::init()
{
	// only one engine initialization is allowed with the application.
	assert(loadedEngine == nullptr);
	loadedEngine = this;

	// We initialize SDL and create a window with it.
	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

	_window = SDL_CreateWindow(
		"Vulkan Engine Learning",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		_windowExtent.width,
		_windowExtent.height,
		window_flags);

	// initialize vulkan functions
	init_vulkan();
	init_swapchain();
	init_commands();
	init_sync_structures();

	// everything went fine
	_isInitialized = true;
}

void VulkanEngine::cleanup()
{
	if (_isInitialized) {
		destroy_swapchain();

		vkDestroySurfaceKHR(_instance, _surface, nullptr);
		vkDestroyDevice(_device, nullptr);

		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger, nullptr);
		vkDestroyInstance(_instance, nullptr);

		SDL_DestroyWindow(_window);
	}

	// clear engine pointer
	loadedEngine = nullptr;
}

void VulkanEngine::draw()
{
	// nothing yet
}

void VulkanEngine::run()
{
	SDL_Event e;
	bool bQuit = false;

	// main loop
	while (!bQuit) {
		// Handle events on queue
		while (SDL_PollEvent(&e) != 0) {
			// close the window when user alt-f4s or clicks the X button
			if (e.type == SDL_QUIT)
				bQuit = true;

			if (e.type == SDL_WINDOWEVENT) {
				if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
					stop_rendering = true;
				}
				if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
					stop_rendering = false;
				}
			}
		}

		// do not draw if we are minimized
		if (stop_rendering) {
			// throttle the speed to avoid the endless spinning
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		draw();
	}
}

/**************/
/* VULKAN */
/**************/
void VulkanEngine::init_vulkan()
{
	// create a vulkan instance through VkBoostrap
	vkb::InstanceBuilder builder{};
	builder.set_app_name("Vulkan Anjay App");
	builder.require_api_version(1, 3, 0);
	builder.use_default_debug_messenger(); // tell the vulkan to catch the debug message from validatiton layers
	builder.request_validation_layers(bUseValidationLayers); // turn validation layers on and off
	auto builder_result = builder.build();

	vkb::Instance vkb_instance = builder_result.value();

	_instance = vkb_instance.instance;
	_debug_messenger = vkb_instance.debug_messenger;

	// create vulkan surface
	// surface is "instance extension" which functionality is to abstract the way window object is handle by vulkan
	SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

	// vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features{};
	features.dynamicRendering = true; // skip renderpass/framebuffer
	features.synchronization2 = true; // upgraded version of synchronization

	// vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features2{};
	features2.bufferDeviceAddress = true; // use GPU pointer without binding buffers
	features2.descriptorIndexing = true; // bindless textures

	// vkboostrap select physical device
	vkb::PhysicalDeviceSelector selector{ vkb_instance }; //select GPU
	vkb::PhysicalDevice physical_device = selector.set_minimum_version(1, 3)
		.set_required_features_13(features)
		.set_required_features_12(features2)
		.set_surface(_surface)
		.select().value();

	// vkboostrap logical device(?)
	vkb::DeviceBuilder device_builder(physical_device);
	vkb::Device vkbDevice = device_builder.build().value();

	_device = vkbDevice.device;
	_chosen_device = vkbDevice.physical_device;
}

void VulkanEngine::init_swapchain()
{
	create_swapchain(_windowExtent.width, _windowExtent.height);
}

void VulkanEngine::init_commands()
{
}

void VulkanEngine::init_sync_structures()
{
}

void VulkanEngine::create_swapchain(uint32_t width, uint32_t height)
{
	vkb::SwapchainBuilder builder{ _chosen_device, _device, _surface };

	_swap_chain_image_format = VK_FORMAT_B8G8R8A8_UNORM;

	vkb::Swapchain vkbSwapChain = builder
		.set_desired_format(VkSurfaceFormatKHR{ .format = _swap_chain_image_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // hard VSync. Limit the FPS to speed up the monitor
		.set_desired_extent(width, height) // the size of the window where images will be drawn
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	_swap_chain_extent = vkbSwapChain.extent;
	_swapchain = vkbSwapChain.swapchain;
	_swap_chain_images = vkbSwapChain.get_images().value();
	_swap_chain_image_views = vkbSwapChain.get_image_views().value();
}

void VulkanEngine::destroy_swapchain()
{
	vkDestroySwapchainKHR(_device, _swapchain, nullptr);

	for (int i = 0; i < _swap_chain_image_views.size(); i++)
	{
		vkDestroyImageView(_device, _swap_chain_image_views[i], nullptr);
	}
}
