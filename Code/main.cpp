#include "utility.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <vector>


constexpr int width{ 800 };
constexpr int height{ 600 };

class HelloTriangleApp
{
public:
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
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
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

		for (const auto& device : devices)
		{
			VkPhysicalDeviceProperties deviceProperties{};
			vkGetPhysicalDeviceProperties(device, &deviceProperties);

			if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
				&& findQueueFamilies(device).isComplete())
			{
				physicalDevice = device;

				std::cout << "\nSelected physical device: " << deviceProperties.deviceName << '\n';

				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE)
			throw std::runtime_error("Failed to find a suitable GPU!");
	}

	void createLogicalDevice()
	{
		QueueFamilyIndices indices{ findQueueFamilies(physicalDevice) };

		float queuePriority{ 1.0f };

		VkDeviceQueueCreateInfo queueCreateInfo{
			.sType{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO },
			.queueFamilyIndex{ indices.graphicsFamily.value() },
			.queueCount{ 1 },
			.pQueuePriorities{ &queuePriority }
		};

		VkPhysicalDeviceFeatures deviceFeatures{};

		VkDeviceCreateInfo createInfo{
			.sType{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO },
			.queueCreateInfoCount{ 1 },
			.pQueueCreateInfos{ &queueCreateInfo },
			.enabledLayerCount{ 0 },
			.enabledExtensionCount{ 0 },
			.pEnabledFeatures{ &deviceFeatures },
		};

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
			throw std::runtime_error("Failed to create the logical device!");

		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
	}

	void initVulkan()
	{
		createInstance();
		setupDebugMessenger();
		pickPhysicalDevice();
		createLogicalDevice();
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

		vkDestroyDevice(device, nullptr);
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
};

int main()
{
	HelloTriangleApp app{};

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}