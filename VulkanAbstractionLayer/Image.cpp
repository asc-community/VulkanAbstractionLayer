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
#include "VulkanContext.h"
#include <cassert>

namespace VulkanAbstractionLayer
{
    vk::ImageAspectFlags GetImageAspectByFormat(Format format)
    {
        switch (format)
        {
        case Format::D16_UNORM:
            return vk::ImageAspectFlagBits::eDepth;
        case Format::X8D24_UNORM_PACK_32:
            return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        case Format::D32_SFLOAT:
            return vk::ImageAspectFlagBits::eDepth;
        case Format::D16_UNORM_S8_UINT:
            return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        case Format::D24_UNORM_S8_UINT:
            return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        case Format::D32_SFLOAT_S8_UINT:
            return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        default:
            return vk::ImageAspectFlagBits::eColor;
        }
    }

    void Image::Destroy()
    {
        if ((bool)this->handle)
        {
            if ((bool)this->allocation) // allocated
            {
                DeallocateImage(this->handle, this->allocation);
            }
            GetCurrentVulkanContext().GetDevice().destroyImageView(this->view);
            this->handle = vk::Image{ };
            this->view = vk::ImageView{ };
            this->extent = vk::Extent2D{ 0u, 0u };
        }
    }

    void Image::InitView(const vk::Image& image, Format format)
    {
        this->handle = image;
        this->format = format;

        vk::ImageSubresourceRange subresourceRange{
            GetImageAspectByFormat(format),
            0, // base mip level
            1, // level count
            0, // base layer
            1  // layer count
        };

        vk::ImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo
            .setImage(this->handle)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(ToNativeFormat(format))
            .setComponents(vk::ComponentMapping{
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity
            })
            .setSubresourceRange(subresourceRange);

        this->view = GetCurrentVulkanContext().GetDevice().createImageView(imageViewCreateInfo);
    }

    Image::Image(uint32_t width, uint32_t height, Format format, vk::ImageUsageFlags usage, MemoryUsage memoryUsage)
    {
        this->Init(width, height, format, usage, memoryUsage);
    }

    Image::Image(vk::Image image, uint32_t width, uint32_t height, Format format)
    {
        this->extent = vk::Extent2D{ width, height };
        this->allocation = { }; // image is external resource
        this->InitView(image, format);
    }

    Image::Image(Image&& other) noexcept
    {
        this->handle = other.handle;
        this->view = other.view;
        this->extent = other.extent;
        this->format = other.format;
        this->allocation = other.allocation;

        other.handle = vk::Image{ };
        other.view = vk::ImageView{ };
        other.extent = vk::Extent2D{ 0u, 0u };
        other.format = Format::UNDEFINED;
        other.allocation = { };
    }

    Image& Image::operator=(Image&& other) noexcept
    {
        this->Destroy();

        this->handle = other.handle;
        this->view = other.view;
        this->extent = other.extent;
        this->format = other.format;
        this->allocation = other.allocation;

        other.handle = vk::Image{ };
        other.view = vk::ImageView{ };
        other.extent = vk::Extent2D{ 0u, 0u };
        other.format = Format::UNDEFINED;
        other.allocation = { };

        return *this;
    }

    Image::~Image()
    {
        this->Destroy();
    }

    void Image::Init(uint32_t width, uint32_t height, Format format, vk::ImageUsageFlags usage, MemoryUsage memoryUsage)
    {
        vk::ImageCreateInfo imageCreateInfo;
        imageCreateInfo
            .setImageType(vk::ImageType::e2D)
            .setFormat(ToNativeFormat(format))
            .setExtent(vk::Extent3D{ width, height, 1 })
            .setSamples(vk::SampleCountFlagBits::e1)
            .setMipLevels(1)
            .setArrayLayers(1)
            .setTiling(vk::ImageTiling::eOptimal)
            .setUsage(usage)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setInitialLayout(vk::ImageLayout::eUndefined);
        
        this->allocation = AllocateImage(imageCreateInfo, memoryUsage, &this->handle);
        this->extent = vk::Extent2D{ (uint32_t)width, (uint32_t)height };
        this->InitView(this->handle, format);
    }
}