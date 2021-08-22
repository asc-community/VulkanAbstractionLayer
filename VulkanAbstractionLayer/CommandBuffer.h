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
    struct PassNative;
    
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

    struct ClearDepthStencil
    {
        float Depth = 1.0f;
        uint32_t Stencil = 0;
    };

    enum class BlitFilter
    {
        NEAREST = 0,
        LINEAR,
        CUBIC,
    };

    struct ImageInfo
    {
        ImageReference Resource;
        ImageUsage::Bits Usage = ImageUsage::UNKNOWN;
        uint32_t MipLevel = 0;
        uint32_t Layer = 0;
    };

    struct BufferInfo
    {
        BufferReference Resource;
        uint32_t Offset = 0;
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
        void BeginPass(const PassNative& renderPass);
        void EndPass(const PassNative& renderPass);
        void Draw(uint32_t vertexCount, uint32_t instanceCount) { this->Draw(vertexCount, instanceCount, 0, 0); }
        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
        void BindIndexBufferInt32(const Buffer& indexBuffer);
        void BindIndexBufferInt16(const Buffer& indexBuffer);
        void SetViewport(const Viewport& viewport);
        void SetScissor(const Rect2D& scissor);
        void SetRenderArea(const Image& image);

        void PushConstants(const PassNative& renderPass, const uint8_t* data, size_t size);
        void Dispatch(uint32_t x, uint32_t y, uint32_t z);
        
        void CopyImage(const ImageInfo& source, const ImageInfo& distance);
        void CopyBufferToImage(const BufferInfo& source, const ImageInfo& distance);
        void CopyImageToBuffer(const ImageInfo& source, const BufferInfo& distance);
        void CopyBuffer(const BufferInfo& source, const BufferInfo& distance, size_t byteSize);
        
        void BlitImage(const Image& source, ImageUsage::Bits sourceUsage, const Image& distance, ImageUsage::Bits distanceUsage, BlitFilter filter);
        void GenerateMipLevels(const Image& image, ImageUsage::Bits initialUsage, BlitFilter filter);
    
        void TransferLayout(const Image& image, ImageUsage::Bits oldLayout, ImageUsage::Bits newLayout);
        void TransferLayout(ArrayView<ImageReference> images, ImageUsage::Bits oldLayout, ImageUsage::Bits newLayout);

        template<typename... Buffers>
        void BindVertexBuffers(const Buffers&... vertexBuffers)
        {
            constexpr size_t BufferCount = sizeof...(Buffers);
            std::array buffers = { vertexBuffers.GetNativeHandle()... };
            uint64_t offsets[BufferCount] = { 0 };
            this->GetNativeHandle().bindVertexBuffers(0, BufferCount, buffers.data(), offsets);
        }

        template<typename T>
        void PushConstants(const PassNative& renderPass, ArrayView<const T> constants)
        {
            this->PushConstants(renderPass, (const uint8_t*)constants.data(), constants.size() * sizeof(T));
        }

        template<typename T>
        void PushConstants(const PassNative& renderPass, const T* constants)
        {
            this->PushConstants(renderPass, (const uint8_t*)constants, sizeof(T));
        }
    };
}