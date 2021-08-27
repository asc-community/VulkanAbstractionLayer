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

#include "VulkanMemoryAllocator.h"
#include "ShaderReflection.h"

#include <vulkan/vulkan.hpp>

struct VkImage_T;
using VkImage = VkImage_T*;

struct VkImageView_T;
using VkImageView = VkImageView_T*;

namespace VulkanAbstractionLayer
{
    struct ImageUsage
    {
        using Value = uint32_t;

        enum Bits : Value
        {
            UNKNOWN = (Value)vk::ImageUsageFlagBits{ },
            TRANSFER_SOURCE = (Value)vk::ImageUsageFlagBits::eTransferSrc,
            TRANSFER_DISTINATION = (Value)vk::ImageUsageFlagBits::eTransferDst,
            SHADER_READ = (Value)vk::ImageUsageFlagBits::eSampled,
            STORAGE = (Value)vk::ImageUsageFlagBits::eStorage,
            COLOR_ATTACHMENT = (Value)vk::ImageUsageFlagBits::eColorAttachment,
            DEPTH_SPENCIL_ATTACHMENT = (Value)vk::ImageUsageFlagBits::eDepthStencilAttachment,
            INPUT_ATTACHMENT = (Value)vk::ImageUsageFlagBits::eInputAttachment,
            FRAGMENT_SHADING_RATE_ATTACHMENT = (Value)vk::ImageUsageFlagBits::eFragmentShadingRateAttachmentKHR,
        };
    };

    enum class ImageView
    {
        NATIVE = 0,
        DEPTH_SPENCIL = 0,
        CUBEMAP,
        DEPTH,
        STENCIL,
    };

    struct ImageOptions
    {
        using Value = uint32_t;

        enum Bits : Value
        {
            DEFAULT = 0,
            MIPMAPS = 1 << 0,
            CUBEMAP = 1 << 1,
        };
    };

    vk::ImageAspectFlags ImageFormatToImageAspect(Format format);
    vk::ImageLayout ImageUsageToImageLayout(ImageUsage::Bits usage);
    vk::AccessFlags ImageUsageToAccessFlags(ImageUsage::Bits usage);
    vk::PipelineStageFlags ImageUsageToPipelineStage(ImageUsage::Bits usage);

    class Image
    {
        struct ImageViews
        {
            vk::ImageView NativeView;
            vk::ImageView CubemapView;
            vk::ImageView DepthOnlyView;
            vk::ImageView StencilOnlyView;
        };

        vk::Image handle;
        ImageViews imageViews;
        vk::Extent2D extent = { 0u, 0u };
        uint32_t mipLevelCount = 1;
        uint32_t layerCount = 1;
        Format format = Format::UNDEFINED;
        VmaAllocation allocation = { };

        void Destroy();
        void InitViews(const vk::Image& image, Format format);
    public:
        Image() = default;
        Image(uint32_t width, uint32_t height, Format format, ImageUsage::Value usage, MemoryUsage memoryUsage, ImageOptions::Value options);
        Image(vk::Image image, uint32_t width, uint32_t height, Format format);
        Image(Image&& other) noexcept;
        Image& operator=(Image&& other) noexcept;
        ~Image();

        void Init(uint32_t width, uint32_t height, Format format, ImageUsage::Value usage, MemoryUsage memoryUsage, ImageOptions::Value mipmapping);

        vk::ImageView GetNativeView(ImageView view) const;
        uint32_t GetMipLevelWidth(uint32_t mipLevel) const;
        uint32_t GetMipLevelHeight(uint32_t mipLevel) const;

        vk::Image GetNativeHandle() const { return this->handle; }
        Format GetFormat() const { return this->format; }
        uint32_t GetWidth() const { return this->extent.width; }
        uint32_t GetHeight() const { return this->extent.height; }
        uint32_t GetMipLevelCount() const { return this->mipLevelCount; }
        uint32_t GetLayerCount() const { return this->layerCount; }
    };

    vk::ImageSubresourceLayers GetDefaultImageSubresourceLayers(const Image& image);
    vk::ImageSubresourceLayers GetDefaultImageSubresourceLayers(const Image& image, uint32_t mipLevel, uint32_t layer);
    vk::ImageSubresourceRange GetDefaultImageSubresourceRange(const Image& image);

    using ImageReference = std::reference_wrapper<const Image>;
}