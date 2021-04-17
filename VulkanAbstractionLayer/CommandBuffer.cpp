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

namespace VulkanAbstractionLayer
{
    void CommandBuffer::BeginRenderPass(const RenderPass& renderPass)
    {
        vk::RenderPassBeginInfo renderPassBeginInfo;
        renderPassBeginInfo
            .setRenderPass(renderPass.GetNativeHandle())
            .setRenderArea(renderPass.GetRenderArea())
            .setFramebuffer(renderPass.GetFramebuffer())
            .setClearValues(renderPass.GetClearValues());

        this->handle.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    }

    void CommandBuffer::EndRenderPass()
    {
        this->handle.endRenderPass();
    }

    void CommandBuffer::CopyImage(const Image& source, vk::ImageLayout sourceLayout, const Image& distance, vk::ImageLayout distanceLayout)
    {
        vk::ImageSubresourceRange subresourceRange{
                vk::ImageAspectFlagBits::eColor,
                0, // base mip level
                1, // level count
                0, // base layer
                1  // layer count
        };

        vk::ImageMemoryBarrier toTransferSrcBarrier;
        toTransferSrcBarrier
            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
            .setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(source.GetNativeHandle())
            .setSubresourceRange(subresourceRange);

        vk::ImageMemoryBarrier toTransferDstBarrier;
        toTransferDstBarrier
            .setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(distance.GetNativeHandle())
            .setSubresourceRange(subresourceRange);

        // TODO: flexible pipeline stage flags
        std::array preCopyBarriers = { toTransferSrcBarrier, toTransferDstBarrier };
        this->handle.pipelineBarrier(
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
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
            .setSrcSubresource(vk::ImageSubresourceLayers{
                vk::ImageAspectFlagBits::eColor,
                0, // base mip level
                0, // base layer
                1  // layer count
            })
            .setDstSubresource(vk::ImageSubresourceLayers{
                vk::ImageAspectFlagBits::eColor,
                0, // base mip level
                0, // base layer
                1  // layer count
            })
            .setExtent(vk::Extent3D{ (uint32_t)distance.GetWidth(), (uint32_t)distance.GetHeight(), 1 });

        this->handle.copyImage(source.GetNativeHandle(), vk::ImageLayout::eTransferSrcOptimal, distance.GetNativeHandle(), vk::ImageLayout::eTransferDstOptimal, imageCopyInfo);
    }
}