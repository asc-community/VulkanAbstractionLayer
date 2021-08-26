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

#include "CommandBuffer.h"
#include "RenderPass.h"
#include "Image.h"
#include "Buffer.h"

namespace VulkanAbstractionLayer
{
    vk::Filter BlitFilterToNative(BlitFilter filter)
    {
        switch (filter)
        {
        case BlitFilter::NEAREST:
            return vk::Filter::eNearest;
        case BlitFilter::LINEAR:
            return vk::Filter::eLinear;
        case BlitFilter::CUBIC:
            return vk::Filter::eCubicEXT;
        default:
            assert(false);
            return vk::Filter::eNearest;
        }
    }

    vk::ShaderStageFlags PipelineTypeToShaderStages(vk::PipelineBindPoint pipelineType);

    void CommandBuffer::Begin()
    {
        vk::CommandBufferBeginInfo commandBufferBeginInfo;
        commandBufferBeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        this->handle.begin(commandBufferBeginInfo);
    }

    void CommandBuffer::End()
    {
        this->handle.end();
    }

    void CommandBuffer::BeginPass(const PassNative& pass)
    {
        if ((bool)pass.RenderPassHandle)
        {
            vk::RenderPassBeginInfo renderPassBeginInfo;
            renderPassBeginInfo
                .setRenderPass(pass.RenderPassHandle)
                .setRenderArea(pass.RenderArea)
                .setFramebuffer(pass.Framebuffer)
                .setClearValues(pass.ClearValues);

            this->handle.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
        }

        vk::Pipeline pipeline = pass.Pipeline;
        vk::PipelineLayout pipelineLayout = pass.PipelineLayout;
        vk::PipelineBindPoint pipelineType = pass.PipelineType;
        vk::DescriptorSet descriptorSet = pass.DescriptorSet;

        if ((bool)pipeline) this->handle.bindPipeline(pipelineType, pipeline);
        if ((bool)descriptorSet) this->handle.bindDescriptorSets(pipelineType, pipelineLayout, 0, descriptorSet, { });
    }

    void CommandBuffer::EndPass(const PassNative& pass)
    {
        if ((bool)pass.RenderPassHandle)
        {
            this->handle.endRenderPass();
        }
    }

    void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        this->handle.draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void CommandBuffer::BindIndexBufferInt32(const Buffer& indexBuffer)
    {
        this->handle.bindIndexBuffer(indexBuffer.GetNativeHandle(), 0, vk::IndexType::eUint32);
    }

    void CommandBuffer::BindIndexBufferInt16(const Buffer& indexBuffer)
    {
        this->handle.bindIndexBuffer(indexBuffer.GetNativeHandle(), 0, vk::IndexType::eUint16);
    }

    void CommandBuffer::SetViewport(const Viewport& viewport)
    {
        this->handle.setViewport(0, vk::Viewport{ 
            viewport.OffsetWidth, 
            viewport.OffsetHeight + viewport.Height, 
            viewport.Width, 
            -viewport.Height, // inverse viewport height to invert coordinate system
            viewport.MinDepth, 
            viewport.MaxDepth 
        });
    }

    void CommandBuffer::SetScissor(const Rect2D& scissor)
    {
        this->handle.setScissor(0, vk::Rect2D{
            vk::Offset2D{
                scissor.OffsetWidth,
                scissor.OffsetHeight
            },
            vk::Extent2D{
                scissor.Width,
                scissor.Height
            }
        });
    }

    void CommandBuffer::SetRenderArea(const Image& image)
    {
        this->SetViewport(Viewport{ 0.0f, 0.0f, (float)image.GetWidth(), (float)image.GetHeight(), 0.0f, 1.0f });
        this->SetScissor(Rect2D{ 0, 0, image.GetWidth(), image.GetHeight() });
    }

    void CommandBuffer::PushConstants(const PassNative& pass, const uint8_t* data, size_t size)
    {
        constexpr size_t MaxPushConstantByteSize = 128;
        std::array<uint8_t, MaxPushConstantByteSize> pushConstants = { };

        std::memcpy(pushConstants.data(), data, size);

        this->handle.pushConstants(
            pass.PipelineLayout,
            PipelineTypeToShaderStages(pass.PipelineType),
            0,
            pushConstants.size(),
            pushConstants.data()
        );
    }

    void CommandBuffer::Dispatch(uint32_t x, uint32_t y, uint32_t z)
    {
        this->handle.dispatch(x, y, z);
    }

    void CommandBuffer::CopyImage(const ImageInfo& source, const ImageInfo& distance)
    {
        auto sourceRange = GetDefaultImageSubresourceRange(source.Resource.get());
        auto distanceRange = GetDefaultImageSubresourceRange(distance.Resource.get());

        std::array<vk::ImageMemoryBarrier, 2> barriers;
        size_t barrierCount = 0;

        vk::ImageMemoryBarrier toTransferSrcBarrier;
        toTransferSrcBarrier
            .setSrcAccessMask(ImageUsageToAccessFlags(source.Usage))
            .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
            .setOldLayout(ImageUsageToImageLayout(source.Usage))
            .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(source.Resource.get().GetNativeHandle())
            .setSubresourceRange(sourceRange);

        vk::ImageMemoryBarrier toTransferDstBarrier;
        toTransferDstBarrier
            .setSrcAccessMask(ImageUsageToAccessFlags(distance.Usage))
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setOldLayout(ImageUsageToImageLayout(distance.Usage))
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(distance.Resource.get().GetNativeHandle())
            .setSubresourceRange(distanceRange);

        if (source.Usage != ImageUsage::TRANSFER_SOURCE)
            barriers[barrierCount++] = toTransferSrcBarrier;
        if (distance.Usage != ImageUsage::TRANSFER_DISTINATION)
            barriers[barrierCount++] = toTransferDstBarrier;
        
        if (barrierCount > 0)
        {
            this->handle.pipelineBarrier(
                ImageUsageToPipelineStage(source.Usage) | ImageUsageToPipelineStage(distance.Usage),
                vk::PipelineStageFlagBits::eTransfer,
                { }, // dependency flags
                0, nullptr, // memory barriers
                0, nullptr, // buffer barriers
                barrierCount, barriers.data()
            );
        }

        auto sourceLayers = GetDefaultImageSubresourceLayers(source.Resource.get(), source.MipLevel, source.Layer);
        auto distanceLayers = GetDefaultImageSubresourceLayers(distance.Resource.get(), distance.MipLevel, distance.Layer);

        vk::ImageCopy imageCopyInfo;
        imageCopyInfo
            .setSrcOffset(0)
            .setDstOffset(0)
            .setSrcSubresource(sourceLayers)
            .setDstSubresource(distanceLayers)
            .setExtent(vk::Extent3D{ 
                distance.Resource.get().GetMipLevelWidth(distance.MipLevel), 
                distance.Resource.get().GetMipLevelHeight(distance.MipLevel),
                1 
            });

        this->handle.copyImage(
            source.Resource.get().GetNativeHandle(), 
            vk::ImageLayout::eTransferSrcOptimal, 
            distance.Resource.get().GetNativeHandle(), 
            vk::ImageLayout::eTransferDstOptimal, 
            imageCopyInfo
        );
    }

    void CommandBuffer::CopyImageToBuffer(const ImageInfo& source, const BufferInfo& distance)
    {
        if (source.Usage != ImageUsage::TRANSFER_SOURCE)
        {
            auto sourceRange = GetDefaultImageSubresourceRange(source.Resource.get());

            vk::ImageMemoryBarrier toTransferSrcBarrier;
            toTransferSrcBarrier
                .setSrcAccessMask(ImageUsageToAccessFlags(source.Usage))
                .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
                .setOldLayout(ImageUsageToImageLayout(source.Usage))
                .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
                .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                .setImage(source.Resource.get().GetNativeHandle())
                .setSubresourceRange(sourceRange);

            this->handle.pipelineBarrier(
                ImageUsageToPipelineStage(source.Usage),
                vk::PipelineStageFlagBits::eTransfer,
                { }, // dependency flags
                { }, // memory barriers
                { }, // buffer barriers
                toTransferSrcBarrier
            );
        }

        auto sourceLayers = GetDefaultImageSubresourceLayers(source.Resource.get(), source.MipLevel, source.Layer);

        vk::BufferImageCopy imageToBufferCopyInfo;
        imageToBufferCopyInfo
            .setBufferOffset(distance.Offset)
            .setBufferImageHeight(0)
            .setBufferRowLength(0)
            .setImageSubresource(sourceLayers)
            .setImageOffset(vk::Offset3D{ 0, 0, 0 })
            .setImageExtent(vk::Extent3D{ 
                source.Resource.get().GetMipLevelWidth(source.MipLevel), 
                source.Resource.get().GetMipLevelHeight(source.MipLevel), 
                1 
            });

        this->handle.copyImageToBuffer(
            source.Resource.get().GetNativeHandle(), 
            vk::ImageLayout::eTransferSrcOptimal, 
            distance.Resource.get().GetNativeHandle(), imageToBufferCopyInfo);
    }

    void CommandBuffer::CopyBufferToImage(const BufferInfo& source, const ImageInfo& distance)
    {
        if (distance.Usage != ImageUsage::TRANSFER_DISTINATION)
        {
            auto distanceRange = GetDefaultImageSubresourceRange(distance.Resource.get());

            vk::ImageMemoryBarrier toTransferDstBarrier;
            toTransferDstBarrier
                .setSrcAccessMask(ImageUsageToAccessFlags(distance.Usage))
                .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
                .setOldLayout(ImageUsageToImageLayout(distance.Usage))
                .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
                .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                .setImage(distance.Resource.get().GetNativeHandle())
                .setSubresourceRange(distanceRange);

            this->handle.pipelineBarrier(
                ImageUsageToPipelineStage(distance.Usage),
                vk::PipelineStageFlagBits::eTransfer,
                { }, // dependency flags
                { }, // memory barriers
                { }, // buffer barriers
                toTransferDstBarrier
            );
        }

        auto distanceLayers = GetDefaultImageSubresourceLayers(distance.Resource.get(), distance.MipLevel, distance.Layer);

        vk::BufferImageCopy bufferToImageCopyInfo;
        bufferToImageCopyInfo
            .setBufferOffset(source.Offset)
            .setBufferImageHeight(0)
            .setBufferRowLength(0)
            .setImageSubresource(distanceLayers)
            .setImageOffset(vk::Offset3D{ 0, 0, 0 })
            .setImageExtent(vk::Extent3D{ 
                distance.Resource.get().GetMipLevelWidth(distance.MipLevel), 
                distance.Resource.get().GetMipLevelHeight(distance.MipLevel), 
                1 
            });

        this->handle.copyBufferToImage(
            source.Resource.get().GetNativeHandle(), 
            distance.Resource.get().GetNativeHandle(), 
            vk::ImageLayout::eTransferDstOptimal, 
            bufferToImageCopyInfo
        );
    }

    void CommandBuffer::CopyBuffer(const BufferInfo& source, const BufferInfo& distance, size_t byteSize)
    {
        assert(source.Resource.get().GetByteSize() >= source.Offset + byteSize);
        assert(distance.Resource.get().GetByteSize() >= distance.Offset + byteSize);

        vk::BufferCopy bufferCopyInfo;
        bufferCopyInfo
            .setDstOffset(distance.Offset)
            .setSize(byteSize)
            .setSrcOffset(source.Offset);

        this->handle.copyBuffer(source.Resource.get().GetNativeHandle(), distance.Resource.get().GetNativeHandle(), bufferCopyInfo);
    }

    void CommandBuffer::BlitImage(const Image& source, ImageUsage::Bits sourceUsage, const Image& distance, ImageUsage::Bits distanceUsage, BlitFilter filter)
    {
        auto sourceRange = GetDefaultImageSubresourceRange(source);
        auto distanceRange = GetDefaultImageSubresourceRange(distance);

        std::array<vk::ImageMemoryBarrier, 2> barriers;
        size_t barrierCount = 0;

        vk::ImageMemoryBarrier toTransferSrcBarrier;
        toTransferSrcBarrier
            .setSrcAccessMask(ImageUsageToAccessFlags(sourceUsage))
            .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
            .setOldLayout(ImageUsageToImageLayout(sourceUsage))
            .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(source.GetNativeHandle())
            .setSubresourceRange(sourceRange);

        vk::ImageMemoryBarrier toTransferDstBarrier;
        toTransferDstBarrier
            .setSrcAccessMask(ImageUsageToAccessFlags(distanceUsage))
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setOldLayout(ImageUsageToImageLayout(distanceUsage))
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(distance.GetNativeHandle())
            .setSubresourceRange(distanceRange);

        if (sourceUsage != ImageUsage::TRANSFER_SOURCE)
            barriers[barrierCount++] = toTransferSrcBarrier;
        if (distanceUsage != ImageUsage::TRANSFER_DISTINATION)
            barriers[barrierCount++] = toTransferDstBarrier;

        if (barrierCount > 0)
        {
            this->handle.pipelineBarrier(
                ImageUsageToPipelineStage(sourceUsage) | ImageUsageToPipelineStage(distanceUsage),
                vk::PipelineStageFlagBits::eTransfer,
                { }, // dependency flags
                0, nullptr, // memory barriers
                0, nullptr, // buffer barriers
                barrierCount, barriers.data()
            );
        }

        auto sourceLayers = GetDefaultImageSubresourceLayers(source);
        auto distanceLayers = GetDefaultImageSubresourceLayers(distance);

        vk::ImageBlit imageBlitInfo;
        imageBlitInfo
            .setSrcOffsets({
                vk::Offset3D{ 0, 0, 0 },
                vk::Offset3D{ (int32_t)source.GetWidth(), (int32_t)source.GetHeight(), 1 }
            })
            .setDstOffsets({
                vk::Offset3D{ 0, 0, 0 },
                vk::Offset3D{ (int32_t)distance.GetWidth(), (int32_t)distance.GetHeight(), 1 }
            })
            .setSrcSubresource(sourceLayers)
            .setDstSubresource(distanceLayers);

        this->handle.blitImage(
            source.GetNativeHandle(),
            vk::ImageLayout::eTransferSrcOptimal,
            distance.GetNativeHandle(),
            vk::ImageLayout::eTransferDstOptimal,
            imageBlitInfo,
            BlitFilterToNative(filter)
        );
    }

    void CommandBuffer::GenerateMipLevels(const Image& image, ImageUsage::Bits initialUsage, BlitFilter filter)
    {
        if (image.GetMipLevelCount() < 2) return;

        auto sourceRange = GetDefaultImageSubresourceRange(image);
        auto distanceRange = GetDefaultImageSubresourceRange(image);
        auto sourceLayers = GetDefaultImageSubresourceLayers(image);
        auto distanceLayers = GetDefaultImageSubresourceLayers(image);
        auto sourceUsage = initialUsage;
        auto distanceUsage = initialUsage;
        uint32_t sourceWidth = image.GetWidth();
        uint32_t sourceHeight = image.GetHeight();
        uint32_t distanceWidth = image.GetWidth();
        uint32_t distanceHeight = image.GetHeight();

        for (size_t i = 0; i + 1 < image.GetMipLevelCount(); i++)
        {
            sourceWidth = distanceWidth;
            sourceHeight = distanceHeight;
            distanceWidth = std::max(sourceWidth / 2, 1u);
            distanceHeight = std::max(sourceHeight / 2, 1u);
            
            sourceLayers.setMipLevel(i);
            sourceRange.setBaseMipLevel(i);
            sourceRange.setLevelCount(1);

            distanceLayers.setMipLevel(i + 1);
            distanceRange.setBaseMipLevel(i + 1);
            distanceRange.setLevelCount(1);

            sourceUsage = distanceUsage;
            distanceUsage = ImageUsage::UNKNOWN;

            std::array<vk::ImageMemoryBarrier, 2> imageBarriers;
            imageBarriers[0] // to transfer source
                .setSrcAccessMask(ImageUsageToAccessFlags(sourceUsage))
                .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
                .setOldLayout(ImageUsageToImageLayout(sourceUsage))
                .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
                .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                .setImage(image.GetNativeHandle())
                .setSubresourceRange(sourceRange);

            imageBarriers[1] // to transfer distance
                .setSrcAccessMask(ImageUsageToAccessFlags(distanceUsage))
                .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
                .setOldLayout(ImageUsageToImageLayout(distanceUsage))
                .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
                .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                .setImage(image.GetNativeHandle())
                .setSubresourceRange(distanceRange);

            this->handle.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer,
                vk::PipelineStageFlagBits::eTransfer,
                { }, // dependencies
                { }, // memory barriers
                { }, // buffer barriers,
                imageBarriers
            );

            vk::ImageBlit imageBlitInfo;
            imageBlitInfo
                .setSrcOffsets({
                    vk::Offset3D{ 0, 0, 0 },
                    vk::Offset3D{ (int32_t)sourceWidth, (int32_t)sourceHeight, 1 }
                })
                .setDstOffsets({
                    vk::Offset3D{ 0, 0, 0 },
                    vk::Offset3D{ (int32_t)distanceWidth, (int32_t)distanceHeight, 1 }
                })
                .setSrcSubresource(sourceLayers)
                .setDstSubresource(distanceLayers);

            this->handle.blitImage(
                image.GetNativeHandle(),
                vk::ImageLayout::eTransferSrcOptimal,
                image.GetNativeHandle(),
                vk::ImageLayout::eTransferDstOptimal,
                imageBlitInfo,
                BlitFilterToNative(filter)
            );

        }

        auto mipLevelsSubresourceRange = GetDefaultImageSubresourceRange(image);
        mipLevelsSubresourceRange.setLevelCount(mipLevelsSubresourceRange.levelCount - 1);
        vk::ImageMemoryBarrier mipLevelsTransfer;
        mipLevelsTransfer
            .setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(image.GetNativeHandle())
            .setSubresourceRange(mipLevelsSubresourceRange);

        this->handle.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            { }, // dependecies
            { }, // memory barriers
            { }, // buffer barriers
            mipLevelsTransfer
        );
    }

    static vk::ImageMemoryBarrier GetImageMemoryBarrier(const Image& image, ImageUsage::Bits oldLayout, ImageUsage::Bits newLayout)
    {
        auto subresourceRange = GetDefaultImageSubresourceRange(image);
        vk::ImageMemoryBarrier barrier;
        barrier
            .setSrcAccessMask(ImageUsageToAccessFlags(oldLayout))
            .setDstAccessMask(ImageUsageToAccessFlags(newLayout))
            .setOldLayout(ImageUsageToImageLayout(oldLayout))
            .setNewLayout(ImageUsageToImageLayout(newLayout))
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(image.GetNativeHandle())
            .setSubresourceRange(subresourceRange);

        return barrier;
    }
    
    void CommandBuffer::TransferLayout(const Image& image, ImageUsage::Bits oldLayout, ImageUsage::Bits newLayout)
    {
        auto barrier = GetImageMemoryBarrier(image, oldLayout, newLayout);

        this->handle.pipelineBarrier(
            ImageUsageToPipelineStage(oldLayout),
            ImageUsageToPipelineStage(newLayout),
            { }, // dependecies
            { }, // memory barriers
            { }, // buffer barriers
            barrier
        );
    }

    void CommandBuffer::TransferLayout(ArrayView<ImageReference> images, ImageUsage::Bits oldLayout, ImageUsage::Bits newLayout)
    {
        std::vector<vk::ImageMemoryBarrier> barriers;
        barriers.reserve(images.size());

        for (const auto& image : images)
        {
            barriers.push_back(GetImageMemoryBarrier(image.get(), oldLayout, newLayout));
        }

        this->handle.pipelineBarrier(
            ImageUsageToPipelineStage(oldLayout),
            ImageUsageToPipelineStage(newLayout),
            { }, // dependecies
            { }, // memory barriers
            { }, // buffer barriers
            barriers
        );
    }
}