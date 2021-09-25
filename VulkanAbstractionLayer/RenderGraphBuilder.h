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
#include <string>
#include <array>
#include <optional>
#include <unordered_map>

#include "RenderGraph.h"
#include "Shader.h"
#include "DescriptorBinding.h"

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
            std::string Name;
            std::unique_ptr<RenderPass> Pass;
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

        using RenderPassName = std::string;

        template<typename ResourceType, typename TransitionType>
        struct ResourceTypeTransitions
        {
            using TransitionHashMap = std::unordered_map<ResourceType, TransitionType>;
            using PerRenderPassTransitionHashMap = std::unordered_map<RenderPassName, TransitionHashMap>;
            using UsageMask = uint32_t;

            PerRenderPassTransitionHashMap Transitions;
            std::unordered_map<ResourceType, UsageMask> TotalUsages;
            std::unordered_map<ResourceType, RenderPassName> FirstUsages;
            std::unordered_map<ResourceType, RenderPassName> LastUsages;
        };

        struct ResourceTransitions
        {
            ResourceTypeTransitions<VkBuffer, BufferTransition> Buffers;
            ResourceTypeTransitions<VkImage, ImageTransition> Images;
            ResourceTypeTransitions<std::string, ImageTransition> Attachments;
        };

        using AttachmentHashMap = std::unordered_map<std::string, Image>;
        using DependencyHashMap = std::unordered_map<RenderPassName, DependencyStorage>;
        using PipelineHashMap = std::unordered_map<RenderPassName, Pipeline>;
        using InternalCallback = std::function<void(CommandBuffer&)>;
        using PresentCallback = std::function<void(CommandBuffer&, const Image&, const Image&)>;
        using ExternalImagesHashMap = std::unordered_map<VkImage, ExternalImage>;
        using ExternalBuffersHashMap = std::unordered_map<VkBuffer, ExternalBuffer>;

        ResourceTransitions resourceTransitions;
        ExternalImagesHashMap externalImages;
        ExternalBuffersHashMap externalBuffers;
        std::vector<RenderPassReference> renderPassReferences;
        std::string outputName;
        RenderGraphOptions::Value options = { };
        
        PassNative BuildRenderPass(const RenderPassReference& renderPassReference, const PipelineHashMap& pipelines, const AttachmentHashMap& attachments, const ResourceTransitions& resourceTransitions);
        DependencyHashMap AcquireRenderPassDependencies(const PipelineHashMap& pipelines);
        InternalCallback CreateInternalOnRenderCallback(const std::string& renderPassName, const Pipeline& pipeline, const ResourceTransitions& resourceTransitions, const AttachmentHashMap& attachments);
        InternalCallback CreateOnCreatePipelineCallback(const ResourceTransitions& resourceTransitions, const AttachmentHashMap& attachments);
        PresentCallback CreateOnPresentCallback(const std::string& outputName, const ResourceTransitions& transitions);
        ResourceTransitions ResolveResourceTransitions(const DependencyHashMap& dependencies, const PipelineHashMap& pipelines);
        AttachmentHashMap AllocateAttachments(const PipelineHashMap& pipelines, const ResourceTransitions& transitions, const DependencyHashMap& dependencies);
        void ResolveDescriptorSets(const std::string& renderPassName, const PassNative& renderPass, PipelineHashMap& pipelines, const AttachmentHashMap& attachments);
        void SetupOutputImage(ResourceTransitions& transitions, const std::string& outputImage);
        PipelineHashMap CreatePipelines();
        void SetupExternalResources(const Pipeline& pipelineState);
        ImageTransition GetOutputImageFinalTransition(const std::string& outputName, const ResourceTransitions& resourceTransitions);
    public:
        RenderGraphBuilder& AddRenderPass(const std::string& name, std::unique_ptr<RenderPass> renderPass);
        RenderGraphBuilder& SetOutputName(const std::string& name);
        RenderGraphBuilder& SetOptions(RenderGraphOptions::Value options);
        std::unique_ptr<RenderGraph> Build();
    };
}