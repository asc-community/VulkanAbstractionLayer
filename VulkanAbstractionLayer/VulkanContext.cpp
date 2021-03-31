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

namespace VulkanAbstractionLayer
{
    void CheckRequestedLayers(const VulkanContextCreateOptions& options)
    {
        if (options.InfoCallback)
            options.InfoCallback("checking requested layers");

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
            }
        }
    }

    void CheckRequestedExtensions(const VulkanContextCreateOptions& options)
    {
        if (options.InfoCallback)
            options.InfoCallback("checking requested extensions");

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
            }
        }
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

        if (options.InfoCallback)
            options.InfoCallback("created vulkan instance object");
    }

    VulkanContext::~VulkanContext()
    {
        this->instance.destroy();
    }
}