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

#include "ArrayUtils.h"
#include "Image.h"
#include "Buffer.h"

namespace VulkanAbstractionLayer
{
    struct RenderPassNative;
    
    struct Rect2D
    {
        int32_t OffsetWidth = 0;
        int32_t OffsetHeight = 0;
        uint32_t Width = 0;
        uint32_t Height = 0;
    };

    struct Viewport
    {
        float OffsetWidth = 0.0f;
        float OffsetHeight = 0.0f;
        float Width = 0.0f;
        float Height = 0.0f;
        float MinDepth = 0.0f;
        float MaxDepth = 0.0f;
    };

    struct ClearColor
    {
        float R = 0.0f;
        float G = 0.0f;
        float B = 0.0f;
        float A = 1.0f;
    };

    struct ClearDepthSpencil
    {
        float Depth = 1.0f;
        uint32_t Spencil = 0;
    };

    enum class BlitFilter
    {
        NEAREST = 0,
        LINEAR,
        CUBIC,
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
        void BeginRenderPass(const RenderPassNative& renderPass);
        void EndRenderPass();
        void Draw(uint32_t vertexCount, uint32_t instanceCount) { this->Draw(vertexCount, instanceCount, 0, 0); }
        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
        void BindIndexBufferInt32(const Buffer& indexBuffer);
        void BindIndexBufferInt16(const Buffer& indexBuffer);
        void SetViewport(const Viewport& viewport);
        void SetScissor(const Rect2D& scissor);
        void SetRenderArea(const Image& image);
        
        void CopyImage(const Image& source, ImageUsage::Bits sourceUsage, const Image& distance, ImageUsage::Bits distanceUsage);
        
        void CopyBufferToImage(const Buffer& source, const Image& distance, ImageUsage::Bits distanceUsage);
        void CopyBufferToImage(const Buffer& source, size_t sourceOffset, const Image& distance, ImageUsage::Bits distanceUsage, size_t byteSize);
        
        void CopyImageToBuffer(const Image& source, ImageUsage::Bits sourceUsage, const Buffer& distance);
        void CopyImageToBuffer(const Image& source, ImageUsage::Bits sourceUsage, const Buffer& distance, size_t distanceOffset, size_t byteSize);
        
        void CopyBuffer(const Buffer& source, const Buffer& distance);
        void CopyBuffer(const Buffer& source, size_t sourceOffset, const Buffer& distance,size_t distanceOffset, size_t byteSize);
        
        void BlitImage(const Image& source, ImageUsage::Bits sourceUsage, const Image& distance, ImageUsage::Bits distanceUsage, BlitFilter filter);
    
        template<typename... Buffers>
        void BindVertexBuffers(const Buffers&... vertexBuffers)
        {
            constexpr size_t BufferCount = sizeof...(Buffers);
            const vk::Buffer buffers[BufferCount] = { vertexBuffers.GetNativeHandle()... };
            vk::DeviceSize offsets[BufferCount] = { 0 };
            this->GetNativeHandle().bindVertexBuffers(0, BufferCount, buffers, offsets);
        }
    };
}