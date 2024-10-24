#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>


constexpr int width{ 800 };
constexpr int height{ 600 };

#ifdef NDEBUG
constexpr bool enableValidationLayers{ false };
#else
constexpr bool enableValidationLayers{ true };
#endif

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily{};
	std::optional<uint32_t> presentFamily{};

	bool isComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities{};
	std::vector<VkSurfaceFormatKHR> formats{};
	std::vector<VkPresentModeKHR> presentModes{};
};

class HelloTriangleApp
{
public:
	VkSurfaceKHR surface;

	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		[[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		[[maybe_unused]] void* pUserData)
	{
		std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

private:
	// TODO: read on learncpp if you should put "{}" here as in structs
	GLFWwindow* window;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;

	void initWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(width, height, "HelloTriangleApp", nullptr, nullptr);
	}

	void createInstance()
	{
		if (enableValidationLayers && !checkValidationLayersSupport())
			throw std::runtime_error("Validation layers requested, but not available!");

		constexpr VkApplicationInfo appInfo{
			.sType{ VK_STRUCTURE_TYPE_APPLICATION_INFO },
			.pApplicationName{ "HelloTriangle" },
			.applicationVersion{ VK_API_VERSION_1_0 },
			.pEngineName{ "Test" },
			.engineVersion{ VK_API_VERSION_1_0 },
			.apiVersion{ VK_API_VERSION_1_0 }
		};

		const std::vector<const char*> requiredExtensions{ getRequiredExtensions() };

		std::cout << "Required extensions:\n";
		for (const auto& ex : requiredExtensions)
			std::cout << '\t' << ex << '\n';

		std::cout << '\n';

		if (!checkExtensionsSupport(requiredExtensions))
			throw std::runtime_error("Not all the required extensions are supported!");

		VkInstanceCreateInfo createInfo{
			.sType{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO },
			.pNext{ nullptr },
			.pApplicationInfo{ &appInfo },
			.enabledLayerCount{ 0 },
			.enabledExtensionCount{ static_cast<uint32_t>(requiredExtensions.size()) },
			.ppEnabledExtensionNames{ requiredExtensions.data() }
		};

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
			throw std::runtime_error("Failed to create the instance!");
	}

	void setupDebugMessenger()
	{
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		populateDebugMessengerCreateInfo(createInfo);

		if (createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
			throw std::runtime_error("Failed to set up the debug messenger!");
	}

	void pickPhysicalDevice()
	{
		// TODO: is this necessary?
		physicalDevice = VK_NULL_HANDLE;

		uint32_t devicesCount{ 0 };
		vkEnumeratePhysicalDevices(instance, &devicesCount, nullptr);

		if (devicesCount == 0)
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");

		std::vector<VkPhysicalDevice> devices(devicesCount);
		vkEnumeratePhysicalDevices(instance, &devicesCount, devices.data());

		for (const auto& dev : devices)
		{
			if (isDeviceSuitable(dev))
			{
				physicalDevice = dev;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE)
			throw std::runtime_error("Failed to find a suitable GPU!");
	}

	void createLogicalDevice()
	{
		QueueFamilyIndices indices{ findQueueFamilies(physicalDevice) };

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		float queuePriority{ 1.0f };
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{
				.sType{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO },
				.queueFamilyIndex{ queueFamily },
				.queueCount{ 1 },
				.pQueuePriorities{ &queuePriority },
			};

			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};

		VkDeviceCreateInfo createInfo{
			.sType{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO },
			.queueCreateInfoCount{ static_cast<uint32_t>(queueCreateInfos.size()) },
			.pQueueCreateInfos{ queueCreateInfos.data() },
			.enabledLayerCount{ 0 },
			.enabledExtensionCount{ static_cast<uint32_t>(deviceExtensions.size()) },
			.ppEnabledExtensionNames{ deviceExtensions.data() },
			.pEnabledFeatures{ &deviceFeatures },
		};

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
			throw std::runtime_error("Failed to create the logical device!");

		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}

	void createSurface()
	{
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
			throw std::runtime_error("Failed to create the window surface!");
	}

	void initVulkan()
	{
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createGraphicsPipeline();
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
		}
	}

	void cleanup()
	{
		if (enableValidationLayers)
			destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

		// NOTE: is the reference appropriate here?
		for (const auto& imageView : swapChainImageViews)
		{
			vkDestroyImageView(device, imageView, nullptr);
		}

		vkDestroySwapchainKHR(device, swapChain, nullptr);
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}

	std::vector<const char*> getRequiredExtensions()
	{
		uint32_t glfwExtensionsCount{ 0 };
		const char** glfwExtensions{ glfwGetRequiredInstanceExtensions(&glfwExtensionsCount) };
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionsCount);

		if (enableValidationLayers)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		return extensions;
	}

	bool checkValidationLayersSupport()
	{
		// TODO: maybe use std::set with erase to perform this check?
		uint32_t layerCount{};
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers)
		{
			bool found{ false };

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					found = true;
					break;
				}
			}

			if (!found)
				return false;
		}

		return true;
	}

	bool checkExtensionsSupport(const std::vector<const char*>& glfwExtensions)
	{
		// TODO: maybe use std::set with erase to perform this check?
		uint32_t extensionsCount{ 0 };
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, availableExtensions.data());

		for (const char* extensionName : glfwExtensions)
		{
			bool found{ false };

			for (const auto& extension : availableExtensions)
			{
				if (strcmp(extensionName, extension.extensionName) == 0)
				{
					found = true;
					break;
				}
			}

			if (!found)
				return false;
		}

		return true;
	}

	bool checkDeviceExtensionsSupport(VkPhysicalDevice dev)
	{
		uint32_t extensionsCount{};
		vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionsCount, nullptr);
		
		std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
		vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionsCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	bool isDeviceSuitable(VkPhysicalDevice dev)
	{
		VkPhysicalDeviceProperties deviceProperties{};
		vkGetPhysicalDeviceProperties(dev, &deviceProperties);

		SwapChainSupportDetails swapChainSupport{ querySwapChainSupport(dev) };
		bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

		return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			&& findQueueFamilies(dev).isComplete()
			&& checkDeviceExtensionsSupport(dev)
			&& swapChainAdequate;
	}

	VkResult createDebugUtilsMessengerEXT(VkInstance inst,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* debugMesng)
	{
		auto func{ (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(inst, "vkCreateDebugUtilsMessengerEXT") };
		if (func != nullptr)
			return func(inst, pCreateInfo, pAllocator, debugMesng);
		else
			return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	void destroyDebugUtilsMessengerEXT(VkInstance inst,
		VkDebugUtilsMessengerEXT debugMesng,
		const VkAllocationCallbacks* pAllocator)
	{
		auto func{ (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(inst, "vkDestroyDebugUtilsMessengerEXT") };
		if (func != nullptr)
			return func(inst, debugMesng, pAllocator);
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice dev)
	{
		QueueFamilyIndices indices{};

		uint32_t queueFamilyCount{ 0 };
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());

		uint32_t index{ 0 };
		VkBool32 presentSupport{ false };
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.graphicsFamily = index;

			vkGetPhysicalDeviceSurfaceSupportKHR(dev, index, surface, &presentSupport);

			if (presentSupport)
				indices.presentFamily = index;

			if (indices.isComplete())
				break;

			++index;
		}

		return indices;
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice dev)
	{
		SwapChainSupportDetails details{};

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &details.capabilities);

		uint32_t formatsCount{};
		vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatsCount, nullptr);
		if (formatsCount != 0)
		{
			details.formats.resize(formatsCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatsCount, details.formats.data());
		}

		uint32_t presentModesCount{};
		vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &presentModesCount, nullptr);
		if (formatsCount != 0)
		{
			details.presentModes.resize(formatsCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &presentModesCount, details.presentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
				&& availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes)
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				return availablePresentMode;

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return capabilities.currentExtent;
		else
		{
			int width{}, height{};
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent{
				.width{ static_cast<uint32_t>(width) },
				.height{ static_cast<uint32_t>(height) }
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
			return actualExtent;
			
		}
	}

	void createSwapChain()
	{
		SwapChainSupportDetails swapChainSupport{ querySwapChainSupport(physicalDevice) };

		VkSurfaceFormatKHR surfaceFormat{ chooseSwapSurfaceFormat(swapChainSupport.formats) };
		VkPresentModeKHR presentMode{ chooseSwapPresentMode(swapChainSupport.presentModes) };
		swapChainExtent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount{ swapChainSupport.capabilities.minImageCount + 1 };
		if (swapChainSupport.capabilities.maxImageCount > 0
			&& imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		swapChainImageFormat = surfaceFormat.format;

		VkSwapchainCreateInfoKHR createInfo{
			.sType{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR },
			.surface{ surface },
			.minImageCount{ imageCount },
			.imageFormat{ surfaceFormat.format },
			.imageColorSpace{ surfaceFormat.colorSpace },
			.imageExtent{ swapChainExtent },
			.imageArrayLayers{ 1 },
			.imageUsage{ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT }
		};

		QueueFamilyIndices indices{ findQueueFamilies(physicalDevice) };
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
			throw std::runtime_error("Failed to create the swap chain!");

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
	}

	void createImageViews()
	{
		swapChainImageViews.resize(swapChainImages.size());

		for (size_t i{ 0 }; i < swapChainImages.size(); ++i)
		{
			VkImageViewCreateInfo createInfo{
				.sType{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO },
				.image{ swapChainImages[i] },
				.viewType{ VK_IMAGE_VIEW_TYPE_2D },
				.format{ swapChainImageFormat },
				.components{ 
					.r{ VK_COMPONENT_SWIZZLE_IDENTITY },
					.g{ VK_COMPONENT_SWIZZLE_IDENTITY },
					.b{ VK_COMPONENT_SWIZZLE_IDENTITY },
					.a{ VK_COMPONENT_SWIZZLE_IDENTITY }
				},
				.subresourceRange{ 
					.aspectMask{ VK_IMAGE_ASPECT_COLOR_BIT },
					.baseMipLevel{ 0 },
					.levelCount{ 1 },
					.baseArrayLayer{ 0 },
					.layerCount{ 1 }
				}
			};

			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to create the image view!");
		}
	}

	void createGraphicsPipeline()
	{

	}
};