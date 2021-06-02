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

#include <vulkan/vulkan.hpp>

namespace VulkanAbstractionLayer
{
    class RenderPass;
    class Image;
    class Buffer;
    
    struct Rect2D
    {
        int32_t OffsetWidth = { };
        int32_t OffsetHeight = { };
        uint32_t Width = { };
        uint32_t Height = { };
    };

    struct Viewport
    {
        float OffsetWidth = { };
        float OffsetHeight = { };
        float Width = { };
        float Height = { };
        float MinDepth = { };
        float MaxDepth = { };
    };

    class CommandBuffer
    {
        vk::CommandBuffer handle;
    public:
        CommandBuffer(vk::CommandBuffer commandBuffer)
            : handle(std::move(commandBuffer)) { }

        const vk::CommandBuffer& GetNativeHandle() const { return this->handle; }
        void Begin();
        void End();
        void BeginRenderPass(const RenderPass& renderPass);
        void EndRenderPass();
        void Draw(uint32_t vertexCount, uint32_t instanceCount) { this->Draw(vertexCount, instanceCount, 0, 0); }
        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
        void BindVertexBuffer(const Buffer& vertexBuffer);
        void BindIndexBufferInt32(const Buffer& indexBuffer);
        void BindIndexBufferInt16(const Buffer& indexBuffer);
        void SetViewport(const Viewport& viewport);
        void SetScissor(const Rect2D& scissor);
        void SetRenderArea(const Image& image);
        void CopyImage(const Image& source, vk::ImageLayout sourceLayout, vk::AccessFlags sourceFlags, const Image& distance, vk::ImageLayout distanceLayout, vk::AccessFlags distanceFlags, vk::PipelineStageFlags pipelineFlags);
        void CopyBufferToImage(const Buffer& source, vk::AccessFlags sourceFlags, const Image& distance, vk::ImageLayout distanceLayout, vk::AccessFlags distanceFlags, vk::PipelineStageFlags pipelineFlags);
        void CopyImageToBuffer(const Image& source, vk::AccessFlags sourceFlags, vk::ImageLayout sourceLayout, const Buffer& distance, vk::AccessFlags distanceFlags, vk::PipelineStageFlags pipelineFlags);
        void CopyBuffer(const Buffer& source, vk::AccessFlags sourceFlags, const Buffer& distance, vk::AccessFlags distanceFlags, vk::PipelineStageFlags pipelineFlags);
        void CopyBuffer(const Buffer& source, vk::AccessFlags sourceFlags, size_t sourceOffset, const Buffer& distance, vk::AccessFlags distanceFlags, size_t distanceOffset, vk::PipelineStageFlags pipelineFlags, size_t byteSize);
        void BlitImage(const Image& source, vk::ImageLayout sourceLayout, vk::AccessFlags sourceFlags, const Image& distance, vk::ImageLayout distanceLayout, vk::AccessFlags distanceFlags, vk::PipelineStageFlags pipelineFlags, vk::Filter filter);
    };
}