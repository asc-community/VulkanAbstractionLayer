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
#include <vector>

namespace VulkanAbstractionLayer
{
    struct WindowSurface;

    struct VulkanContextCreateOptions
    {
        int VulkanApiMajorVersion = 1;
        int VulkanApiMinorVersion = 0;
        void (*ErrorCallback)(const char*) = nullptr;
        void (*InfoCallback)(const char*) = nullptr;
        std::vector<const char*> Extensions;
        std::vector<const char*> Layers;
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

    struct ContextInitializeOptions
    {
        DeviceType PreferredDeviceType = DeviceType::DISCRETE_GPU;
        void (*ErrorCallback)(const char*) = nullptr;
        void (*InfoCallback)(const char*) = nullptr;
        std::vector<const char*> DeviceExtensions;
    };

    class VulkanContext
    {
        vk::Instance instance;
        vk::SurfaceKHR surface;
        vk::SurfaceFormatKHR surfaceFormat;
        vk::PresentModeKHR surfacePresentMode;
        uint32_t presentImageCount = { };
        vk::PhysicalDevice physicalDevice;
        vk::PhysicalDeviceProperties physicalDeviceProperties;
        vk::Device device;
        vk::Queue deviceQueue;
        vk::Semaphore imageAvailableSemaphore;
        vk::Semaphore renderingFinishedSemaphore;
        vk::SwapchainKHR swapchain;
        std::vector<vk::ImageView> swapchainImageViews;
        uint32_t queueFamilyIndex = { };
        uint32_t apiVersion = { };
    public:
        VulkanContext(const VulkanContextCreateOptions& options);
        ~VulkanContext();
        VulkanContext(const VulkanContext&) = delete;
        VulkanContext& operator=(const VulkanContext&) = delete;
        VulkanContext(VulkanContext&& other) = delete;
        VulkanContext& operator=(VulkanContext&& other) = delete;

        const vk::Instance& GetInstance() const { return this->instance; }
        const vk::SurfaceKHR& GetSurface() const { return this->surface; }
        const vk::PhysicalDevice& GetPhysicalDevice() const { return this->physicalDevice; }
        const vk::Device& GetDevice() const { return this->device; }
        const vk::Queue& GetPresentQueue() const { return this->deviceQueue; }
        const vk::Queue& GetGraphicsQueue() const { return this->deviceQueue; }

        void InitializeContext(const WindowSurface& surface, const ContextInitializeOptions& options);
        void RecreateSwapchain(uint32_t surfaceWidth, uint32_t surfaceHeight);
    };
}