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

#include <vulkan/vulkan.hpp>

namespace VulkanAbstractionLayer
{
    struct WindowSurface;

    struct VulkanContextCreateOptions
    {
        int VulkanApiMajorVersion = 1;
        int VulkanApiMinorVersion = 0;
        void (*ErrorCallback)(const char*) = nullptr;
        void (*InfoCallback)(const char*) = nullptr;
        vk::ArrayProxyNoTemporaries<const char* const> Extensions;
        vk::ArrayProxyNoTemporaries<const char* const> Layers;
        const char* ApplicationName = "VulkanAbstractionLayer";
        const char* EngineName = "VulkanAbstractionLayer";
    };

    enum class DeviceType
    {
        CPU = 0,
        DISCRETE_GPU,
        INTEGRATED_GPU,
        VIRTUAL_GPU,
        OTHER,
    };

    struct DeviceInitializeOptions
    {
        DeviceType PreferredType = DeviceType::DISCRETE_GPU;
        void (*ErrorCallback)(const char*) = nullptr;
        void (*InfoCallback)(const char*) = nullptr;
    };

    class VulkanContext
    {
        vk::Instance instance;
        vk::SurfaceKHR surface;
        vk::PhysicalDevice physicalDevice;
        vk::PhysicalDeviceProperties physicalDeviceProperties;
        uint32_t queueFamilyIndex = { };
        uint32_t apiVersion = { };

        void Move(VulkanContext&& other);
    public:
        VulkanContext(const VulkanContextCreateOptions& options);
        ~VulkanContext();
        VulkanContext(const VulkanContext&) = delete;
        VulkanContext& operator=(const VulkanContext&) = delete;
        VulkanContext(VulkanContext&& other) noexcept;
        VulkanContext& operator=(VulkanContext&& other) noexcept;

        const vk::Instance& GetInstance() const { return this->instance; }
        const vk::SurfaceKHR& GetSurface() const { return this->surface; }
        const vk::PhysicalDevice& GetPhysicalDevice() const { return this->physicalDevice; }

        void InitializePhysicalDevice(const WindowSurface& surface, const DeviceInitializeOptions& options);
    };
}