// Copyright(c) 2021, #Momo
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
// 
// 1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and /or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "VulkanContext.h"

#include "vk_mem_alloc.h"
#include "ShaderLang.h"

#include <algorithm>
#include <cstring>
#include <optional>

namespace VulkanAbstractionLayer
{
    bool CheckVulkanPresentationSupport(const vk::Instance& instance, const vk::PhysicalDevice& physicalDevice, uint32_t familyQueueIndex);

    constexpr vk::PhysicalDeviceType DeviceTypeMapping[] = {
        vk::PhysicalDeviceType::eCpu,
        vk::PhysicalDeviceType::eDiscreteGpu,
        vk::PhysicalDeviceType::eIntegratedGpu,
        vk::PhysicalDeviceType::eVirtualGpu,
        vk::PhysicalDeviceType::eOther,
    };

    void CheckRequestedLayers(const VulkanContextCreateOptions& options)
    {
        options.InfoCallback("enumerating requested layers:");

        auto layers = vk::enumerateInstanceLayerProperties();
        for (const char* layerName : options.Layers)
        {
            options.InfoCallback(layerName);

            auto layerIt = std::find_if(layers.begin(), layers.end(),
                [layerName](const vk::LayerProperties& layer)
                {
                    return std::strcmp(layer.layerName.data(), layerName) == 0;
                });

            if (layerIt == layers.end())
            {
                options.ErrorCallback(("cannot enable requested layer: " + std::string(layerName)).c_str());
                return;
            }
        }
    }

    void CheckRequestedExtensions(const VulkanContextCreateOptions& options)
    {
        options.InfoCallback("enumerating requested extensions:");

        auto extensions = vk::enumerateInstanceExtensionProperties();
        for (const char* extensionName : options.Extensions)
        {
            options.InfoCallback(extensionName);

            auto layerIt = std::find_if(extensions.begin(), extensions.end(),
                [extensionName](const vk::ExtensionProperties& extension)
                {
                    return std::strcmp(extension.extensionName.data(), extensionName) == 0;
                });

            if (layerIt == extensions.end())
            {
                options.ErrorCallback("cannot enable requested extension");
                return;
            }
        }
    }

    std::optional<uint32_t> DetermineQueueFamilyIndex(const vk::Instance& instance, const vk::PhysicalDevice device, const vk::SurfaceKHR& surface)
    {
        auto queueFamilyProperties = device.getQueueFamilyProperties();
        uint32_t index = 0;
        for (const auto& property : queueFamilyProperties)
        {
            if ((property.queueCount > 0) &&
                (device.getSurfaceSupportKHR(index, surface)) &&
                (CheckVulkanPresentationSupport(instance, device, index)) &&
                (property.queueFlags & vk::QueueFlagBits::eGraphics) &&
                (property.queueFlags & vk::QueueFlagBits::eCompute))
            {
                return index;
            }
            index++;
        }
        return { };
    }

    VulkanContext::VulkanContext(const VulkanContextCreateOptions& options)
    {
        vk::ApplicationInfo applicationInfo;
        applicationInfo
            .setPApplicationName(options.ApplicationName)
            .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
            .setPEngineName(options.EngineName)
            .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
            .setApiVersion(VK_MAKE_VERSION(options.VulkanApiMajorVersion, options.VulkanApiMinorVersion, 0));
    
        vk::InstanceCreateInfo instanceCreateInfo;
        instanceCreateInfo
            .setPApplicationInfo(&applicationInfo)
            .setPEnabledExtensionNames(options.Extensions)
            .setPEnabledLayerNames(options.Layers);

        CheckRequestedExtensions(options);
        CheckRequestedLayers(options);

        this->instance = vk::createInstance(instanceCreateInfo);
        this->apiVersion = applicationInfo.apiVersion;

        options.InfoCallback("created vulkan instance object");
    }

    VulkanContext::~VulkanContext()
    {
        this->device.waitIdle();

        this->virtualFrames.Destroy();
        this->descriptorCache.Destroy();
       
        if ((bool)this->commandPool) this->device.destroyCommandPool(this->commandPool);

        this->swapchainImages.clear();

        vmaDestroyAllocator(this->allocator);

        glslang::FinalizeProcess();

        if ((bool)this->swapchain) this->device.destroySwapchainKHR(this->swapchain);
        if ((bool)this->imageAvailableSemaphore) this->device.destroySemaphore(this->imageAvailableSemaphore);
        if ((bool)this->renderingFinishedSemaphore) this->device.destroySemaphore(this->renderingFinishedSemaphore);
        if ((bool)this->immediateFence) this->device.destroyFence(this->immediateFence);
        if ((bool)this->device) this->device.destroy();
        if ((bool)this->surface) this->instance.destroySurfaceKHR(this->surface);
        if ((bool)this->instance) this->instance.destroy();
        this->presentImageCount = { };
        this->queueFamilyIndex = { };
        this->apiVersion = { };
    }

    void VulkanContext::InitializeContext(const WindowSurface& surface, const ContextInitializeOptions& options)
    {
        this->surface = *reinterpret_cast<const VkSurfaceKHR*>(&surface);

        if (!(bool)this->surface)
        {
            options.ErrorCallback("failed to initialize surface");
            return;
        }

        options.InfoCallback("enumerating physical devices:");
        
        // enumerate physical devices
        auto physicalDevices = this->instance.enumeratePhysicalDevices();
        for (const auto& device : physicalDevices)
        {
            auto properties = device.getProperties();

            options.InfoCallback("checking " + std::string(properties.deviceName.data()) + "...");

            if (properties.apiVersion < this->apiVersion)
            {
                options.InfoCallback(std::string(properties.deviceName.data()) + ": skipping device as its Vulkan API version is less than required");
                options.InfoCallback("    " +
                    std::to_string(VK_VERSION_MAJOR(properties.apiVersion)) + "." + std::to_string(VK_VERSION_MINOR(properties.apiVersion)) + " < " +
                    std::to_string(VK_VERSION_MAJOR(this->apiVersion)) + "." + std::to_string(VK_VERSION_MINOR(this->apiVersion))
                );
                continue;
            }

            auto queueFamilyIndex = DetermineQueueFamilyIndex(this->instance, device, this->surface);
            if (!queueFamilyIndex.has_value())
            {
                options.InfoCallback(std::string(properties.deviceName.data()) + ": skipping device as its queue families does not satisfy the requirements");
                continue;
            }

            this->physicalDevice = device;
            this->physicalDeviceProperties = properties;
            this->queueFamilyIndex = queueFamilyIndex.value();
            if (properties.deviceType == DeviceTypeMapping[(size_t)options.PreferredDeviceType])
                break;
        }

        if (!(bool)this->physicalDevice)
        {
            options.ErrorCallback("failed to find appropriate physical device");
            return;
        }
        else
        {
            options.InfoCallback((std::string("selected physical device: ") + this->physicalDeviceProperties.deviceName.data()).c_str());
        }

        // collect surface present info

        auto presentModes = this->physicalDevice.getSurfacePresentModesKHR(this->surface);
        auto surfaceCapabilities = this->physicalDevice.getSurfaceCapabilitiesKHR(this->surface);
        auto surfaceFormats = this->physicalDevice.getSurfaceFormatsKHR(this->surface);

        // find best surface present mode
        this->surfacePresentMode = vk::PresentModeKHR::eImmediate;
        if (std::find(presentModes.begin(), presentModes.end(), vk::PresentModeKHR::eMailbox) != presentModes.end())
            this->surfacePresentMode = vk::PresentModeKHR::eMailbox;

        // determine present image count
        this->presentImageCount = std::max(surfaceCapabilities.maxImageCount, 1u);

        // find best surface format
        this->surfaceFormat = surfaceFormats.front();
        for (const auto& format : surfaceFormats)
        {
            if (format.format == vk::Format::eR8G8B8A8Unorm || format.format == vk::Format::eB8G8R8A8Unorm)
                this->surfaceFormat = format;
        }

        options.InfoCallback("selected surface present mode: " + vk::to_string(this->surfacePresentMode));
        options.InfoCallback("selected surface format: " + vk::to_string(this->surfaceFormat.format));
        options.InfoCallback("present image count: " + std::to_string(this->presentImageCount));

        // logical device and device queue

        vk::DeviceQueueCreateInfo deviceQueueCreateInfo;
        std::array queuePriorities = { 1.0f };
        deviceQueueCreateInfo.setQueueFamilyIndex(this->queueFamilyIndex);
        deviceQueueCreateInfo.setQueuePriorities(queuePriorities);

        auto deviceExtensions = options.DeviceExtensions;
        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

        vk::PhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures;
        descriptorIndexingFeatures.descriptorBindingPartiallyBound = true;

        vk::DeviceCreateInfo deviceCreateInfo;
        deviceCreateInfo
            .setQueueCreateInfos(deviceQueueCreateInfo)
            .setPEnabledExtensionNames(deviceExtensions)
            .setPNext(&descriptorIndexingFeatures);

        this->device = this->physicalDevice.createDevice(deviceCreateInfo);
        this->deviceQueue = this->device.getQueue(this->queueFamilyIndex, 0);

        options.InfoCallback("created logical device and device queues");

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.vulkanApiVersion = this->apiVersion;
        allocatorInfo.physicalDevice = this->physicalDevice;
        allocatorInfo.device = this->device;
        allocatorInfo.instance = this->instance;
        vmaCreateAllocator(&allocatorInfo, &this->allocator);

        options.InfoCallback("created vulkan memory allocator");

        glslang::InitializeProcess();
        options.InfoCallback("initialized glslang compiler");

        this->RecreateSwapchain(surfaceCapabilities.maxImageExtent.width, surfaceCapabilities.maxImageExtent.height);

        options.InfoCallback("created swapchain");

        this->imageAvailableSemaphore = this->device.createSemaphore(vk::SemaphoreCreateInfo{ });
        this->renderingFinishedSemaphore = this->device.createSemaphore(vk::SemaphoreCreateInfo{ });
        this->immediateFence = this->device.createFence(vk::FenceCreateInfo{ });

        vk::CommandPoolCreateInfo commandPoolCreateInfo;
        commandPoolCreateInfo
            .setQueueFamilyIndex(this->queueFamilyIndex)
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient);

        this->commandPool = this->device.createCommandPool(commandPoolCreateInfo);

        options.InfoCallback("created command buffer pool");

        this->descriptorCache.Init();
        this->virtualFrames.Init(options.VirtualFrameCount, options.MaxStageBufferSize);

        options.InfoCallback("initialization finished");
    }

    void VulkanContext::RecreateSwapchain(uint32_t surfaceWidth, uint32_t surfaceHeight)
    {
        this->device.waitIdle();

        auto surfaceCapabilities = this->physicalDevice.getSurfaceCapabilitiesKHR(this->surface);
        this->surfaceExtent = vk::Extent2D(
            std::clamp(surfaceWidth,  surfaceCapabilities.minImageExtent.width,  surfaceCapabilities.maxImageExtent.width ),
            std::clamp(surfaceHeight, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)
        );

        if (this->surfaceExtent == vk::Extent2D{ 0, 0 })
        {
            this->surfaceExtent = vk::Extent2D{ 1, 1 };
            this->renderingEnabled = false;
            return;
        }
        this->renderingEnabled = true;

        vk::SwapchainCreateInfoKHR swapchainCreateInfo;
        swapchainCreateInfo
            .setSurface(this->surface)
            .setMinImageCount(this->presentImageCount)
            .setImageFormat(this->surfaceFormat.format)
            .setImageColorSpace(this->surfaceFormat.colorSpace)
            .setImageExtent(this->surfaceExtent)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
            .setImageSharingMode(vk::SharingMode::eExclusive)
            .setPreTransform(surfaceCapabilities.currentTransform)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setPresentMode(this->surfacePresentMode)
            .setClipped(true)
            .setOldSwapchain(this->swapchain);

        this->swapchain = this->device.createSwapchainKHR(swapchainCreateInfo);

        if (bool(swapchainCreateInfo.oldSwapchain))
            this->device.destroySwapchainKHR(swapchainCreateInfo.oldSwapchain);

        auto swapchainImages = this->device.getSwapchainImagesKHR(this->swapchain);
        this->presentImageCount = (uint32_t)swapchainImages.size();
        this->swapchainImages.clear();
        this->swapchainImages.reserve(this->presentImageCount);
        this->swapchainImageUsages.assign(this->presentImageCount, ImageUsage::UNKNOWN);

        for (uint32_t i = 0; i <this->presentImageCount; i++)
        {
            this->swapchainImages.push_back(Image(
                swapchainImages[i], 
                this->surfaceExtent.width, 
                this->surfaceExtent.height, 
                FromNative(this->surfaceFormat.format)
            ));
        }
    }

    const Image& VulkanContext::AcquireSwapchainImage(size_t index, ImageUsage::Bits usage)
    {
        this->swapchainImageUsages[index] = usage;
        return this->swapchainImages[index];
    }

    ImageUsage::Bits VulkanContext::GetSwapchainImageUsage(size_t index) const
    {
        return this->swapchainImageUsages[index];
    }

    void VulkanContext::StartFrame()
    {
        this->virtualFrames.StartFrame();
    }

    const Image& VulkanContext::AcquireCurrentSwapchainImage(ImageUsage::Bits usage)
    {
        return this->AcquireSwapchainImage(this->virtualFrames.GetPresentImageIndex(), usage);
    }

    CommandBuffer& VulkanContext::GetCurrentCommandBuffer()
    {
        return this->virtualFrames.GetCurrentFrame().Commands;
    }

    StageBuffer& VulkanContext::GetCurrentStageBuffer()
    {
        return this->virtualFrames.GetCurrentFrame().StagingBuffer;
    }

    void VulkanContext::SubmitCommandsImmediate(const CommandBuffer& commands)
    {
        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBuffers(commands.GetNativeHandle());
        this->GetGraphicsQueue().submit(submitInfo, this->immediateFence);
        auto waitResult = this->device.waitForFences(this->immediateFence, false, UINT64_MAX);
        assert(waitResult == vk::Result::eSuccess);
        this->device.resetFences(this->immediateFence);
    }

    void VulkanContext::EndFrame()
    {
        this->virtualFrames.EndFrame();
    }

    static VulkanContext* CurrentVulkanContext = nullptr;

    void SetCurrentVulkanContext(VulkanContext& context)
    {
        CurrentVulkanContext = std::addressof(context);
    }

    VulkanContext& GetCurrentVulkanContext()
    {
        assert(CurrentVulkanContext != nullptr);
        return *CurrentVulkanContext;
    }
}