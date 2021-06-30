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

#include "RenderGraphBuilder.h"
#include "CommandBuffer.h"
#include "VulkanContext.h"

namespace VulkanAbstractionLayer
{
    RenderPassBuilder::RenderPassBuilder(StringId name)
    {
        this->name = name;
    }

    RenderPassBuilder& RenderPassBuilder::AddRenderPass(std::unique_ptr<RenderPass> renderPass)
    {
        this->renderPass = std::move(renderPass);
        return *this;
    }

    RenderPassBuilder& RenderPassBuilder::SetPipeline(GraphicPipeline pipeline)
    {
        this->graphicPipeline = std::move(pipeline);
        return *this;
    }

    std::array InputRateTable = {
        vk::VertexInputRate::eVertex,
        vk::VertexInputRate::eInstance,
    };

    vk::AttachmentLoadOp AttachmentStateToLoadOp(AttachmentState state)
    {
        switch (state)
        {
        case AttachmentState::DISCARD_COLOR:
            return vk::AttachmentLoadOp::eDontCare;
        case AttachmentState::DISCARD_DEPTH_SPENCIL:
            return vk::AttachmentLoadOp::eDontCare;
        case AttachmentState::LOAD_COLOR:
            return vk::AttachmentLoadOp::eLoad;
        case AttachmentState::LOAD_DEPTH_SPENCIL:
            return vk::AttachmentLoadOp::eLoad;
        case AttachmentState::CLEAR_COLOR:
            return vk::AttachmentLoadOp::eClear;
        case AttachmentState::CLEAR_DEPTH_SPENCIL:
            return vk::AttachmentLoadOp::eClear;
        default:
            assert(false);
            return vk::AttachmentLoadOp::eDontCare;
        }
    }

    ImageUsage::Bits AttachmentStateToImageUsage(AttachmentState state)
    {
        switch (state)
        {
        case AttachmentState::DISCARD_COLOR:
            return ImageUsage::COLOR_ATTACHMENT;
        case AttachmentState::DISCARD_DEPTH_SPENCIL:
            return ImageUsage::DEPTH_SPENCIL_ATTACHMENT;
        case AttachmentState::LOAD_COLOR:
            return ImageUsage::COLOR_ATTACHMENT;
        case AttachmentState::LOAD_DEPTH_SPENCIL:
            return ImageUsage::DEPTH_SPENCIL_ATTACHMENT;
        case AttachmentState::CLEAR_COLOR:
            return ImageUsage::COLOR_ATTACHMENT;
        case AttachmentState::CLEAR_DEPTH_SPENCIL:
            return ImageUsage::DEPTH_SPENCIL_ATTACHMENT;
        default:
            assert(false);
            return ImageUsage::COLOR_ATTACHMENT;
        }
    }

    vk::ImageLayout ImageUsageToImageLayout(ImageUsage::Bits layout)
    {
        switch (layout)
        {
        case VulkanAbstractionLayer::ImageUsage::UNKNOWN:
            return vk::ImageLayout::eUndefined;
        case VulkanAbstractionLayer::ImageUsage::TRANSFER_SOURCE:
            return vk::ImageLayout::eTransferSrcOptimal;
        case VulkanAbstractionLayer::ImageUsage::TRANSFER_DISTINATION:
            return vk::ImageLayout::eTransferDstOptimal;
        case VulkanAbstractionLayer::ImageUsage::SHADER_READ:
            return vk::ImageLayout::eShaderReadOnlyOptimal;
        case VulkanAbstractionLayer::ImageUsage::STORAGE:
            return vk::ImageLayout::eShaderReadOnlyOptimal; // TODO: what if writes?
        case VulkanAbstractionLayer::ImageUsage::COLOR_ATTACHMENT:
            return vk::ImageLayout::eColorAttachmentOptimal;
        case VulkanAbstractionLayer::ImageUsage::DEPTH_SPENCIL_ATTACHMENT:
            return vk::ImageLayout::eDepthStencilAttachmentOptimal;
        case VulkanAbstractionLayer::ImageUsage::INPUT_ATTACHMENT:
            return vk::ImageLayout::eAttachmentOptimalKHR; // TODO: is it ok?
        case VulkanAbstractionLayer::ImageUsage::FRAGMENT_SHADING_RATE_ATTACHMENT:
            return vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR;
        default:
            assert(false);
            return vk::ImageLayout::eUndefined;
        }
    }

    vk::PipelineStageFlags ImageUsageToPipelineStage(ImageUsage::Bits layout)
    {
        switch (layout)
        {
        case VulkanAbstractionLayer::ImageUsage::UNKNOWN:
            return vk::PipelineStageFlags{ };
        case VulkanAbstractionLayer::ImageUsage::TRANSFER_SOURCE:
            return vk::PipelineStageFlagBits::eTransfer;
        case VulkanAbstractionLayer::ImageUsage::TRANSFER_DISTINATION:
            return vk::PipelineStageFlagBits::eTransfer;
        case VulkanAbstractionLayer::ImageUsage::SHADER_READ:
            return vk::PipelineStageFlagBits::eFragmentShader; // TODO: whats for vertex shader reads?
        case VulkanAbstractionLayer::ImageUsage::STORAGE:
            return vk::PipelineStageFlagBits::eFragmentShader; // TODO: whats for vertex shader reads?
        case VulkanAbstractionLayer::ImageUsage::COLOR_ATTACHMENT:
            return vk::PipelineStageFlagBits::eColorAttachmentOutput;
        case VulkanAbstractionLayer::ImageUsage::DEPTH_SPENCIL_ATTACHMENT:
            return vk::PipelineStageFlagBits::eEarlyFragmentTests; // TODO: whats for late fragment test?
        case VulkanAbstractionLayer::ImageUsage::INPUT_ATTACHMENT:
            return vk::PipelineStageFlagBits::eFragmentShader; // TODO: check if at least works
        case VulkanAbstractionLayer::ImageUsage::FRAGMENT_SHADING_RATE_ATTACHMENT:
            return vk::PipelineStageFlagBits::eFragmentShadingRateAttachmentKHR;
        default:
            assert(false);
            return vk::PipelineStageFlags{ };
        }
    }

    vk::AccessFlags ImageUsageToAccessFlags(ImageUsage::Bits layout)
    {
        switch (layout)
        {
        case ImageUsage::UNKNOWN:
            return vk::AccessFlags{ };
        case ImageUsage::TRANSFER_SOURCE:
            return vk::AccessFlagBits::eTransferRead;
        case ImageUsage::TRANSFER_DISTINATION:
            return vk::AccessFlagBits::eTransferWrite;
        case ImageUsage::STORAGE:
            return vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite; // TODO: what if storage is not read or write?
        case ImageUsage::COLOR_ATTACHMENT:
            return vk::AccessFlagBits::eColorAttachmentWrite;
        case ImageUsage::DEPTH_SPENCIL_ATTACHMENT:
            return vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        case ImageUsage::INPUT_ATTACHMENT:
            return vk::AccessFlagBits::eInputAttachmentRead;
        case ImageUsage::FRAGMENT_SHADING_RATE_ATTACHMENT:
            return vk::AccessFlagBits::eFragmentShadingRateAttachmentReadKHR;
        default:
            assert(false);
            return vk::AccessFlags{ };
        }
    }

    RenderPassNative RenderGraphBuilder::BuildRenderPass(const RenderPassBuilder& renderPassBuilder, const AttachmentHashMap& attachments, const ResourceTransitions& resourceTransitions)
    {
        std::vector<vk::AttachmentDescription> attachmentDescriptions;
        std::vector<vk::AttachmentReference> attachmentReferences;
        std::vector<vk::ImageView> attachmentViews;
        std::vector<vk::ClearValue> clearValues;
        
        uint32_t renderAreaWidth = 0, renderAreaHeight = 0;

        DependencyStorage dependencies;
        renderPassBuilder.renderPass->SetupDependencies(dependencies);
        auto& renderPassAttachments = dependencies.GetAttachmentDependencies();
        auto& attachmentTransitions = resourceTransitions.ImageTransitions.at(renderPassBuilder.name);
        std::optional<vk::AttachmentReference> depthSpencilAttachmentReference;

        for (size_t attachmentIndex = 0; attachmentIndex < renderPassAttachments.size(); attachmentIndex++)
        {
            const auto& attachment = renderPassAttachments[attachmentIndex];
            const auto& imageReference = attachments.at(attachment.Name);
            const auto& attachmentTransition = attachmentTransitions.at(attachment.Name);

            if (renderAreaWidth == 0 && renderAreaHeight == 0)
            {
                renderAreaWidth = std::max(renderAreaWidth, (uint32_t)imageReference.GetWidth());
                renderAreaHeight = std::max(renderAreaHeight, (uint32_t)imageReference.GetHeight());
            }
        
            vk::AttachmentDescription attachmentDescription;
            attachmentDescription
                .setFormat(ToNative(imageReference.GetFormat()))
                .setSamples(vk::SampleCountFlagBits::e1)
                .setLoadOp(AttachmentStateToLoadOp(attachment.OnLoad))
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setInitialLayout(ImageUsageToImageLayout(attachmentTransition.InitialUsage))
                .setFinalLayout(ImageUsageToImageLayout(attachmentTransition.FinalUsage));
        
            attachmentDescriptions.push_back(std::move(attachmentDescription));
            attachmentViews.push_back(imageReference.GetNativeView());
        
            vk::AttachmentReference attachmentReference;
            attachmentReference
                .setAttachment((uint32_t)attachmentIndex)
                .setLayout(ImageUsageToImageLayout(attachmentTransition.FinalUsage));
        
            if (attachmentTransition.FinalUsage == ImageUsage::DEPTH_SPENCIL_ATTACHMENT)
            {
                depthSpencilAttachmentReference = std::move(attachmentReference);
                clearValues.push_back(vk::ClearDepthStencilValue{ 
                    attachment.DepthSpencilClear.Depth, attachment.DepthSpencilClear.Spencil 
                });
            }
            else
            {
                attachmentReferences.push_back(std::move(attachmentReference));
                clearValues.push_back(vk::ClearColorValue{ 
                    std::array{ attachment.ColorClear.R, attachment.ColorClear.G, attachment.ColorClear.B, attachment.ColorClear.A } 
                });
            }
        }

        vk::SubpassDescription subpassDescription;
        subpassDescription
            .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setColorAttachments(attachmentReferences)
            .setPDepthStencilAttachment(depthSpencilAttachmentReference.has_value() ? 
                std::addressof(depthSpencilAttachmentReference.value()) : nullptr
            );
        
        // TODO
        std::array subpassDependencies = {
            vk::SubpassDependency {
                VK_SUBPASS_EXTERNAL,
                0,
                vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
                vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
                vk::AccessFlagBits::eMemoryRead,
                vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                vk::DependencyFlagBits::eByRegion
            },
            vk::SubpassDependency {
                0,
                VK_SUBPASS_EXTERNAL,
                vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
                vk::PipelineStageFlagBits::eBottomOfPipe,
                vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                vk::AccessFlagBits::eMemoryRead,
                vk::DependencyFlagBits::eByRegion
            },
        };
        
        vk::RenderPassCreateInfo renderPassCreateInfo;
        renderPassCreateInfo
            .setAttachments(attachmentDescriptions)
            .setSubpasses(subpassDescription)
            .setDependencies(subpassDependencies);
        
        auto renderPass = GetCurrentVulkanContext().GetDevice().createRenderPass(renderPassCreateInfo);
        
        vk::FramebufferCreateInfo framebufferCreateInfo;
        framebufferCreateInfo
            .setRenderPass(renderPass)
            .setAttachments(attachmentViews)
            .setWidth(renderAreaWidth)
            .setHeight(renderAreaHeight)
            .setLayers(1);
        
        auto framebuffer = GetCurrentVulkanContext().GetDevice().createFramebuffer(framebufferCreateInfo);
        
        auto renderArea = vk::Rect2D{ vk::Offset2D{ 0u, 0u }, vk::Extent2D{ renderAreaWidth, renderAreaHeight } };
        
        vk::Pipeline pipeline;
        vk::PipelineLayout pipelineLayout;
        vk::DescriptorSet descriptorSet;
        vk::DescriptorSetLayout descriptorSetLayout;
        if (renderPassBuilder.graphicPipeline.has_value())
        {
            const auto& graphicPipeline = renderPassBuilder.graphicPipeline.value();
            auto descriptor = GetCurrentVulkanContext().GetDescriptorCache().GetDescriptor(graphicPipeline.Shader.GetShaderUniforms());
            descriptorSet = descriptor.Set;
            descriptorSetLayout = descriptor.SetLayout;

            std::array shaderStageCreateInfos = {
                vk::PipelineShaderStageCreateInfo {
                    vk::PipelineShaderStageCreateFlags{ },
                    ToNative(ShaderType::VERTEX),
                    graphicPipeline.Shader.GetNativeShader(ShaderType::VERTEX),
                    "main"
                },
                vk::PipelineShaderStageCreateInfo {
                    vk::PipelineShaderStageCreateFlags{ },
                    ToNative(ShaderType::FRAGMENT),
                    graphicPipeline.Shader.GetNativeShader(ShaderType::FRAGMENT),
                    "main"
                }
            };
            
            auto& vertexAttributes = graphicPipeline.Shader.GetVertexAttributes();
            auto& vertexBindings = graphicPipeline.VertexBindings;            

            std::vector<vk::VertexInputBindingDescription> vertexBindingDescriptions;
            std::vector<vk::VertexInputAttributeDescription> vertexAttributeDescriptions;
            uint32_t vertexBindingId = 0, attributeLocationId = 0, vertexBindingOffset = 0;
            uint32_t attributeLocationOffset = 0;

            for (const auto& attribute : vertexAttributes)
            {
                for (size_t i = 0; i < attribute.ComponentCount; i++)
                {
                    vertexAttributeDescriptions.push_back(
                        vk::VertexInputAttributeDescription{
                            attributeLocationId,
                            vertexBindingId,
                            ToNative(attribute.LayoutFormat),
                            vertexBindingOffset,
                        });
                    attributeLocationId++;
                    vertexBindingOffset += attribute.ByteSize / attribute.ComponentCount;
                }

                if (attributeLocationId == attributeLocationOffset + vertexBindings[vertexBindingId].BindingRange)
                {
                    vertexBindingDescriptions.push_back(
                        vk::VertexInputBindingDescription{
                            vertexBindingId,
                            vertexBindingOffset,
                            InputRateTable[(size_t)vertexBindings[vertexBindingId].InputRate]
                        });
                    vertexBindingId++;
                    attributeLocationOffset = attributeLocationId;
                    vertexBindingOffset = 0; // vertex binding offset is local to binding
                }
            }
            // move everything else to last vertex buffer
            if (attributeLocationId != attributeLocationOffset)
            {
                vertexBindingDescriptions.push_back(
                    vk::VertexInputBindingDescription{
                        vertexBindingId,
                        vertexBindingOffset,
                        InputRateTable[(size_t)vertexBindings[vertexBindingId].InputRate]
                    });
            }

            vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
            vertexInputStateCreateInfo
                .setVertexBindingDescriptions(vertexBindingDescriptions)
                .setVertexAttributeDescriptions(vertexAttributeDescriptions);

            vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
            inputAssemblyStateCreateInfo
                .setPrimitiveRestartEnable(false)
                .setTopology(vk::PrimitiveTopology::eTriangleList);

            vk::PipelineViewportStateCreateInfo viewportStateCreateInfo;
            viewportStateCreateInfo
                .setViewportCount(1) // defined dynamic
                .setScissorCount(1); // defined dynamic

            vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
            rasterizationStateCreateInfo
                .setPolygonMode(vk::PolygonMode::eFill)
                .setCullMode(vk::CullModeFlagBits::eBack)
                .setFrontFace(vk::FrontFace::eCounterClockwise)
                .setLineWidth(1.0f);

            vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
            multisampleStateCreateInfo
                .setRasterizationSamples(vk::SampleCountFlagBits::e1)
                .setMinSampleShading(1.0f);

            vk::PipelineColorBlendAttachmentState colorBlendAttachmentState;
            colorBlendAttachmentState
                .setBlendEnable(true)
                .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                .setColorBlendOp(vk::BlendOp::eAdd)
                .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                .setAlphaBlendOp(vk::BlendOp::eAdd)
                .setColorWriteMask(
                    vk::ColorComponentFlagBits::eR | 
                    vk::ColorComponentFlagBits::eG | 
                    vk::ColorComponentFlagBits::eB | 
                    vk::ColorComponentFlagBits::eA
                );

            vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo;
            colorBlendStateCreateInfo
                .setLogicOpEnable(false)
                .setLogicOp(vk::LogicOp::eCopy)
                .setAttachments(colorBlendAttachmentState)
                .setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

            std::array dynamicStates = {
                vk::DynamicState::eViewport,
                vk::DynamicState::eScissor,
            };

            vk::PipelineDepthStencilStateCreateInfo depthSpencilStateCreateInfo;
            depthSpencilStateCreateInfo
                .setStencilTestEnable(false)
                .setDepthTestEnable(true)
                .setDepthWriteEnable(true)
                .setDepthBoundsTestEnable(false)
                .setDepthCompareOp(vk::CompareOp::eLess)
                .setMinDepthBounds(0.0f)
                .setMaxDepthBounds(1.0f);

            vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo;
            dynamicStateCreateInfo.setDynamicStates(dynamicStates);

            vk::PipelineLayoutCreateInfo layoutCreateInfo;
            layoutCreateInfo.setSetLayouts(descriptorSetLayout);

            pipelineLayout = GetCurrentVulkanContext().GetDevice().createPipelineLayout(layoutCreateInfo);

            vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
            pipelineCreateInfo
                .setStages(shaderStageCreateInfos)
                .setPVertexInputState(&vertexInputStateCreateInfo)
                .setPInputAssemblyState(&inputAssemblyStateCreateInfo)
                .setPTessellationState(nullptr)
                .setPViewportState(&viewportStateCreateInfo)
                .setPRasterizationState(&rasterizationStateCreateInfo)
                .setPMultisampleState(&multisampleStateCreateInfo)
                .setPDepthStencilState(&depthSpencilStateCreateInfo)
                .setPColorBlendState(&colorBlendStateCreateInfo)
                .setPDynamicState(&dynamicStateCreateInfo)
                .setLayout(pipelineLayout)
                .setRenderPass(renderPass)
                .setSubpass(0)
                .setBasePipelineHandle(vk::Pipeline{ })
                .setBasePipelineIndex(0);

            pipeline = GetCurrentVulkanContext().GetDevice().createGraphicsPipeline(vk::PipelineCache{ }, pipelineCreateInfo).value;

            graphicPipeline.DescriptorBindings.Write(descriptorSet);
        }

        return RenderPassNative{ renderPass, descriptorSet, framebuffer, pipeline, pipelineLayout, renderArea, clearValues };
    }

    RenderGraphBuilder::DependencyHashMap RenderGraphBuilder::AcquireRenderPassDependencies()
    {
        DependencyHashMap dependencies;
        for (auto& renderPassBuilder : this->renderPasses)
        {
            auto& dependency = dependencies[renderPassBuilder.name];
            renderPassBuilder.renderPass->SetupDependencies(dependency);
        }
        return dependencies;
    }

    RenderGraphBuilder::ResourceTransitions RenderGraphBuilder::ResolveResourceTransitions(const DependencyHashMap& dependencies)
    {
        ResourceTransitions resourceTransitions;
        std::unordered_map<StringId, ImageUsage::Bits> lastImageUsages = this->externalImagesInitialUsage;
        std::unordered_map<StringId, BufferUsage::Bits> lastBufferUsages = this->externalBufferInitialUsage;

        for (const auto& renderPassBuilder : this->renderPasses)
        {
            auto& renderPassDependencies = dependencies.at(renderPassBuilder.name);
            auto& bufferTransitions = resourceTransitions.BufferTransitions[renderPassBuilder.name];
            auto& imageTransitions = resourceTransitions.ImageTransitions[renderPassBuilder.name];

            for (const auto& bufferDependency : renderPassDependencies.GetBufferDependencies())
            {
                auto lastBufferUsage = lastBufferUsages.at(bufferDependency.Name);
                bufferTransitions[bufferDependency.Name].InitialUsage = lastBufferUsage;
                bufferTransitions[bufferDependency.Name].FinalUsage = bufferDependency.Usage;
                resourceTransitions.TotalBufferUsages[bufferDependency.Name] |= bufferDependency.Usage;
                lastBufferUsages[bufferDependency.Name] = bufferDependency.Usage;
            }

            for (const auto& imageDependency : renderPassDependencies.GetImageDependencies())
            {
                auto lastImageUsage = lastImageUsages.at(imageDependency.Name);
                imageTransitions[imageDependency.Name].InitialUsage = lastImageUsage;
                imageTransitions[imageDependency.Name].FinalUsage = imageDependency.Usage;
                resourceTransitions.TotalImageUsages[imageDependency.Name] |= imageDependency.Usage;
                lastImageUsages[imageDependency.Name] = imageDependency.Usage;
            }

            for (const auto& attachmentDependency : renderPassDependencies.GetAttachmentDependencies())
            {
                auto attachmentDependencyUsage = AttachmentStateToImageUsage(attachmentDependency.OnLoad);
                auto lastImageUsage = lastImageUsages.at(attachmentDependency.Name);
                imageTransitions[attachmentDependency.Name].InitialUsage = lastImageUsage;
                imageTransitions[attachmentDependency.Name].FinalUsage = attachmentDependencyUsage;
                resourceTransitions.TotalImageUsages[attachmentDependency.Name] |= attachmentDependencyUsage;
                lastImageUsages[attachmentDependency.Name] = attachmentDependencyUsage;
            }
        }

        return resourceTransitions;
    }

    RenderGraphBuilder::AttachmentHashMap RenderGraphBuilder::AllocateAttachments(const ResourceTransitions& transitions, const DependencyHashMap& dependencies)
    {
        AttachmentHashMap attachments;

        for (const auto& [attachmentName, attachmentUsage] : transitions.TotalImageUsages)
        {
            auto& attachmentCreateOptions = this->attachmentsCreateOptions.at(attachmentName);

            attachments.emplace(attachmentName, Image(
                attachmentCreateOptions.Width,
                attachmentCreateOptions.Height,
                attachmentCreateOptions.LayoutFormat,
                attachmentUsage,
                MemoryUsage::GPU_ONLY
            ));
        }
        return attachments;
    }

    void RenderGraphBuilder::SetupOutputImage(ResourceTransitions& transitions, StringId outputImage)
    {
        transitions.TotalImageUsages.at(outputImage) |= ImageUsage::TRANSFER_SOURCE;
    }

    RenderGraphBuilder& RenderGraphBuilder::AddRenderPass(RenderPassBuilder&& renderPass)
    {
        this->renderPasses.push_back(std::move(renderPass));
        return *this;
    }

    RenderGraphBuilder& RenderGraphBuilder::AddRenderPass(RenderPassBuilder& renderPass)
    {
        // bad but simplifies user code. As RenderPassBuilder is move-only, chain function which return reference cannot match other Add function
        this->renderPasses.push_back(std::move(renderPass));
        return *this;
    }

    RenderGraphBuilder& RenderGraphBuilder::SetOutputName(StringId name)
    {
        this->outputName = name;
        return *this;
    }

    RenderGraphBuilder& RenderGraphBuilder::AddAttachment(StringId name, Format format)
    {
        auto& vulkanContext = GetCurrentVulkanContext();
        auto [width, height] = vulkanContext.GetSurfaceExtent();
        return this->AddAttachment(name, format, width, height);
    }

    RenderGraphBuilder& RenderGraphBuilder::AddAttachment(StringId name, Format format, uint32_t width, uint32_t height)
    {
        this->attachmentsCreateOptions.emplace(name, AttachmentCreateOptions{ format, width, height });
        this->AddExternalImage(name, ImageUsage::UNKNOWN);
        return *this;
    }

    RenderGraphBuilder& RenderGraphBuilder::AddExternalImage(StringId name, ImageUsage::Bits initialUsage)
    {
        this->externalImagesInitialUsage[name] = initialUsage;
        return *this;
    }

    RenderGraphBuilder& RenderGraphBuilder::AddExternalBuffer(StringId name, BufferUsage::Bits initialUsage)
    {
        this->externalBufferInitialUsage[name] = initialUsage;
        return *this;
    }

    void RenderGraphBuilder::PreWarmDescriptorSets()
    {
        auto& descriptorCache = GetCurrentVulkanContext().GetDescriptorCache();
        for (const auto& renderPass : this->renderPasses)
        {
            if (renderPass.graphicPipeline.has_value())
            {
                auto& uniforms = renderPass.graphicPipeline->Shader.GetShaderUniforms();
                (void)descriptorCache.GetDescriptor(uniforms);
            }
        }
    }

    RenderGraph RenderGraphBuilder::Build()
    {
        auto dependencies = this->AcquireRenderPassDependencies();
        auto resourceTransitions = this->ResolveResourceTransitions(dependencies);
        this->SetupOutputImage(resourceTransitions, this->outputName);
        AttachmentHashMap attachments = this->AllocateAttachments(resourceTransitions, dependencies);

        std::vector<RenderGraphNode> nodes;

        this->PreWarmDescriptorSets();

        for (auto& renderPassBuilder : this->renderPasses)
        {
            std::vector<StringId> colorAttachments;
            for (const auto& attachment : dependencies[renderPassBuilder.name].GetAttachmentDependencies())
                colorAttachments.push_back(attachment.Name);

            nodes.push_back(RenderGraphNode{
                renderPassBuilder.name,
                this->BuildRenderPass(renderPassBuilder, attachments, resourceTransitions),
                std::move(renderPassBuilder.renderPass),
                std::move(colorAttachments)
            });
        }

        auto OnPresent = [](CommandBuffer& commandBuffer, const Image& outputImage, const Image& presentImage)
        {
            constexpr auto attachmentType = ImageUsage::COLOR_ATTACHMENT;
            commandBuffer.BlitImage(
                outputImage, ImageUsageToImageLayout(attachmentType), ImageUsageToAccessFlags(attachmentType),
                presentImage, vk::ImageLayout::eUndefined, vk::AccessFlagBits::eMemoryRead,
                ImageUsageToPipelineStage(attachmentType), vk::Filter::eLinear
            );
        };

        auto OnDestroy = [](const RenderPassNative& pass)
        {
            auto& device = GetCurrentVulkanContext().GetDevice();
            if ((bool)pass.Pipeline)            device.destroyPipeline(pass.Pipeline);
            if ((bool)pass.PipelineLayout)      device.destroyPipelineLayout(pass.PipelineLayout);
            if ((bool)pass.Framebuffer)         device.destroyFramebuffer(pass.Framebuffer);
            if ((bool)pass.RenderPassHandle)          device.destroyRenderPass(pass.RenderPassHandle);
        };

        return RenderGraph{ std::move(nodes), std::move(attachments), std::move(this->outputName), std::move(OnPresent), std::move(OnDestroy) };
    }
}