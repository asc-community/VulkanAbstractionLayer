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

#include "Shader.h"
#include "DescriptorBinding.h"
#include "StringId.h"
#include "CommandBuffer.h"

namespace VulkanAbstractionLayer
{
    enum class AttachmentState
    {
        DISCARD_COLOR = 0,
        DISCARD_DEPTH_SPENCIL,
        LOAD_COLOR,
        LOAD_DEPTH_SPENCIL,
        CLEAR_COLOR,
        CLEAR_DEPTH_SPENCIL,
    };

    class Pipeline
    {
    public:
        struct BufferDeclaration
        {
            VkBuffer BufferNativeHandle;
            BufferUsage::Bits InitialUsage;
        };

        struct ImageDeclaration
        {
            VkImage ImageNativeHandle;
            ImageUsage::Bits InitialUsage;
            Format ImageFormat;
            uint32_t MipLevelCount;
            uint32_t LayerCount;
        };

        struct AttachmentDeclaration
        {
            StringId Name;
            Format ImageFormat;
            uint32_t Width;
            uint32_t Height;
            ImageOptions::Value Options;
        };

        struct OutputAttachment
        {
            constexpr static uint32_t ALL_LAYERS = uint32_t(-1);

            StringId Name;
            ClearColor ColorClear;
            ClearDepthStencil DepthSpencilClear;
            AttachmentState OnLoad;
            uint32_t Layer;
        };

    private:
        std::vector<BufferDeclaration> bufferDeclarations;
        std::vector<ImageDeclaration> imageDeclarations;
        std::vector<AttachmentDeclaration> attachmentDeclarations;
        std::vector<OutputAttachment> outputAttachments;

    public:
        std::shared_ptr<Shader> Shader;
        std::vector<VertexBinding> VertexBindings;
        DescriptorBinding DescriptorBindings;

        void AddOutputAttachment(StringId name, ClearColor clear);
        void AddOutputAttachment(StringId name, ClearDepthStencil clear);
        void AddOutputAttachment(StringId name, AttachmentState onLoad);
        void AddOutputAttachment(StringId name, ClearColor clear, uint32_t layer);
        void AddOutputAttachment(StringId name, ClearDepthStencil clear, uint32_t layer);
        void AddOutputAttachment(StringId name, AttachmentState onLoad, uint32_t layer);

        void DeclareBuffer(const Buffer& buffer);
        void DeclareImage(const Image& image, ImageUsage::Bits oldUsage);

        void DeclareBuffers(ArrayView<BufferReference> buffers);
        void DeclareBuffers(ArrayView<Buffer> buffers);
        void DeclareImages(ArrayView<ImageReference> images, ImageUsage::Bits oldUsage);
        void DeclareImages(ArrayView<Image> images, ImageUsage::Bits oldUsage);

        void DeclareAttachment(StringId name, Format format);
        void DeclareAttachment(StringId name, Format format, uint32_t width, uint32_t height);
        void DeclareAttachment(StringId name, Format format, uint32_t width, uint32_t height, ImageOptions::Value options);

        const auto& GetBufferDeclarations() const { return this->bufferDeclarations; }
        const auto& GetImageDeclarations() const { return this->imageDeclarations; }
        const auto& GetAttachmentDeclarations() const { return this->attachmentDeclarations; }
        const auto& GetOutputAttachments() const { return this->outputAttachments; }
    };
}