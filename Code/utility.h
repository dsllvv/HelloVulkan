#pragma once

#include <vulkan/vulkan.h>

#include <optional>
#include <vector>


#ifdef NDEBUG
	constexpr bool enableValidationLayers{ false };
#else
	constexpr bool enableValidationLayers{ true };
#endif

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily{};

	bool isComplete()
	{
		return graphicsFamily.has_value();
	}
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

std::vector<const char*> getRequiredExtensions();

bool checkValidationLayersSupport();

bool checkExtensionsSupport(const std::vector<const char*>& glfwExtensions);

VkResult createDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger);

void destroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator);