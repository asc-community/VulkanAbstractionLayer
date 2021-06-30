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

#include <vector>
#include <array>
#include <optional>

#include "StringId.h"
#include "RenderGraph.h"
#include "GraphicShader.h"
#include "DescriptorBinding.h"

namespace VulkanAbstractionLayer
{
    struct GraphicPipeline
    {
        GraphicShader Shader;
        std::vector<VertexBinding> VertexBindings;
        DescriptorBinding DescriptorBindings;
    };

    class RenderPassBuilder
    {
        StringId name = { };
        std::unique_ptr<RenderPass> renderPass;
        std::optional<GraphicPipeline> graphicPipeline;
    public:
        RenderPassBuilder(StringId name);
        RenderPassBuilder(RenderPassBuilder&&) = default;
        RenderPassBuilder& operator=(RenderPassBuilder&&) = default;

        RenderPassBuilder& AddRenderPass(std::unique_ptr<RenderPass> renderPass);
        RenderPassBuilder& SetPipeline(GraphicPipeline pipeline);

        friend class RenderGraphBuilder;
    };

    class RenderGraphBuilder
    {
        struct AttachmentCreateOptions
        {
            Format LayoutFormat;
            uint32_t Width;
            uint32_t Height;
        };

        struct ImageTransition
        {
            ImageUsage::Bits InitialUsage;
            ImageUsage::Bits FinalUsage;
        };

        struct BufferTransition
        {
            BufferUsage::Value InitialUsage;
            BufferUsage::Value FinalUsage;
        };

        struct ResourceTransitions
        {
            using ImageTransitionHashMap = std::unordered_map<StringId, ImageTransition>;
            using BufferTransitionHashMap = std::unordered_map<StringId, BufferTransition>;

            using PerRenderPassImageTransitionHashMap = std::unordered_map<StringId, ImageTransitionHashMap>;
            using PerRenderPassBufferTransitionHashMap = std::unordered_map<StringId, BufferTransitionHashMap>;

            PerRenderPassImageTransitionHashMap ImageTransitions;
            PerRenderPassBufferTransitionHashMap BufferTransitions;

            std::unordered_map<StringId, ImageUsage::Value> TotalImageUsages;
            std::unordered_map<StringId, BufferUsage::Value> TotalBufferUsages;
        };

        using AttachmentHashMap = std::unordered_map<StringId, Image>;
        using AttachmentCreateOptionsHashMap = std::unordered_map<StringId, AttachmentCreateOptions>;
        using DependencyHashMap = std::unordered_map<StringId, DependencyStorage>;
        using ExternalImagesInitialUsage = std::unordered_map<StringId, ImageUsage::Bits>;
        using ExternalBuffersInitialUsage = std::unordered_map<StringId, BufferUsage::Bits>;

        AttachmentCreateOptionsHashMap attachmentsCreateOptions;
        ExternalImagesInitialUsage externalImagesInitialUsage;
        ExternalBuffersInitialUsage externalBufferInitialUsage;
        std::vector<RenderPassBuilder> renderPasses;
        StringId outputName = { };

        RenderPassNative BuildRenderPass(const RenderPassBuilder& renderPassBuilder, const AttachmentHashMap& attachments, const ResourceTransitions& resourceTransitions);
        DependencyHashMap AcquireRenderPassDependencies();
        ResourceTransitions ResolveResourceTransitions(const DependencyHashMap& dependencies);
        AttachmentHashMap AllocateAttachments(const ResourceTransitions& transitions, const DependencyHashMap& dependencies);
        void SetupOutputImage(ResourceTransitions& transitions, StringId outputImage);
        void PreWarmDescriptorSets();
    public:
        RenderGraphBuilder& AddRenderPass(RenderPassBuilder&& renderPass);
        RenderGraphBuilder& AddRenderPass(RenderPassBuilder& renderPass);
        RenderGraphBuilder& SetOutputName(StringId name);
        RenderGraphBuilder& AddAttachment(StringId name, Format format);
        RenderGraphBuilder& AddAttachment(StringId name, Format format, uint32_t width, uint32_t height);
        RenderGraphBuilder& AddExternalImage(StringId name, ImageUsage::Bits initialUsage);
        RenderGraphBuilder& AddExternalBuffer(StringId name, BufferUsage::Bits initialUsage);
        RenderGraph Build();
    };
}