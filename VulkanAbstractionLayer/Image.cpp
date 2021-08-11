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
    vk::ImageAspectFlags ImageFormatToImageAspect(Format format)
    {
        switch (format)
        {
        case Format::D16_UNORM:
            return vk::ImageAspectFlagBits::eDepth;
        case Format::X8D24_UNORM_PACK_32:
            return vk::ImageAspectFlagBits::eDepth;
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

    vk::ImageLayout ImageUsageToImageLayout(ImageUsage::Bits layout)
    {
        switch (layout)
        {
        case VulkanAbstractionLayer::ImageUsage::UNKNOWN:
            return vk::ImageLayout::eUndefined;
        case VulkanAbstractionLayer::ImageUsage::TRANSFER_SOURCE:
            return vk::ImageLayout::eTransferSrcOptimal;
        case VulkanAbstractionLayer::ImageUsage::TRANSFER_DISTINATION:
            return vk::ImageLayout::eTransferDstOptimal;
        case VulkanAbstractionLayer::ImageUsage::SHADER_READ:
            return vk::ImageLayout::eShaderReadOnlyOptimal;
        case VulkanAbstractionLayer::ImageUsage::STORAGE:
            return vk::ImageLayout::eShaderReadOnlyOptimal; // TODO: what if writes?
        case VulkanAbstractionLayer::ImageUsage::COLOR_ATTACHMENT:
            return vk::ImageLayout::eColorAttachmentOptimal;
        case VulkanAbstractionLayer::ImageUsage::DEPTH_SPENCIL_ATTACHMENT:
            return vk::ImageLayout::eDepthStencilAttachmentOptimal;
        case VulkanAbstractionLayer::ImageUsage::INPUT_ATTACHMENT:
            return vk::ImageLayout::eAttachmentOptimalKHR; // TODO: is it ok?
        case VulkanAbstractionLayer::ImageUsage::FRAGMENT_SHADING_RATE_ATTACHMENT:
            return vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR;
        default:
            assert(false);
            return vk::ImageLayout::eUndefined;
        }
    }

    vk::AccessFlags ImageUsageToAccessFlags(ImageUsage::Bits layout)
    {
        switch (layout)
        {
        case ImageUsage::UNKNOWN:
            return vk::AccessFlags{ };
        case ImageUsage::TRANSFER_SOURCE:
            return vk::AccessFlagBits::eTransferRead;
        case ImageUsage::TRANSFER_DISTINATION:
            return vk::AccessFlagBits::eTransferWrite;
        case ImageUsage::SHADER_READ:
            return vk::AccessFlagBits::eShaderRead;
        case ImageUsage::STORAGE:
            return vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite; // TODO: what if storage is not read or write?
        case ImageUsage::COLOR_ATTACHMENT:
            return vk::AccessFlagBits::eColorAttachmentWrite;
        case ImageUsage::DEPTH_SPENCIL_ATTACHMENT:
            return vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        case ImageUsage::INPUT_ATTACHMENT:
            return vk::AccessFlagBits::eInputAttachmentRead;
        case ImageUsage::FRAGMENT_SHADING_RATE_ATTACHMENT:
            return vk::AccessFlagBits::eFragmentShadingRateAttachmentReadKHR;
        default:
            assert(false);
            return vk::AccessFlags{ };
        }
    }

    vk::PipelineStageFlags ImageUsageToPipelineStage(ImageUsage::Bits layout)
    {
        switch (layout)
        {
        case VulkanAbstractionLayer::ImageUsage::UNKNOWN:
            return vk::PipelineStageFlagBits::eTopOfPipe;
        case VulkanAbstractionLayer::ImageUsage::TRANSFER_SOURCE:
            return vk::PipelineStageFlagBits::eTransfer;
        case VulkanAbstractionLayer::ImageUsage::TRANSFER_DISTINATION:
            return vk::PipelineStageFlagBits::eTransfer;
        case VulkanAbstractionLayer::ImageUsage::SHADER_READ:
            return vk::PipelineStageFlagBits::eFragmentShader; // TODO: whats for vertex shader reads?
        case VulkanAbstractionLayer::ImageUsage::STORAGE:
            return vk::PipelineStageFlagBits::eFragmentShader; // TODO: whats for vertex shader reads?
        case VulkanAbstractionLayer::ImageUsage::COLOR_ATTACHMENT:
            return vk::PipelineStageFlagBits::eColorAttachmentOutput;
        case VulkanAbstractionLayer::ImageUsage::DEPTH_SPENCIL_ATTACHMENT:
            return vk::PipelineStageFlagBits::eEarlyFragmentTests; // TODO: whats for late fragment test?
        case VulkanAbstractionLayer::ImageUsage::INPUT_ATTACHMENT:
            return vk::PipelineStageFlagBits::eFragmentShader; // TODO: check if at least works
        case VulkanAbstractionLayer::ImageUsage::FRAGMENT_SHADING_RATE_ATTACHMENT:
            return vk::PipelineStageFlagBits::eFragmentShadingRateAttachmentKHR;
        default:
            assert(false);
            return vk::PipelineStageFlags{ };
        }
    }

    vk::ImageSubresourceLayers GetDefaultImageSubresourceLayers(const Image& image)
    {
        auto subresourceRange = GetDefaultImageSubresourceRange(image);
        return vk::ImageSubresourceLayers{
            subresourceRange.aspectMask,
            subresourceRange.baseMipLevel,
            subresourceRange.baseArrayLayer,
            subresourceRange.layerCount
        };
    }

    vk::ImageSubresourceLayers GetDefaultImageSubresourceLayers(const Image& image, uint32_t mipLevel)
    {
        auto subresourceRange = GetDefaultImageSubresourceRange(image);
        return vk::ImageSubresourceLayers{
            subresourceRange.aspectMask,
            mipLevel,
            subresourceRange.baseArrayLayer,
            subresourceRange.layerCount
        };
    }

    vk::ImageSubresourceRange GetDefaultImageSubresourceRange(const Image& image)
    {
        return vk::ImageSubresourceRange{
            ImageFormatToImageAspect(image.GetFormat()),
            0, // base mip level
            image.GetMipLevelCount(),
            0, // base layer
            1  // layer count
        };
    }

    void Image::Destroy()
    {
        if ((bool)this->handle)
        {
            if ((bool)this->allocation) // allocated
            {
                DeallocateImage(this->handle, this->allocation);
            }

            GetCurrentVulkanContext().GetDevice().destroyImageView(this->imageViews.NativeView);
            if ((bool)this->imageViews.DepthOnlyView) 
                GetCurrentVulkanContext().GetDevice().destroyImageView(this->imageViews.DepthOnlyView);
            if ((bool)this->imageViews.StencilOnlyView)
                GetCurrentVulkanContext().GetDevice().destroyImageView(this->imageViews.StencilOnlyView);

            this->handle = vk::Image{ };
            this->imageViews = { };
            this->extent = vk::Extent2D{ 0u, 0u };
        }
    }

    void Image::InitViews(const vk::Image& image, Format format)
    {
        this->handle = image;
        this->format = format;

        auto& Vulkan = GetCurrentVulkanContext();
        auto subresourceRange = GetDefaultImageSubresourceRange(*this);

        vk::ImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo
            .setImage(this->handle)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(ToNative(format))
            .setComponents(vk::ComponentMapping{
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity
            })
            .setSubresourceRange(subresourceRange);

        this->imageViews.NativeView = Vulkan.GetDevice().createImageView(imageViewCreateInfo);

        auto depthSubresourceRange = GetDefaultImageSubresourceRange(*this);
        depthSubresourceRange.aspectMask &= vk::ImageAspectFlagBits::eDepth;
        if (depthSubresourceRange.aspectMask != vk::ImageAspectFlags{ })
        {
            imageViewCreateInfo.setSubresourceRange(depthSubresourceRange);
            this->imageViews.DepthOnlyView = GetCurrentVulkanContext().GetDevice().createImageView(imageViewCreateInfo);
        }

        auto stencilSubresourceRange = GetDefaultImageSubresourceRange(*this);
        stencilSubresourceRange.aspectMask &= vk::ImageAspectFlagBits::eStencil;
        if (stencilSubresourceRange.aspectMask != vk::ImageAspectFlags{ })
        {
            imageViewCreateInfo.setSubresourceRange(stencilSubresourceRange);
            this->imageViews.StencilOnlyView = GetCurrentVulkanContext().GetDevice().createImageView(imageViewCreateInfo);
        }
    }

    Image::Image(uint32_t width, uint32_t height, Format format, ImageUsage::Value usage, MemoryUsage memoryUsage, Mipmapping mipmapping)
    {
        this->Init(width, height, format, usage, memoryUsage, mipmapping);
    }

    Image::Image(vk::Image image, uint32_t width, uint32_t height, Format format)
    {
        this->extent = vk::Extent2D{ width, height };
        this->allocation = { }; // image is external resource
        this->InitViews(image, format);
    }

    Image::Image(Image&& other) noexcept
    {
        this->handle = other.handle;
        this->imageViews = other.imageViews;
        this->extent = other.extent;
        this->format = other.format;
        this->allocation = other.allocation;
        this->mipLevelCount = other.mipLevelCount;

        other.handle = vk::Image{ };
        other.imageViews = { };
        other.extent = vk::Extent2D{ 0u, 0u };
        other.format = Format::UNDEFINED;
        other.allocation = { };
        other.mipLevelCount = 1;
    }

    Image& Image::operator=(Image&& other) noexcept
    {
        this->Destroy();

        this->handle = other.handle;
        this->imageViews = other.imageViews;
        this->extent = other.extent;
        this->format = other.format;
        this->allocation = other.allocation;
        this->mipLevelCount = other.mipLevelCount;

        other.handle = vk::Image{ };
        other.imageViews = { };
        other.extent = vk::Extent2D{ 0u, 0u };
        other.format = Format::UNDEFINED;
        other.allocation = { };
        other.mipLevelCount = 1;

        return *this;
    }

    Image::~Image()
    {
        this->Destroy();
    }

    void Image::Init(uint32_t width, uint32_t height, Format format, ImageUsage::Value usage, MemoryUsage memoryUsage, Mipmapping mipmapping)
    {
        if (mipmapping == Mipmapping::USE_MIPMAPS)
            this->mipLevelCount = (uint32_t)std::floor(std::log2(std::max(width, height))) + 1;
        else
            this->mipLevelCount = 1;

        vk::ImageCreateInfo imageCreateInfo;
        imageCreateInfo
            .setImageType(vk::ImageType::e2D)
            .setFormat(ToNative(format))
            .setExtent(vk::Extent3D{ width, height, 1 })
            .setSamples(vk::SampleCountFlagBits::e1)
            .setMipLevels(this->GetMipLevelCount())
            .setArrayLayers(1)
            .setTiling(vk::ImageTiling::eOptimal)
            .setUsage((vk::ImageUsageFlags)usage)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setInitialLayout(vk::ImageLayout::eUndefined);
        
        this->extent = vk::Extent2D{ (uint32_t)width, (uint32_t)height };
        this->allocation = AllocateImage(imageCreateInfo, memoryUsage, &this->handle);
        this->InitViews(this->handle, format);
    }

    vk::ImageView Image::GetNativeView(ImageView view) const
    {
        switch (view)
        {
        case VulkanAbstractionLayer::ImageView::NATIVE:
            return this->imageViews.NativeView;
        case VulkanAbstractionLayer::ImageView::DEPTH:
            return this->imageViews.DepthOnlyView;
        case VulkanAbstractionLayer::ImageView::STENCIL:
            return this->imageViews.StencilOnlyView;
        default:
            assert(false);
            return this->imageViews.NativeView;
        }
    }

    uint32_t Image::GetMipLevelWidth(uint32_t mipLevel) const
    {
        auto safeWidth = std::max(this->GetWidth(), 1u << mipLevel);
        return safeWidth >> mipLevel;
    }

    uint32_t Image::GetMipLevelHeight(uint32_t mipLevel) const
    {
        auto safeHeight = std::max(this->GetHeight(), 1u << mipLevel);
        return safeHeight >> mipLevel;
    }
}