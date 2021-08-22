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
#include <unordered_map>

#include "StringId.h"
#include "RenderGraph.h"
#include "Shader.h"
#include "DescriptorBinding.h"

struct VkBuffer_T;
struct VkImage_T;
using VkBuffer = VkBuffer_T*;
using VkImage = VkImage_T*;

namespace VulkanAbstractionLayer
{
    struct RenderGraphOptions
    {
        using Value = uint32_t;

        enum Bits
        {
            ON_SWAPCHAIN_RESIZE = 1 << 0,
        };
    };

    class RenderGraphBuilder
    {
        struct RenderPassReference
        {
            StringId Name;
            std::unique_ptr<RenderPass> Pass;
        };

        struct ImageTransition
        {
            ImageUsage::Bits InitialUsage;
            ImageUsage::Bits FinalUsage;
        };

        struct BufferTransition
        {
            BufferUsage::Bits InitialUsage;
            BufferUsage::Bits FinalUsage;
        };

        struct ExternalImage
        {
            ImageUsage::Bits InitialUsage;
            Format ImageFormat;
            uint32_t MipLevelCount;
            uint32_t LayerCount;
        };

        struct ExternalBuffer
        {
            BufferUsage::Bits InitialUsage;
        };

        struct ResourceTransitions
        {
            using RenderPassNameId = StringId;

            using ImageTransitionHashMap = std::unordered_map<VkImage, ImageTransition>;
            using BufferTransitionHashMap = std::unordered_map<VkBuffer, BufferTransition>;

            using PerRenderPassImageTransitionHashMap = std::unordered_map<RenderPassNameId, ImageTransitionHashMap>;
            using PerRenderPassBufferTransitionHashMap = std::unordered_map<RenderPassNameId, BufferTransitionHashMap>;

            PerRenderPassImageTransitionHashMap ImageTransitions;
            PerRenderPassBufferTransitionHashMap BufferTransitions;

            std::unordered_map<VkImage, ImageUsage::Value> TotalImageUsages;
            std::unordered_map<VkBuffer, BufferUsage::Value> TotalBufferUsages;

            std::unordered_map<VkBuffer, RenderPassNameId> FirstBufferUsages;
            std::unordered_map<VkImage, RenderPassNameId> FirstImageUsages;
            std::unordered_map<VkBuffer, RenderPassNameId> LastBufferUsages;
            std::unordered_map<VkImage, RenderPassNameId> LastImageUsages;
        };

        using AttachmentHashMap = std::unordered_map<StringId, Image>;
        using DependencyHashMap = std::unordered_map<StringId, DependencyStorage>;
        using PipelineHashMap = std::unordered_map<StringId, Pipeline>;
        using InternalCallback = std::function<void(CommandBuffer&)>;
        using PresentCallback = std::function<void(CommandBuffer&, const Image&, const Image&)>;
        using ExternalImagesHashMap = std::unordered_map<VkImage, ExternalImage>;
        using ExternalBuffersHashMap = std::unordered_map<VkBuffer, ExternalBuffer>;

        ExternalImagesHashMap externalImages;
        ExternalBuffersHashMap externalBuffers;
        std::vector<RenderPassReference> renderPassReferences;
        StringId outputName = { };
        RenderGraphOptions::Value options = { };

        RenderPassNative BuildRenderPass(const RenderPassReference& renderPassReference, const PipelineHashMap& pipelines, const AttachmentHashMap& attachments, const ResourceTransitions& resourceTransitions);
        DependencyHashMap AcquireRenderPassDependencies();
        InternalCallback CreateInternalOnRenderCallback(StringId renderPassName, const DependencyStorage& dependencies, const ResourceTransitions& resourceTransitions, const AttachmentHashMap& attachments);
        InternalCallback CreateOnCreatePipelineCallback(const ResourceTransitions& resourceTransitions, const AttachmentHashMap& attachments);
        PresentCallback CreateOnPresentCallback(StringId outputName, const ResourceTransitions& transitions);
        ResourceTransitions ResolveResourceTransitions(const DependencyHashMap& dependencies);
        AttachmentHashMap AllocateAttachments(const PipelineHashMap& pipelines, const ResourceTransitions& transitions, const DependencyHashMap& dependencies);
        void WriteDescriptorSets(StringId renderPassName, const RenderPassNative& renderPass, PipelineHashMap& pipelines, const AttachmentHashMap& attachments);
        void SetupOutputImage(ResourceTransitions& transitions, StringId outputImage);
        PipelineHashMap CreatePipelines();
        void SetupExternalResources(const Pipeline& pipelineState);
        ImageTransition GetOutputImageFinalTransition(StringId outputName, const ResourceTransitions& resourceTransitions);
    public:
        RenderGraphBuilder& AddRenderPass(StringId name, std::unique_ptr<RenderPass> renderPass);
        RenderGraphBuilder& SetOutputName(StringId name);
        RenderGraphBuilder& SetOptions(RenderGraphOptions::Value options);
        std::unique_ptr<RenderGraph> Build();
    };
}