#include <filesystem>
#include <iostream>

#include "VulkanAbstractionLayer/Window.h"
#include "VulkanAbstractionLayer/VulkanContext.h"

using namespace VulkanAbstractionLayer;

int main()
{
    std::filesystem::current_path(APPLICATION_WORKING_DIRECTORY);

    WindowCreateOptions windowOptions;
    windowOptions.Position = { 300.0f, 100.0f };
    windowOptions.Size = { 1280.0f, 720.0f };
    windowOptions.ErrorCallback = [](const char* message)
    {
        std::cerr << "[ERROR Window]: " << message << std::endl;
    };

    Window window(windowOptions);
    auto requiredExtensions = window.GetRequiredExtensions();

    std::array validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    VulkanContextCreateOptions vulkanOptions;
    vulkanOptions.VulkanApiMajorVersion = 1;
    vulkanOptions.VulkanApiMinorVersion = 2;
    vulkanOptions.Extensions = {
        requiredExtensions.ExtensionCount,
        requiredExtensions.ExtensionNames
    };
    vulkanOptions.Layers = validationLayers;
    vulkanOptions.ErrorCallback = [](const char* message)
    {
        std::cerr << "[ERROR Vulkan]: " << message << std::endl;
    };
    vulkanOptions.InfoCallback = [](const char* message)
    {
        std::cout << "[INFO Vulkan]: " << message << std::endl;
    };

    VulkanContext Vulkan(vulkanOptions);

    DeviceInitializeOptions deviceOptions;
    deviceOptions.PreferredType = DeviceType::DISCRETE_GPU;
    deviceOptions.ErrorCallback = [](const char* message)
    {
        std::cerr << "[ERROR Vulkan]: " << message << std::endl;
    };
    deviceOptions.InfoCallback = [](const char* message)
    {
        std::cout << "[INFO Vulkan]: " << message << std::endl;
    };

    Vulkan.InitializePhysicalDevice(window.CreateWindowSurface(Vulkan), deviceOptions);
    
    while (!window.ShouldClose())
    {
        window.PollEvents();
    }

    return 0;
}