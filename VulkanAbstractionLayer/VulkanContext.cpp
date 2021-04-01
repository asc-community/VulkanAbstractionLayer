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
        if (options.InfoCallback)
            options.InfoCallback("enumerating requested layers:");

        auto layers = vk::enumerateInstanceLayerProperties();
        for (const char* layerName : options.Layers)
        {
            if (options.InfoCallback)
                options.InfoCallback(layerName);

            auto layerIt = std::find_if(layers.begin(), layers.end(),
                [layerName](const vk::LayerProperties& layer)
                {
                    return std::strcmp(layer.layerName.data(), layerName) == 0;
                });

            if (layerIt == layers.end())
            {
                if (options.ErrorCallback)
                    options.ErrorCallback("cannot enable requested layer");
                return;
            }
        }
    }

    void CheckRequestedExtensions(const VulkanContextCreateOptions& options)
    {
        if (options.InfoCallback)
            options.InfoCallback("enumerating requested extensions:");

        auto extensions = vk::enumerateInstanceExtensionProperties();
        for (const char* extensionName : options.Extensions)
        {
            if (options.InfoCallback)
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

    std::optional<uint32_t> GetQueueFamilyIndex(const vk::Instance& instance, const vk::PhysicalDevice device, const vk::SurfaceKHR& surface)
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

    void VulkanContext::Move(VulkanContext&& other)
    {
        this->instance = std::move(other.instance);
        this->surface = std::move(other.surface);
        this->physicalDevice = std::move(other.physicalDevice);
        this->physicalDeviceProperties = std::move(other.physicalDeviceProperties);
        this->queueFamilyIndex = std::move(other.queueFamilyIndex);
        this->apiVersion = std::move(other.apiVersion);

        other.surface = nullptr;
        other.instance = nullptr;
        other.physicalDevice = nullptr;
        other.queueFamilyIndex = { };
        other.apiVersion = { };
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

        if (options.InfoCallback)
            options.InfoCallback("created vulkan instance object");
    }

    VulkanContext::~VulkanContext()
    {
        if((bool)this->surface ) this->instance.destroySurfaceKHR(this->surface);
        if((bool)this->instance) this->instance.destroy();
    }

    VulkanContext::VulkanContext(VulkanContext&& other) noexcept
    {
        this->Move(std::move(other));
    }

    VulkanContext& VulkanContext::operator=(VulkanContext&& other) noexcept
    {
        this->Move(std::move(other));
        return *this;
    }

    void VulkanContext::InitializePhysicalDevice(const WindowSurface& surface, const DeviceInitializeOptions& options)
    {
        this->surface = *reinterpret_cast<const VkSurfaceKHR*>(&surface);

        if (!(bool)this->surface)
        {
            if (options.ErrorCallback)
                options.ErrorCallback("failed to initialize surface");
            return;
        }
        
        if (options.InfoCallback)
            options.InfoCallback("enumerating physical devices:");
        
        auto physicalDevices = this->instance.enumeratePhysicalDevices();
        for (const auto& device : physicalDevices)
        {
            auto properties = device.getProperties();

            if (options.InfoCallback) 
                options.InfoCallback(properties.deviceName);

            if (properties.apiVersion < this->apiVersion)
            {
                if (options.InfoCallback) 
                    options.InfoCallback("skipping device as its Vulkan API version is less than required");
                continue;
            }

            auto queueFamilyIndex = GetQueueFamilyIndex(this->instance, device, this->surface);
            if (!queueFamilyIndex.has_value())
            {
                if (options.InfoCallback)
                    options.InfoCallback("skipping device as its queue families does not satisfy the requirements");
                continue;
            }
            
            this->physicalDevice = device;
            this->physicalDeviceProperties = properties;
            this->queueFamilyIndex = queueFamilyIndex.value();
            if (properties.deviceType == DeviceTypeMapping[(size_t)options.PreferredType])
                break;
        }

        if (!(bool)this->physicalDevice)
        {
            if (options.ErrorCallback)
                options.ErrorCallback("failed to find appropriate physical device");
            return;
        }
        else
        {
            if (options.InfoCallback)
            {
                options.InfoCallback("selected physical device:");
                options.InfoCallback(this->physicalDeviceProperties.deviceName);
            }
        }
    }
}