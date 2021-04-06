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

#include "VirtualFrame.h"
#include "VulkanContext.h"

namespace VulkanAbstractionLayer
{
    void VirtualFrameProvider::EndFrame(const VulkanContext& context)
    {
        auto& frame = this->GetCurrentFrame();

        vk::ImageSubresourceRange subresourceRange{
                vk::ImageAspectFlagBits::eColor,
                0, // base mip level
                1, // level count
                0, // base layer
                1  // layer count
        };

        auto& presentImage = context.GetSwapchainImage(this->presentImageIndex);

        vk::ImageMemoryBarrier presentImageUndefinedToClear;
        presentImageUndefinedToClear
            .setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setSrcQueueFamilyIndex(context.GetQueueFamilyIndex())
            .setDstQueueFamilyIndex(context.GetQueueFamilyIndex())
            .setImage(presentImage.GetNativeHandle())
            .setSubresourceRange(subresourceRange);
       
        frame.CommandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer,
            { }, // dependency flags
            { }, // memory barriers
            { }, // buffer barriers
            presentImageUndefinedToClear
        );

        vk::ClearColorValue color = std::array{ 1.0f, 0.7f, 0.0f, 1.0f };
        frame.CommandBuffer.clearColorImage(presentImage.GetNativeHandle(), vk::ImageLayout::eTransferDstOptimal, color, subresourceRange);

        vk::ImageMemoryBarrier presentImageClearToPresent;
        presentImageClearToPresent
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eMemoryRead)
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
            .setSrcQueueFamilyIndex(context.GetQueueFamilyIndex())
            .setDstQueueFamilyIndex(context.GetQueueFamilyIndex())
            .setImage(presentImage.GetNativeHandle())
            .setSubresourceRange(subresourceRange);

        frame.CommandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            { }, // dependency flags
            { }, // memory barriers
            { }, // buffer barriers
            presentImageClearToPresent
        );

        frame.CommandBuffer.end();

        std::array waitDstStageMask = { (vk::PipelineStageFlags)vk::PipelineStageFlagBits::eTransfer };

        vk::SubmitInfo submitInfo;
        submitInfo
            .setWaitSemaphores(context.GetImageAvailableSemaphore())
            .setWaitDstStageMask(waitDstStageMask)
            .setSignalSemaphores(context.GetRenderingFinishedSemaphore())
            .setCommandBuffers(frame.CommandBuffer);

        context.GetGraphicsQueue().submit(std::array{ submitInfo }, frame.CommandQueueFence);

        vk::PresentInfoKHR presentInfo;
        presentInfo
            .setWaitSemaphores(context.GetRenderingFinishedSemaphore())
            .setSwapchains(context.GetSwapchain())
            .setImageIndices(this->presentImageIndex);

        auto presetSucceeded = context.GetPresentQueue().presentKHR(presentInfo);
        assert(presetSucceeded == vk::Result::eSuccess);

        this->currentFrame = (this->currentFrame + 1) % this->virtualFrames.size();
    }

    void VirtualFrameProvider::RecreateFramebuffer(const VulkanContext& context)
    {
        VirtualFrame& frame = this->GetCurrentFrame();
    }

    void VirtualFrameProvider::Init(size_t frameCount, const VulkanContext& context)
    {
        this->virtualFrames.resize(frameCount);

        vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
        commandBufferAllocateInfo
            .setCommandPool(context.GetCommandPool())
            .setCommandBufferCount((uint32_t)frameCount)
            .setLevel(vk::CommandBufferLevel::ePrimary);
        
        auto commandBuffers = context.GetDevice().allocateCommandBuffers(commandBufferAllocateInfo);

        for (size_t i = 0; i < frameCount; i++)
        {
            VirtualFrame& virtualFrame = this->virtualFrames[i];
            virtualFrame.CommandBuffer = std::move(commandBuffers[i]);
            virtualFrame.CommandQueueFence = context.GetDevice().createFence(vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled });
            // virtualFrame.framebuffer is recreated on StartFrame
        }
    }

    void VirtualFrameProvider::Destroy(const VulkanContext& context)
    {
        for (const auto& virtualFrame : this->virtualFrames)
        {
            if((bool)virtualFrame.CommandQueueFence) context.GetDevice().destroyFence(virtualFrame.CommandQueueFence);
        }
        this->virtualFrames.clear();
    }

    void VirtualFrameProvider::StartFrame(const VulkanContext& context)
    {
        auto acquireNextImage = context.GetDevice().acquireNextImageKHR(context.GetSwapchain(), UINT64_MAX, context.GetImageAvailableSemaphore());
        assert(acquireNextImage.result == vk::Result::eSuccess || acquireNextImage.result == vk::Result::eSuboptimalKHR);
        this->presentImageIndex = acquireNextImage.value;
        
        auto& frame = this->GetCurrentFrame();

        vk::Result waitFenceResult = context.GetDevice().waitForFences(frame.CommandQueueFence, false, UINT64_MAX);
        assert(waitFenceResult == vk::Result::eSuccess);
        context.GetDevice().resetFences(frame.CommandQueueFence);

        vk::CommandBufferBeginInfo commandBufferBeginInfo;
        commandBufferBeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        frame.CommandBuffer.begin(commandBufferBeginInfo);
    }

    VirtualFrame& VirtualFrameProvider::GetCurrentFrame()
    {
        return this->virtualFrames[this->currentFrame];
    }

    VirtualFrame& VirtualFrameProvider::GetNextFrame()
    {
        return this->virtualFrames[(this->currentFrame + 1) % this->virtualFrames.size()];
    }
}