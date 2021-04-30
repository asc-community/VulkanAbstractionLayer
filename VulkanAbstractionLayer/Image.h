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

#pragma once

#include <vulkan/vulkan.hpp>
#include "VulkanMemoryAllocator.h"

namespace VulkanAbstractionLayer
{
    class Image
    {
        vk::Image handle;
        vk::ImageView view;
        vk::Extent2D extent = { 0u, 0u };
        vk::Format format = vk::Format::eUndefined;
        VmaAllocation allocation = { };

        void Destroy();
        void InitView(const vk::Image& image, vk::Format format);
    public:
        Image() = default;
        Image(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage, MemoryUsage memoryUsage);
        Image(vk::Image image, uint32_t width, uint32_t height, vk::Format format);
        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;
        Image(Image&& other) noexcept;
        Image& operator=(Image&& other) noexcept;
        ~Image();

        void Init(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage, MemoryUsage memoryUsage);

        vk::Image GetNativeHandle() const { return this->handle; }
        vk::ImageView GetNativeView() const { return this->view; }
        vk::Format GetFormat() const { return this->format; }
        uint32_t GetWidth() const { return this->extent.width; }
        uint32_t GetHeight() const { return this->extent.height; }
    };
}