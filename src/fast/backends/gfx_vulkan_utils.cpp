#include "fast/backends/gfx_vulkan_utils.h"

#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <SDL_vulkan.h>
#else
#include <SDL2/SDL_vulkan.h>
#endif

#include <algorithm>
#include <cstring>
#include <set>
#include <stdexcept>
#include <string>

namespace Fast {
namespace Vulkan {
namespace {

constexpr const char* ValidationLayerName = "VK_LAYER_KHRONOS_validation";

void CheckVk(VkResult result, const char* message) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(message) + " (VkResult " + std::to_string(result) + ")");
    }
}

bool HasExtension(const std::vector<VkExtensionProperties>& extensions, const char* extensionName) {
    return std::any_of(extensions.begin(), extensions.end(), [extensionName](const VkExtensionProperties& extension) {
        return std::strcmp(extension.extensionName, extensionName) == 0;
    });
}

bool HasLayer(const std::vector<VkLayerProperties>& layers, const char* layerName) {
    return std::any_of(layers.begin(), layers.end(), [layerName](const VkLayerProperties& layer) {
        return std::strcmp(layer.layerName, layerName) == 0;
    });
}

std::vector<VkExtensionProperties> EnumerateInstanceExtensions(const char* layerName) {
    uint32_t extensionCount = 0;
    CheckVk(vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr),
            "Failed to enumerate Vulkan instance extension count");

    std::vector<VkExtensionProperties> extensions(extensionCount);
    CheckVk(vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, extensions.data()),
            "Failed to enumerate Vulkan instance extensions");
    return extensions;
}

std::vector<VkLayerProperties> EnumerateInstanceLayers() {
    uint32_t layerCount = 0;
    CheckVk(vkEnumerateInstanceLayerProperties(&layerCount, nullptr), "Failed to enumerate Vulkan layer count");

    std::vector<VkLayerProperties> layers(layerCount);
    CheckVk(vkEnumerateInstanceLayerProperties(&layerCount, layers.data()), "Failed to enumerate Vulkan layers");
    return layers;
}

std::vector<VkExtensionProperties> EnumerateDeviceExtensions(VkPhysicalDevice physicalDevice) {
    uint32_t extensionCount = 0;
    CheckVk(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr),
            "Failed to enumerate Vulkan device extension count");

    std::vector<VkExtensionProperties> extensions(extensionCount);
    CheckVk(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data()),
            "Failed to enumerate Vulkan device extensions");
    return extensions;
}

std::vector<VkExtensionProperties> GetAvailableInstanceExtensions(const std::vector<const char*>& enabledLayers) {
    auto extensions = EnumerateInstanceExtensions(nullptr);

    for (const char* layerName : enabledLayers) {
        auto layerExtensions = EnumerateInstanceExtensions(layerName);
        extensions.insert(extensions.end(), layerExtensions.begin(), layerExtensions.end());
    }

    return extensions;
}

std::vector<const char*> GetRequiredInstanceExtensions(SDL_Window* window) {
    uint32_t sdlExtensionCount = 0;
    if (SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, nullptr) != SDL_TRUE) {
        throw std::runtime_error(std::string("Failed to query SDL Vulkan instance extension count: ") + SDL_GetError());
    }

    std::vector<const char*> extensions(sdlExtensionCount);
    if (SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, extensions.data()) != SDL_TRUE) {
        throw std::runtime_error(std::string("Failed to query SDL Vulkan instance extensions: ") + SDL_GetError());
    }

    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
    return extensions;
}

bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice) {
    auto availableExtensions = EnumerateDeviceExtensions(physicalDevice);
    for (const char* requiredExtension : GetRequiredDeviceExtensions()) {
        if (!HasExtension(availableExtensions, requiredExtension)) {
            return false;
        }
    }
    return true;
}

SwapchainSupport QuerySwapchainSupportInternal(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    SwapchainSupport support;

    CheckVk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &support.capabilities),
            "Failed to query Vulkan surface capabilities");

    uint32_t formatCount = 0;
    CheckVk(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr),
            "Failed to query Vulkan surface format count");
    support.formats.resize(formatCount);
    if (formatCount > 0) {
        CheckVk(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, support.formats.data()),
                "Failed to query Vulkan surface formats");
    }

    uint32_t presentModeCount = 0;
    CheckVk(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr),
            "Failed to query Vulkan present mode count");
    support.presentModes.resize(presentModeCount);
    if (presentModeCount > 0) {
        CheckVk(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount,
                                                          support.presentModes.data()),
                "Failed to query Vulkan present modes");
    }

    return support;
}

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = VK_FALSE;
        CheckVk(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport),
                "Failed to query Vulkan surface support");
        if (presentSupport == VK_TRUE) {
            indices.presentFamily = i;
        }

        if (indices.IsComplete()) {
            break;
        }
    }

    return indices;
}

bool CheckRequiredFeatures(VkPhysicalDevice physicalDevice) {
    VkPhysicalDeviceVulkan12Features vulkan12Features = {};
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

    VkPhysicalDeviceVulkan13Features vulkan13Features = {};
    vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vulkan13Features.pNext = &vulkan12Features;

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures = {};
    extendedDynamicStateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    extendedDynamicStateFeatures.pNext = &vulkan13Features;

    VkPhysicalDeviceFeatures2 features = {};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &extendedDynamicStateFeatures;

    vkGetPhysicalDeviceFeatures2(physicalDevice, &features);

    return vulkan12Features.descriptorIndexing == VK_TRUE &&
           vulkan12Features.runtimeDescriptorArray == VK_TRUE &&
           vulkan12Features.descriptorBindingVariableDescriptorCount == VK_TRUE &&
           vulkan12Features.descriptorBindingPartiallyBound == VK_TRUE &&
           vulkan12Features.descriptorBindingSampledImageUpdateAfterBind == VK_TRUE &&
           vulkan12Features.shaderSampledImageArrayNonUniformIndexing == VK_TRUE &&
           vulkan12Features.bufferDeviceAddress == VK_TRUE && vulkan12Features.timelineSemaphore == VK_TRUE &&
           vulkan13Features.dynamicRendering == VK_TRUE && vulkan13Features.synchronization2 == VK_TRUE &&
           vulkan13Features.shaderDemoteToHelperInvocation == VK_TRUE &&
           extendedDynamicStateFeatures.extendedDynamicState == VK_TRUE;
}

std::string MakeSingleLineMessage(const char* message) {
    if (message == nullptr) {
        return "No message provided";
    }

    std::string singleLineMessage;
    bool pendingWhitespace = false;
    for (const char* current = message; *current != '\0'; current++) {
        if (*current == '\r' || *current == '\n') {
            pendingWhitespace = true;
            continue;
        }

        if (pendingWhitespace && !singleLineMessage.empty() && *current != ' ' && *current != '\t') {
            singleLineMessage += ' ';
        }
        pendingWhitespace = false;
        singleLineMessage += *current;
    }

    return singleLineMessage;
}

bool IsDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, QueueFamilyIndices* queueFamilies) {
    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    if (properties.apiVersion < MinimumApiVersion) {
        return false;
    }

    if (!CheckDeviceExtensionSupport(physicalDevice)) {
        return false;
    }

    auto swapchainSupport = QuerySwapchainSupportInternal(physicalDevice, surface);
    if (swapchainSupport.formats.empty() || swapchainSupport.presentModes.empty()) {
        return false;
    }

    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);
    if (!indices.IsComplete()) {
        return false;
    }

    if (!CheckRequiredFeatures(physicalDevice)) {
        return false;
    }

    *queueFamilies = indices;
    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                             VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                             const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
    (void)userData;

    std::string messageType;
    if ((messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) != 0) {
        messageType += "GENERAL";
    }
    if ((messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0) {
        messageType += messageType.empty() ? "VALIDATION" : "|VALIDATION";
    }
    if ((messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0) {
        messageType += messageType.empty() ? "PERFORMANCE" : "|PERFORMANCE";
    }
    if (messageType.empty()) {
        messageType = "UNKNOWN";
    }

    const char* severity = "UNKNOWN";
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            severity = "VERBOSE";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            severity = "INFO";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            severity = "WARNING";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            severity = "ERROR";
            break;
        default:
            break;
    }

    const char* messageId = callbackData != nullptr && callbackData->pMessageIdName != nullptr
                                ? callbackData->pMessageIdName
                                : "no-message-id";
    std::string message = MakeSingleLineMessage(callbackData != nullptr ? callbackData->pMessage : nullptr);

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        SPDLOG_ERROR("[VULKAN] [{}][{}][{}] {}", severity, messageType, messageId, message);
    } else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        SPDLOG_WARN("[VULKAN] [{}][{}][{}] {}", severity, messageType, messageId, message);
    } else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        SPDLOG_INFO("[VULKAN] [{}][{}][{}] {}", severity, messageType, messageId, message);
    } else {
        SPDLOG_DEBUG("[VULKAN] [{}][{}][{}] {}", severity, messageType, messageId, message);
    }

    return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT GetDebugMessengerCreateInfo() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
    return createInfo;
}

} // namespace

bool QueueFamilyIndices::IsComplete() const {
    return graphicsFamily.has_value() && presentFamily.has_value();
}

std::vector<const char*> GetValidationLayers() {
    return { ValidationLayerName };
}

std::vector<const char*> GetRequiredDeviceExtensions() {
    return { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME };
}

VkInstance CreateInstance(SDL_Window* window) {
    uint32_t instanceVersion = 0;
    CheckVk(vkEnumerateInstanceVersion(&instanceVersion), "Failed to enumerate Vulkan instance version");
    if (instanceVersion < MinimumApiVersion) {
        throw std::runtime_error("Vulkan 1.3 is required but the loader does not support it");
    }

    auto availableLayers = EnumerateInstanceLayers();
    auto validationLayers = GetValidationLayers();
    for (const char* validationLayer : validationLayers) {
        if (!HasLayer(availableLayers, validationLayer)) {
            throw std::runtime_error(std::string("Missing required Vulkan validation layer: ") + validationLayer);
        }
    }

    auto availableExtensions = GetAvailableInstanceExtensions(validationLayers);
    auto requiredExtensions = GetRequiredInstanceExtensions(window);
    for (const char* requiredExtension : requiredExtensions) {
        if (!HasExtension(availableExtensions, requiredExtension)) {
            throw std::runtime_error(std::string("Missing required Vulkan instance extension: ") + requiredExtension);
        }
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Shipwright";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "libultraship";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = MinimumApiVersion;

    VkValidationFeatureEnableEXT validationFeatures[] = {
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
    };

    VkValidationFeaturesEXT validationFeaturesInfo = {};
    validationFeaturesInfo.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    validationFeaturesInfo.enabledValidationFeatureCount = static_cast<uint32_t>(std::size(validationFeatures));
    validationFeaturesInfo.pEnabledValidationFeatures = validationFeatures;

    auto debugMessengerCreateInfo = GetDebugMessengerCreateInfo();
    debugMessengerCreateInfo.pNext = &validationFeaturesInfo;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = &debugMessengerCreateInfo;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    VkInstance instance = VK_NULL_HANDLE;
    CheckVk(vkCreateInstance(&createInfo, nullptr, &instance), "Failed to create Vulkan instance");
    return instance;
}

VkDebugUtilsMessengerEXT CreateDebugMessenger(VkInstance instance) {
    auto createInfo = GetDebugMessengerCreateInfo();

    auto createDebugUtilsMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (createDebugUtilsMessenger == nullptr) {
        throw std::runtime_error("Failed to load vkCreateDebugUtilsMessengerEXT");
    }

    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    CheckVk(createDebugUtilsMessenger(instance, &createInfo, nullptr, &debugMessenger),
            "Failed to create Vulkan debug messenger");
    return debugMessenger;
}

void DestroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger) {
    if (debugMessenger == VK_NULL_HANDLE) {
        return;
    }

    auto destroyDebugUtilsMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (destroyDebugUtilsMessenger != nullptr) {
        destroyDebugUtilsMessenger(instance, debugMessenger, nullptr);
    }
}

VkSurfaceKHR CreateSurface(VkInstance instance, SDL_Window* window) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (SDL_Vulkan_CreateSurface(window, instance, &surface) != SDL_TRUE) {
        throw std::runtime_error(std::string("Failed to create Vulkan surface: ") + SDL_GetError());
    }
    return surface;
}

DeviceSelection PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
    uint32_t deviceCount = 0;
    CheckVk(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr), "Failed to enumerate Vulkan physical devices");
    if (deviceCount == 0) {
        throw std::runtime_error("No Vulkan physical devices found");
    }

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    CheckVk(vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data()),
            "Failed to enumerate Vulkan physical devices");

    for (VkPhysicalDevice physicalDevice : physicalDevices) {
        QueueFamilyIndices queueFamilies;
        if (IsDeviceSuitable(physicalDevice, surface, &queueFamilies)) {
            VkPhysicalDeviceProperties properties = {};
            vkGetPhysicalDeviceProperties(physicalDevice, &properties);
            SPDLOG_INFO("Selected Vulkan device: {} (API {}.{}.{})", properties.deviceName,
                        VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion),
                        VK_VERSION_PATCH(properties.apiVersion));
            return { physicalDevice, queueFamilies };
        }
    }

    throw std::runtime_error(
        "No Vulkan physical device supports Vulkan 1.3, presentation, swapchain, and required features");
}

VkDevice CreateLogicalDevice(VkPhysicalDevice physicalDevice, const QueueFamilyIndices& queueFamilies,
                             VkQueue* graphicsQueue, VkQueue* presentQueue) {
    std::set<uint32_t> uniqueQueueFamilies = { *queueFamilies.graphicsFamily, *queueFamilies.presentFamily };

    float queuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceVulkan12Features vulkan12Features = {};
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12Features.descriptorIndexing = VK_TRUE;
    vulkan12Features.runtimeDescriptorArray = VK_TRUE;
    vulkan12Features.descriptorBindingVariableDescriptorCount = VK_TRUE;
    vulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
    vulkan12Features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    vulkan12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    vulkan12Features.bufferDeviceAddress = VK_TRUE;
    vulkan12Features.timelineSemaphore = VK_TRUE;

    VkPhysicalDeviceVulkan13Features vulkan13Features = {};
    vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vulkan13Features.pNext = &vulkan12Features;
    vulkan13Features.dynamicRendering = VK_TRUE;
    vulkan13Features.synchronization2 = VK_TRUE;
    vulkan13Features.shaderDemoteToHelperInvocation = VK_TRUE;

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures = {};
    extendedDynamicStateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    extendedDynamicStateFeatures.pNext = &vulkan13Features;
    extendedDynamicStateFeatures.extendedDynamicState = VK_TRUE;

    VkPhysicalDeviceFeatures2 features = {};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &extendedDynamicStateFeatures;

    auto deviceExtensions = GetRequiredDeviceExtensions();

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &features;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkDevice device = VK_NULL_HANDLE;
    CheckVk(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device), "Failed to create Vulkan logical device");

    vkGetDeviceQueue(device, *queueFamilies.graphicsFamily, 0, graphicsQueue);
    vkGetDeviceQueue(device, *queueFamilies.presentFamily, 0, presentQueue);

    return device;
}

SwapchainSupport QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    return QuerySwapchainSupportInternal(physicalDevice, surface);
}

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        // TODO: is this the best format we can use that is guaranteed to be
        // supported?
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // TODO: will need to couple this to vsync/fps settings

    for (const auto& availablePresentMode : availablePresentModes) {
        // TODO: why prefer mailbox???
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window* window) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }

    int width = 0;
    int height = 0;
    SDL_Vulkan_GetDrawableSize(window, &width, &height);

    VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    return actualExtent;
}

} // namespace Vulkan
} // namespace Fast
