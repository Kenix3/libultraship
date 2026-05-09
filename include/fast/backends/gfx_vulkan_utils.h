#pragma once

#include <vulkan/vulkan.h>

#include <optional>
#include <vector>

struct SDL_Window;

namespace Fast {
namespace Vulkan {

constexpr uint32_t MinimumApiVersion = VK_API_VERSION_1_3;

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool IsComplete() const;
};

struct DeviceSelection {
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    QueueFamilyIndices queueFamilies;
};

struct SwapchainSupport {
    VkSurfaceCapabilitiesKHR capabilities = {};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

std::vector<const char*> GetValidationLayers();
std::vector<const char*> GetRequiredDeviceExtensions();

VkInstance CreateInstance(SDL_Window* window);
VkDebugUtilsMessengerEXT CreateDebugMessenger(VkInstance instance);
void DestroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger);
VkSurfaceKHR CreateSurface(VkInstance instance, SDL_Window* window);
DeviceSelection PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
VkDevice CreateLogicalDevice(VkPhysicalDevice physicalDevice, const QueueFamilyIndices& queueFamilies,
                             VkQueue* graphicsQueue, VkQueue* presentQueue);
SwapchainSupport QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window* window);

} // namespace Vulkan
} // namespace Fast
