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

#include "Image.h"
#include "vk_mem_alloc.h"
#include "VulkanMemoryAllocator.h"
#include <cassert>

namespace VulkanAbstractionLayer
{
    void Image::Destroy()
    {
        if ((bool)this->handle)
        {
            if ((bool)this->allocation) // allocated
            {
                (void)vmaDestroyImage(this->allocator, this->handle, this->allocation);
            }
            VmaGetDevice(this->allocator).destroyImageView(this->view);
            this->handle = vk::Image{ };
            this->view = vk::ImageView{ };
            this->extent = vk::Extent2D{ 0u, 0u };
        }
    }

    void Image::InitExternal(const vk::Image& image, vk::Format format)
    {
        this->handle = image;
        this->format = format;

        vk::ImageSubresourceRange subresourceRange{
            vk::ImageAspectFlagBits::eColor,
            0, // base mip level
            1, // level count
            0, // base layer
            1  // layer count
        };

        vk::ImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo
            .setImage(this->handle)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(format)
            .setComponents(vk::ComponentMapping{
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity
                })
            .setSubresourceRange(subresourceRange);

        this->view = VmaGetDevice(this->allocator).createImageView(imageViewCreateInfo);
    }

    Image::Image(const vk::Image& image, vk::Extent2D extent, vk::Format format, VmaAllocator allocator)
        : Image(allocator)
    {
        this->extent = extent;
        this->InitExternal(image, format);
    }

    Image::Image(size_t width, size_t height, vk::Format format, vk::ImageUsageFlags usage, MemoryUsage memoryUsage, VmaAllocator allocator)
        : Image(allocator)
    {
        this->Init(width, height, format, usage, memoryUsage);
    }

    Image::Image(Image&& other) noexcept
    {
        this->handle = other.handle;
        this->view = other.view;
        this->extent = other.extent;
        this->format = other.format;
        this->allocator = other.allocator;
        this->allocation = other.allocation;

        other.handle = vk::Image{ };
        other.view = vk::ImageView{ };
        other.extent = vk::Extent2D{ 0u, 0u };
        other.format = vk::Format::eUndefined;
        other.allocation = { };
    }

    Image& Image::operator=(Image&& other) noexcept
    {
        this->Destroy();

        this->handle = other.handle;
        this->view = other.view;
        this->extent = other.extent;
        this->format = other.format;
        this->allocator = other.allocator;
        this->allocation = other.allocation;

        other.handle = vk::Image{ };
        other.view = vk::ImageView{ };
        other.extent = vk::Extent2D{ 0u, 0u };
        other.format = vk::Format::eUndefined;
        other.allocation = { };

        return *this;
    }

    Image::~Image()
    {
        this->Destroy();
    }

    void Image::Init(size_t width, size_t height, vk::Format format, vk::ImageUsageFlags usage, MemoryUsage memoryUsage)
    {
        vk::ImageCreateInfo imageCreateInfo;
        imageCreateInfo
            .setImageType(vk::ImageType::e2D)
            .setFormat(format)
            .setExtent(vk::Extent3D{ (uint32_t)width, (uint32_t)height, 1 })
            .setSamples(vk::SampleCountFlagBits::e1)
            .setMipLevels(1)
            .setArrayLayers(1)
            .setTiling(vk::ImageTiling::eOptimal)
            .setUsage(usage)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setInitialLayout(vk::ImageLayout::eUndefined);

        VmaAllocationCreateInfo allocationInfo = { };
        allocationInfo.usage = (VmaMemoryUsage)MemoryUsageToNative(memoryUsage);

        vmaCreateImage(this->allocator, (VkImageCreateInfo*)&imageCreateInfo, &allocationInfo, (VkImage*)&this->handle, &this->allocation, nullptr);

        this->extent = vk::Extent2D{ (uint32_t)width, (uint32_t)height };
        this->InitExternal(this->handle, format);
    }

    Image Image::CreateReference(const Image& image)
    {
        return Image(image.GetNativeHandle(), vk::Extent2D{ (uint32_t)image.GetWidth(), (uint32_t)image.GetHeight() }, image.GetFormat(), image.GetAllocator());
    }
}