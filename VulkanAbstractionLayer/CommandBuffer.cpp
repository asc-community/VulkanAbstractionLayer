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

    void CommandBuffer::BeginRenderPass(const RenderPass& renderPass)
    {
        vk::RenderPassBeginInfo renderPassBeginInfo;
        renderPassBeginInfo
            .setRenderPass(renderPass.GetNativeHandle())
            .setRenderArea(renderPass.GetRenderArea())
            .setFramebuffer(renderPass.GetFramebuffer())
            .setClearValues(renderPass.GetClearValues());

        this->handle.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

        vk::Pipeline graphicPipeline = renderPass.GetPipeline();
        if ((bool)graphicPipeline) this->handle.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicPipeline);
    }

    void CommandBuffer::EndRenderPass()
    {
        this->handle.endRenderPass();
    }

    void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        this->handle.draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void CommandBuffer::BindVertexBuffer(const Buffer& vertexBuffer)
    {
        this->handle.bindVertexBuffers(0, vertexBuffer.GetNativeHandle(), { 0 });
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
            viewport.OffsetHeight, 
            viewport.Width, 
            viewport.Height, 
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

    void CommandBuffer::CopyImage(const Image& source, vk::ImageLayout sourceLayout, vk::AccessFlags sourceFlags, const Image& distance, vk::ImageLayout distanceLayout, vk::AccessFlags distanceFlags, vk::PipelineStageFlags pipelineFlags)
    {
        vk::ImageSubresourceRange subresourceRange{
            vk::ImageAspectFlagBits::eColor,
            0, // base mip level
            1, // level count
            0, // base layer
            1  // layer count
        };

        vk::ImageSubresourceLayers subresourceLayer{
            subresourceRange.aspectMask,
            subresourceRange.baseMipLevel,
            subresourceRange.baseArrayLayer,
            subresourceRange.layerCount
        };

        vk::ImageMemoryBarrier toTransferSrcBarrier;
        toTransferSrcBarrier
            .setSrcAccessMask(sourceFlags)
            .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
            .setOldLayout(sourceLayout)
            .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(source.GetNativeHandle())
            .setSubresourceRange(subresourceRange);

        vk::ImageMemoryBarrier toTransferDstBarrier;
        toTransferDstBarrier
            .setSrcAccessMask(distanceFlags)
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setOldLayout(distanceLayout)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(distance.GetNativeHandle())
            .setSubresourceRange(subresourceRange);

        std::array preCopyBarriers = { toTransferSrcBarrier, toTransferDstBarrier };
        this->handle.pipelineBarrier(
            pipelineFlags,
            vk::PipelineStageFlagBits::eTransfer,
            { }, // dependency flags
            { }, // memory barriers
            { }, // buffer barriers
            preCopyBarriers
        );

        vk::ImageCopy imageCopyInfo;
        imageCopyInfo
            .setSrcOffset(0)
            .setDstOffset(0)
            .setSrcSubresource(subresourceLayer)
            .setDstSubresource(subresourceLayer)
            .setExtent(vk::Extent3D{ (uint32_t)distance.GetWidth(), (uint32_t)distance.GetHeight(), 1 });

        this->handle.copyImage(source.GetNativeHandle(), vk::ImageLayout::eTransferSrcOptimal, distance.GetNativeHandle(), vk::ImageLayout::eTransferDstOptimal, imageCopyInfo);
    }

    void CommandBuffer::CopyBufferToImage(const Buffer& source, vk::AccessFlags sourceFlags, const Image& distance, vk::ImageLayout distanceLayout, vk::AccessFlags distanceFlags, vk::PipelineStageFlags pipelineFlags)
    {
        vk::ImageSubresourceRange subresourceRange{
            vk::ImageAspectFlagBits::eColor,
            0, // base mip level
            1, // level count
            0, // base layer
            1  // layer count
        };

        vk::ImageSubresourceLayers subresourceLayer{
            subresourceRange.aspectMask,
            subresourceRange.baseMipLevel,
            subresourceRange.baseArrayLayer,
            subresourceRange.layerCount
        };

        vk::BufferMemoryBarrier toTransferSrcBarrier;
        toTransferSrcBarrier
            .setSrcAccessMask(sourceFlags)
            .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setBuffer(source.GetNativeHandle())
            .setSize(source.GetByteSize())
            .setOffset(0);

        vk::ImageMemoryBarrier toTransferDstBarrier;
        toTransferDstBarrier
            .setSrcAccessMask(distanceFlags)
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setOldLayout(distanceLayout)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(distance.GetNativeHandle())
            .setSubresourceRange(subresourceRange);

        this->handle.pipelineBarrier(
            pipelineFlags,
            vk::PipelineStageFlagBits::eTransfer,
            { }, // dependency flags
            { }, // memory barriers
            toTransferSrcBarrier,
            toTransferDstBarrier
        );

        vk::BufferImageCopy bufferToImageCopyInfo;
        bufferToImageCopyInfo
            .setBufferOffset(0)
            .setBufferImageHeight(0)
            .setBufferRowLength(0)
            .setImageSubresource(subresourceLayer)
            .setImageOffset(vk::Offset3D{ 0, 0, 0 })
            .setImageExtent(vk::Extent3D{ distance.GetWidth(), distance.GetHeight(), 1 });

        this->handle.copyBufferToImage(source.GetNativeHandle(), distance.GetNativeHandle(), distanceLayout, bufferToImageCopyInfo);
    }

    void CommandBuffer::CopyImageToBuffer(const Image& source, vk::AccessFlags sourceFlags, vk::ImageLayout sourceLayout, const Buffer& distance, vk::AccessFlags distanceFlags, vk::PipelineStageFlags pipelineFlags)
    {
        vk::ImageSubresourceRange subresourceRange{
            vk::ImageAspectFlagBits::eColor,
            0, // base mip level
            1, // level count
            0, // base layer
            1  // layer count
        };

        vk::ImageSubresourceLayers subresourceLayer{
            subresourceRange.aspectMask,
            subresourceRange.baseMipLevel,
            subresourceRange.baseArrayLayer,
            subresourceRange.layerCount
        };

        vk::ImageMemoryBarrier toTransferSrcBarrier;
        toTransferSrcBarrier
            .setSrcAccessMask(distanceFlags)
            .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
            .setOldLayout(sourceLayout)
            .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(source.GetNativeHandle())
            .setSubresourceRange(subresourceRange);

        vk::BufferMemoryBarrier toTransferDstBarrier;
        toTransferDstBarrier
            .setSrcAccessMask(sourceFlags)
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setBuffer(distance.GetNativeHandle())
            .setSize(distance.GetByteSize())
            .setOffset(0);

        this->handle.pipelineBarrier(
            pipelineFlags,
            vk::PipelineStageFlagBits::eTransfer,
            { }, // dependency flags
            { }, // memory barriers
            toTransferDstBarrier,
            toTransferSrcBarrier
        );

        vk::BufferImageCopy imageToBufferCopyInfo;
        imageToBufferCopyInfo
            .setBufferOffset(0)
            .setBufferImageHeight(0)
            .setBufferRowLength(0)
            .setImageSubresource(subresourceLayer)
            .setImageOffset(vk::Offset3D{ 0, 0, 0 })
            .setImageExtent(vk::Extent3D{ source.GetWidth(), source.GetHeight(), 1 });

        this->handle.copyImageToBuffer(source.GetNativeHandle(), sourceLayout, distance.GetNativeHandle(), imageToBufferCopyInfo);
    }

    void CommandBuffer::CopyBuffer(const Buffer& source, vk::AccessFlags sourceFlags, const Buffer& distance, vk::AccessFlags distanceFlags, vk::PipelineStageFlags pipelineFlags)
    {
        this->CopyBuffer(source, sourceFlags, distance, distanceFlags, pipelineFlags, distance.GetByteSize(), 0);
    }

    void CommandBuffer::CopyBuffer(const Buffer& source, vk::AccessFlags sourceFlags, const Buffer& distance, vk::AccessFlags distanceFlags, vk::PipelineStageFlags pipelineFlags, size_t size, size_t offset)
    {
        assert(source.GetByteSize() >= size);
        assert(distance.GetByteSize() >= size + offset);

        vk::BufferMemoryBarrier toTransferSrcBarrier;
        toTransferSrcBarrier
            .setSrcAccessMask(distanceFlags)
            .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setBuffer(source.GetNativeHandle())
            .setSize(size)
            .setOffset(0);

        vk::BufferMemoryBarrier toTransferDstBarrier;
        toTransferDstBarrier
            .setSrcAccessMask(sourceFlags)
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setBuffer(source.GetNativeHandle())
            .setSize(size)
            .setOffset(0);

        std::array preCopyBarriers = { toTransferSrcBarrier, toTransferDstBarrier };
        this->handle.pipelineBarrier(
            pipelineFlags,
            vk::PipelineStageFlagBits::eTransfer,
            { }, // dependency flags
            { }, // memory barriers
            preCopyBarriers,
            { } // image barriers
        );

        vk::BufferCopy bufferCopyInfo;
        bufferCopyInfo
            .setDstOffset(offset)
            .setSize(size)
            .setSrcOffset(0);

        this->handle.copyBuffer(source.GetNativeHandle(), distance.GetNativeHandle(), bufferCopyInfo);
    }

    void CommandBuffer::BlitImage(const Image& source, vk::ImageLayout sourceLayout, vk::AccessFlags sourceFlags, const Image& distance, vk::ImageLayout distanceLayout, vk::AccessFlags distanceFlags, vk::PipelineStageFlags pipelineFlags, vk::Filter filter)
    {
        vk::ImageSubresourceRange subresourceRange{
             vk::ImageAspectFlagBits::eColor,
             0, // base mip level
             1, // level count
             0, // base layer
             1  // layer count
        };

        vk::ImageSubresourceLayers subresourceLayer{
            subresourceRange.aspectMask,
            subresourceRange.baseMipLevel,
            subresourceRange.baseArrayLayer,
            subresourceRange.layerCount
        };

        vk::ImageMemoryBarrier toTransferSrcBarrier;
        toTransferSrcBarrier
            .setSrcAccessMask(sourceFlags)
            .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
            .setOldLayout(sourceLayout)
            .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(source.GetNativeHandle())
            .setSubresourceRange(subresourceRange);

        vk::ImageMemoryBarrier toTransferDstBarrier;
        toTransferDstBarrier
            .setSrcAccessMask(distanceFlags)
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setOldLayout(distanceLayout)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(distance.GetNativeHandle())
            .setSubresourceRange(subresourceRange);

        std::array preCopyBarriers = { toTransferSrcBarrier, toTransferDstBarrier };
        this->handle.pipelineBarrier(
            pipelineFlags,
            vk::PipelineStageFlagBits::eTransfer,
            { }, // dependency flags
            { }, // memory barriers
            { }, // buffer barriers
            preCopyBarriers
        );

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
            .setSrcSubresource(subresourceLayer)
            .setDstSubresource(subresourceLayer);

        this->handle.blitImage(
            source.GetNativeHandle(), 
            vk::ImageLayout::eTransferSrcOptimal, 
            distance.GetNativeHandle(), 
            vk::ImageLayout::eTransferDstOptimal, 
            imageBlitInfo,
            filter
        );
    }
}