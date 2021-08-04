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

    void CommandBuffer::BeginRenderPass(const RenderPassNative& renderPass)
    {
        vk::RenderPassBeginInfo renderPassBeginInfo;
        renderPassBeginInfo
            .setRenderPass(renderPass.RenderPassHandle)
            .setRenderArea(renderPass.RenderArea)
            .setFramebuffer(renderPass.Framebuffer)
            .setClearValues(renderPass.ClearValues);

        this->handle.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

        vk::Pipeline graphicPipeline = renderPass.Pipeline;
        vk::PipelineLayout pipelineLayout = renderPass.PipelineLayout;
        vk::DescriptorSet descriptorSet = renderPass.DescriptorSet;
        if ((bool)graphicPipeline) this->handle.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicPipeline);
        if ((bool)descriptorSet) this->handle.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSet, { });
    }

    void CommandBuffer::EndRenderPass()
    {
        this->handle.endRenderPass();
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

    void CommandBuffer::PushConstants(const RenderPassNative& renderPass, const uint8_t* data, size_t size)
    {
        this->GetNativeHandle().pushConstants(
            renderPass.PipelineLayout,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            0,
            size,
            data
        );
    }

    void CommandBuffer::CopyImage(const Image& source, ImageUsage::Bits sourceUsage, const Image& distance, ImageUsage::Bits distanceUsage)
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

        vk::ImageCopy imageCopyInfo;
        imageCopyInfo
            .setSrcOffset(0)
            .setDstOffset(0)
            .setSrcSubresource(sourceLayers)
            .setDstSubresource(distanceLayers)
            .setExtent(vk::Extent3D{ (uint32_t)distance.GetWidth(), (uint32_t)distance.GetHeight(), 1 });

        this->handle.copyImage(source.GetNativeHandle(), vk::ImageLayout::eTransferSrcOptimal, distance.GetNativeHandle(), vk::ImageLayout::eTransferDstOptimal, imageCopyInfo);
    }

    void CommandBuffer::CopyImageToBuffer(const Image& source, ImageUsage::Bits sourceUsage, const Buffer& distance)
    {
        this->CopyImageToBuffer(source, sourceUsage, distance, 0, distance.GetByteSize());
    }

    void CommandBuffer::CopyImageToBuffer(const Image& source, ImageUsage::Bits sourceUsage, const Buffer& distance, size_t distanceOffset, size_t byteSize)
    {
        if (sourceUsage != ImageUsage::TRANSFER_SOURCE)
        {
            auto sourceRange = GetDefaultImageSubresourceRange(source);

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

            this->handle.pipelineBarrier(
                ImageUsageToPipelineStage(sourceUsage),
                vk::PipelineStageFlagBits::eTransfer,
                { }, // dependency flags
                { }, // memory barriers
                { }, // buffer barriers
                toTransferSrcBarrier
            );
        }

        auto sourceLayers = GetDefaultImageSubresourceLayers(source);

        vk::BufferImageCopy imageToBufferCopyInfo;
        imageToBufferCopyInfo
            .setBufferOffset(distanceOffset)
            .setBufferImageHeight(0)
            .setBufferRowLength(0)
            .setImageSubresource(sourceLayers)
            .setImageOffset(vk::Offset3D{ 0, 0, 0 })
            .setImageExtent(vk::Extent3D{ source.GetWidth(), source.GetHeight(), 1 });

        this->handle.copyImageToBuffer(source.GetNativeHandle(), vk::ImageLayout::eTransferSrcOptimal, distance.GetNativeHandle(), imageToBufferCopyInfo);
    }

    void CommandBuffer::CopyBufferToImage(const Buffer& source, const Image& distance, ImageUsage::Bits distanceUsage)
    {
        this->CopyBufferToImage(source, 0, distance, distanceUsage, source.GetByteSize());
    }

    void CommandBuffer::CopyBufferToImage(const Buffer& source, size_t sourceOffset, const Image& distance, ImageUsage::Bits distanceUsage, size_t byteSize)
    {
        if (distanceUsage != ImageUsage::TRANSFER_DISTINATION)
        {
            auto distanceRange = GetDefaultImageSubresourceRange(distance);

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

            this->handle.pipelineBarrier(
                ImageUsageToPipelineStage(distanceUsage),
                vk::PipelineStageFlagBits::eTransfer,
                { }, // dependency flags
                { }, // memory barriers
                { }, // buffer barriers
                toTransferDstBarrier
            );
        }

        auto distanceLayers = GetDefaultImageSubresourceLayers(distance);

        vk::BufferImageCopy bufferToImageCopyInfo;
        bufferToImageCopyInfo
            .setBufferOffset(sourceOffset)
            .setBufferImageHeight(0)
            .setBufferRowLength(0)
            .setImageSubresource(distanceLayers)
            .setImageOffset(vk::Offset3D{ 0, 0, 0 })
            .setImageExtent(vk::Extent3D{ distance.GetWidth(), distance.GetHeight(), 1 });

        this->handle.copyBufferToImage(source.GetNativeHandle(), distance.GetNativeHandle(), vk::ImageLayout::eTransferDstOptimal, bufferToImageCopyInfo);
    }

    void CommandBuffer::CopyBuffer(const Buffer& source, const Buffer& distance)
    {
        this->CopyBuffer(source, 0, distance, 0, distance.GetByteSize());
    }

    void CommandBuffer::CopyBuffer(const Buffer& source, size_t sourceOffset, const Buffer& distance, size_t distanceOffset, size_t byteSize)
    {
        assert(source.GetByteSize() >= sourceOffset + byteSize);
        assert(distance.GetByteSize() >= distanceOffset + byteSize);

        vk::BufferCopy bufferCopyInfo;
        bufferCopyInfo
            .setDstOffset(distanceOffset)
            .setSize(byteSize)
            .setSrcOffset(sourceOffset);

        this->handle.copyBuffer(source.GetNativeHandle(), distance.GetNativeHandle(), bufferCopyInfo);
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
}