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

namespace VulkanAbstractionLayer
{
    void VirtualFrameProvider::EndFrame()
    {
        this->currentFrame = (this->currentFrame + 1) % this->virtualFrames.size();
    }

    void VirtualFrameProvider::Init(size_t frameCount, const vk::Device& device, const vk::CommandPool& pool)
    {
        this->virtualFrames.resize(frameCount);

        vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
        commandBufferAllocateInfo
            .setCommandPool(pool)
            .setCommandBufferCount(frameCount)
            .setLevel(vk::CommandBufferLevel::ePrimary);
        
        auto commandBuffers = device.allocateCommandBuffers(commandBufferAllocateInfo);

        for (size_t i = 0; i < frameCount; i++)
        {
            VirtualFrame& virtualFrame = this->virtualFrames[i];
            virtualFrame.commandBuffer = std::move(commandBuffers[i]);
            virtualFrame.commandQueueFence = device.createFence(vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled });
            // virtualFrame.framebuffer is recreated on StartFrame
        }
    }

    void VirtualFrameProvider::Destroy(const vk::Device& device)
    {
        for (const auto& virtualFrame : this->virtualFrames)
        {
            if((bool)virtualFrame.commandQueueFence) device.destroyFence(virtualFrame.commandQueueFence);
            if((bool)virtualFrame.framebuffer) device.destroyFramebuffer(virtualFrame.framebuffer);
        }
        this->virtualFrames.clear();
    }

    void VirtualFrameProvider::StartFrame()
    {
       
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