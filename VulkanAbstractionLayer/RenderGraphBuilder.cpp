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

    RenderPassBuilder& RenderPassBuilder::AddOnRenderCallback(RenderGraphNode::RenderCallback callback)
    {
        this->onRenderCallback = std::move(callback);
        return *this;
    }

    RenderPassBuilder& RenderPassBuilder::AddBeforeRenderCallback(RenderGraphNode::RenderCallback callback)
    {
        this->beforeRenderCallback = std::move(callback);
        return *this;
    }

    RenderPassBuilder& RenderPassBuilder::AddAfterRenderCallback(RenderGraphNode::RenderCallback callback)
    {
        this->afterRenderCallback = std::move(callback);
        return *this;
    }

    RenderPassBuilder& RenderPassBuilder::AddReadOnlyColorAttachment(StringId name)
    {
        this->inputColorAttachments.push_back(ReadOnlyColorAttachment{ name });
        return *this;
    }

    RenderPassBuilder& RenderPassBuilder::AddWriteOnlyColorAttachment(StringId name)
    {
        this->outputColorAttachments.push_back(WriteOnlyColorAttachment{ name });
        return *this;
    }

    RenderPassBuilder& RenderPassBuilder::AddWriteOnlyColorAttachment(StringId name, ClearColor clear)
    {
        this->outputColorAttachments.push_back(WriteOnlyColorAttachment{ name, AttachmentLayout::UNKWNON, AttachmentInitialState::CLEAR, clear });
        return *this;
    }

    RenderPassBuilder& RenderPassBuilder::AddWriteOnlyColorAttachment(StringId name, AttachmentInitialState state)
    {
        this->outputColorAttachments.push_back(WriteOnlyColorAttachment{ name, AttachmentLayout::UNKWNON, state });
        return *this;
    }

    RenderPassBuilder& RenderPassBuilder::SetWriteOnlyDepthAttachment(StringId name)
    {
        this->depthAttachment = WriteOnlyDepthAttachment{ name };
        return *this;
    }

    RenderPassBuilder& RenderPassBuilder::SetWriteOnlyDepthAttachment(StringId name, ClearDepthSpencil clear)
    {
        this->depthAttachment = WriteOnlyDepthAttachment{ name, AttachmentLayout::UNKWNON, AttachmentInitialState::CLEAR, clear };
        return *this;
    }

    RenderPassBuilder& RenderPassBuilder::SetWriteOnlyDepthAttachment(StringId name, AttachmentInitialState state)
    {
        this->depthAttachment = WriteOnlyDepthAttachment{ name, AttachmentLayout::UNKWNON, state };
        return *this;
    }

    RenderPassBuilder& RenderPassBuilder::SetPipeline(GraphicPipeline pipeline)
    {
        this->graphicPipeline = std::move(pipeline);
        return *this;
    }

    std::array LayoutTable = {
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::eDepthAttachmentOptimal,
        vk::ImageLayout::eTransferSrcOptimal,
        vk::ImageLayout::eTransferDstOptimal,
    };

    std::array LoadOpTable = {
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentLoadOp::eLoad,
    };

    std::array InputRateTable = {
        vk::VertexInputRate::eVertex,
        vk::VertexInputRate::eInstance,
    };

    vk::AccessFlags AttachmentLayoutToAccessFlags(AttachmentLayout layout)
    {
        switch (layout)
        {
        case AttachmentLayout::UNKWNON:
            return vk::AccessFlags{ };
        case AttachmentLayout::SHADER_READ:
            return vk::AccessFlagBits::eShaderRead;
        case AttachmentLayout::COLOR_ATTACHMENT:
            return vk::AccessFlagBits::eColorAttachmentWrite;
        case AttachmentLayout::DEPTH_ATTACHMENT:
            return vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        case AttachmentLayout::TRANSFER_SOURCE:
            return vk::AccessFlagBits::eTransferRead;
        case AttachmentLayout::TRANSFER_DISTANCE:
            return vk::AccessFlagBits::eTransferWrite;
        default:
            assert(false);
            return vk::AccessFlags{ };
        }
    }

    vk::ImageUsageFlags AttachmentLayoutToImageUsage(AttachmentLayout layout)
    {
        switch (layout)
        {
        case AttachmentLayout::UNKWNON:
            return vk::ImageUsageFlags{ };
        case AttachmentLayout::SHADER_READ:
            return vk::ImageUsageFlagBits::eSampled;
        case AttachmentLayout::COLOR_ATTACHMENT:
            return vk::ImageUsageFlagBits::eColorAttachment;
        case AttachmentLayout::DEPTH_ATTACHMENT:
            return vk::ImageUsageFlagBits::eDepthStencilAttachment;
        case AttachmentLayout::TRANSFER_SOURCE:
            return vk::ImageUsageFlagBits::eTransferSrc;
        case AttachmentLayout::TRANSFER_DISTANCE:
            return vk::ImageUsageFlagBits::eTransferDst;
        default:
            assert(false);
            return vk::ImageUsageFlags{ };
        }
    }

    RenderPass RenderGraphBuilder::BuildRenderPass(const RenderPassBuilder& renderPassBuilder, const AttachmentHashMap& attachments)
    {
        std::vector<vk::AttachmentDescription> attachmentDescriptions;
        std::vector<vk::AttachmentReference> attachmentReferences;
        std::vector<vk::ImageView> attachmentViews;
        std::vector<vk::ClearValue> clearValues;
        
        uint32_t renderAreaWidth = 0, renderAreaHeight = 0;
        
        for (size_t attachmentIndex = 0; attachmentIndex < renderPassBuilder.outputColorAttachments.size(); attachmentIndex++)
        {
            const WriteOnlyColorAttachment& outputColorAttachment = renderPassBuilder.outputColorAttachments[attachmentIndex];
            const Image& imageReference = attachments.at(outputColorAttachment.Name);

            if (renderAreaWidth == 0 && renderAreaHeight == 0)
            {
                renderAreaWidth = std::max(renderAreaWidth, (uint32_t)imageReference.GetWidth());
                renderAreaHeight = std::max(renderAreaHeight, (uint32_t)imageReference.GetHeight());
            }
        
            vk::AttachmentDescription attachmentDescription;
            attachmentDescription
                .setFormat(ToNativeFormat(imageReference.GetFormat()))
                .setSamples(vk::SampleCountFlagBits::e1)
                .setLoadOp(LoadOpTable[(size_t)outputColorAttachment.InitialState])
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setInitialLayout(LayoutTable[(size_t)outputColorAttachment.InitialLayout])
                .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
        
            attachmentDescriptions.push_back(std::move(attachmentDescription));
        
            vk::AttachmentReference attachmentReference;
            attachmentReference
                .setAttachment((uint32_t)attachmentIndex)
                .setLayout(vk::ImageLayout::eColorAttachmentOptimal);
        
            attachmentReferences.push_back(std::move(attachmentReference));
            attachmentViews.push_back(imageReference.GetNativeView());
        
            ClearColor clear = outputColorAttachment.ClearValue;
            clearValues.push_back(vk::ClearValue{ vk::ClearColorValue{ std::array{ clear.R, clear.G, clear.B, clear.A } } });
        }
        
        const WriteOnlyDepthAttachment& depthAttachment = renderPassBuilder.depthAttachment;
        vk::AttachmentReference depthAttachmentReference;

        if (depthAttachment.Name != StringId{ })
        {
            const Image& depthImageReference = attachments.at(depthAttachment.Name);

            vk::AttachmentDescription depthAttachmentDescription;
            depthAttachmentDescription
                .setFormat(ToNativeFormat(depthImageReference.GetFormat()))
                .setSamples(vk::SampleCountFlagBits::e1)
                .setLoadOp(LoadOpTable[(size_t)depthAttachment.InitialState])
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setInitialLayout(LayoutTable[(size_t)depthAttachment.InitialLayout])
                .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

            attachmentDescriptions.push_back(std::move(depthAttachmentDescription));

            depthAttachmentReference
                .setAttachment((uint32_t)(attachmentDescriptions.size() - 1))
                .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

            attachmentViews.push_back(depthImageReference.GetNativeView());

            ClearDepthSpencil depthClear = depthAttachment.ClearValue;
            clearValues.push_back(vk::ClearValue{ vk::ClearDepthStencilValue{ depthClear.Depth, depthClear.Spencil } });
        }

        vk::SubpassDescription subpassDescription;
        subpassDescription
            .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setColorAttachments(attachmentReferences)
            .setPDepthStencilAttachment(depthAttachmentReference != vk::AttachmentReference{ } ? &depthAttachmentReference : nullptr);
        
        // TODO
        std::array dependencies = {
            vk::SubpassDependency {
                VK_SUBPASS_EXTERNAL,
                0,
                vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::AccessFlagBits::eMemoryRead,
                vk::AccessFlagBits::eColorAttachmentWrite,
                vk::DependencyFlagBits::eByRegion
            },
            vk::SubpassDependency {
                0,
                VK_SUBPASS_EXTERNAL,
                vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::PipelineStageFlagBits::eBottomOfPipe,
                vk::AccessFlagBits::eColorAttachmentWrite,
                vk::AccessFlagBits::eMemoryRead,
                vk::DependencyFlagBits::eByRegion
            },
        };
        
        vk::RenderPassCreateInfo renderPassCreateInfo;
        renderPassCreateInfo
            .setAttachments(attachmentDescriptions)
            .setSubpasses(subpassDescription)
            .setDependencies(dependencies);
        
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
        if (renderPassBuilder.graphicPipeline.has_value())
        {
            const auto& graphicPipeline = renderPassBuilder.graphicPipeline.value();

            std::array shaderStageCreateInfos = {
                vk::PipelineShaderStageCreateInfo {
                    vk::PipelineShaderStageCreateFlags{ },
                    vk::ShaderStageFlagBits::eVertex,
                    graphicPipeline.Shader.GetNativeShader(ShaderType::VERTEX),
                    "main"
                },
                vk::PipelineShaderStageCreateInfo {
                    vk::PipelineShaderStageCreateFlags{ },
                    vk::ShaderStageFlagBits::eFragment,
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
                            ToNativeFormat(attribute.LayoutFormat),
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
                .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

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
                .setDepthCompareOp(vk::CompareOp::eLess)
                .setMinDepthBounds(0.0f)
                .setMaxDepthBounds(1.0f);

            vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo;
            dynamicStateCreateInfo.setDynamicStates(dynamicStates);

            vk::PipelineLayoutCreateInfo layoutCreateInfo;
            layoutCreateInfo.setSetLayouts(graphicPipeline.Shader.GetDescriptorSetLayout());

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
        }

        return RenderPass{ renderPass, pipeline, pipelineLayout, renderArea, framebuffer, clearValues };
    }

    AttachmentLayout RenderGraphBuilder::ResolveImageTransitions(StringId outputName)
    {
        std::unordered_map<StringId, AttachmentLayout> layoutTransitions;
        for (auto& renderPass : this->renderPasses)
        {
            for (auto& inputAttachment : renderPass.inputColorAttachments)
            {
                inputAttachment.InitialLayout = layoutTransitions[inputAttachment.Name];
                layoutTransitions[inputAttachment.Name] = AttachmentLayout::SHADER_READ;
            }
            for (auto& outputAttachment : renderPass.outputColorAttachments)
            {
                outputAttachment.InitialLayout = layoutTransitions[outputAttachment.Name];
                layoutTransitions[outputAttachment.Name] = AttachmentLayout::COLOR_ATTACHMENT;
            }
            renderPass.depthAttachment.InitialLayout = layoutTransitions[renderPass.depthAttachment.Name];
            layoutTransitions[renderPass.depthAttachment.Name] = AttachmentLayout::DEPTH_ATTACHMENT;
        }
        return layoutTransitions[outputName];
    }

    RenderGraphBuilder::AttachmentHashMap RenderGraphBuilder::AllocateAttachments()
    {
        AttachmentHashMap attachments;

        std::unordered_map<StringId, vk::ImageUsageFlags> attachmentUsages;
        for (const auto& renderPass : this->renderPasses)
        {
            for (const auto& colorAttachment : renderPass.inputColorAttachments)
            {
                attachmentUsages[colorAttachment.Name] |= AttachmentLayoutToImageUsage(AttachmentLayout::SHADER_READ);
            }
            for (const auto& colorAttachment : renderPass.outputColorAttachments)
            {
                attachmentUsages[colorAttachment.Name] |= AttachmentLayoutToImageUsage(AttachmentLayout::COLOR_ATTACHMENT);
            }
            attachmentUsages[renderPass.depthAttachment.Name] |= AttachmentLayoutToImageUsage(AttachmentLayout::DEPTH_ATTACHMENT);
        }
        attachmentUsages[this->outputName] |= AttachmentLayoutToImageUsage(AttachmentLayout::TRANSFER_SOURCE);
        // remove empty attachment
        attachmentUsages.erase(StringId{ });

        auto& vulkanContext = GetCurrentVulkanContext();

        for (const auto& [attachmentName, attachmentUsage] : attachmentUsages)
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
        return *this;
    }

    RenderGraph RenderGraphBuilder::Build()
    {
        AttachmentLayout outputLayout = this->ResolveImageTransitions(this->outputName);
        AttachmentHashMap attachments = this->AllocateAttachments();

        std::vector<RenderGraphNode> nodes;
        for (auto& renderPass : this->renderPasses)
        {
            std::vector<StringId> colorAttachments;
            for (const auto& colorAttachment : renderPass.outputColorAttachments)
                colorAttachments.push_back(colorAttachment.Name);

            nodes.push_back(RenderGraphNode{
                renderPass.name,
                this->BuildRenderPass(renderPass, attachments),
                std::move(renderPass.beforeRenderCallback),
                std::move(renderPass.onRenderCallback),
                std::move(renderPass.afterRenderCallback),
                std::move(colorAttachments)
            });
        }

        auto OnPresent = [outputLayout](CommandBuffer& commandBuffer, const Image& outputImage, const Image& presentImage)
        {
            commandBuffer.BlitImage(
                outputImage, LayoutTable[(size_t)outputLayout], AttachmentLayoutToAccessFlags(outputLayout), 
                presentImage, vk::ImageLayout::eUndefined, vk::AccessFlagBits::eMemoryRead,
                vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::Filter::eLinear
            );
        };

        auto OnDestroy = [](const RenderPass& pass)
        {
            auto& device = GetCurrentVulkanContext().GetDevice();
            if((bool)pass.GetPipeline())       device.destroyPipeline(pass.GetPipeline());
            if((bool)pass.GetPipelineLayout()) device.destroyPipelineLayout(pass.GetPipelineLayout());
            if((bool)pass.GetFramebuffer())    device.destroyFramebuffer(pass.GetFramebuffer());
            if((bool)pass.GetNativeHandle())   device.destroyRenderPass(pass.GetNativeHandle());
        };

        return RenderGraph{ std::move(nodes), std::move(attachments), std::move(this->outputName), std::move(OnPresent), std::move(OnDestroy) };
    }
}