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

#include "GraphicShader.h"
#include "VectorMath.h"
#include "VulkanContext.h"

namespace VulkanAbstractionLayer
{
    GraphicShader::~GraphicShader()
    {
        this->Destroy();
    }

    void GraphicShader::Destroy()
    {
        auto& vulkan = GetCurrentVulkanContext();
        auto& device = vulkan.GetDevice();
        if ((bool)this->vertexShader) device.destroyShaderModule(this->vertexShader);
        if ((bool)this->fragmentShader) device.destroyShaderModule(this->fragmentShader);
        if ((bool)this->tessControlShader) device.destroyShaderModule(this->tessControlShader);
        if ((bool)this->tessEvalShader) device.destroyShaderModule(this->tessEvalShader);

        this->vertexShader = vk::ShaderModule{ };
        this->fragmentShader = vk::ShaderModule{ };
        this->tessControlShader = vk::ShaderModule{ };
        this->tessEvalShader = vk::ShaderModule{ };
    }

    GraphicShader::GraphicShader(const ShaderData& vertex, const ShaderData& fragment)
    {
        this->Init(vertex, fragment);
    }

    GraphicShader::GraphicShader(const ShaderData& vertex, const ShaderData& tessControl, const ShaderData& tessEval, const ShaderData& fragment)
	    :GraphicShader(std::move(vertex),std::move(fragment))
    {
        auto& context = GetCurrentVulkanContext();
        auto& device = context.GetDevice();

        assert(!tessControl.Bytecode.empty());
        vk::ShaderModuleCreateInfo tessControlShaderInfo;
        tessControlShaderInfo.setCode(tessControl.Bytecode);
        this->tessControlShader = device.createShaderModule(tessControlShaderInfo);

        assert(!tessEval.Bytecode.empty());
        vk::ShaderModuleCreateInfo tessEvalShaderInfo;
        tessEvalShaderInfo.setCode(tessEval.Bytecode);
        this->tessEvalShader = device.createShaderModule(tessEvalShaderInfo);
        
        this->shaderUniforms.push_back(ShaderUniforms{ tessControl.DescriptorSets[0], ShaderType::TESS_CONTROL });
        this->shaderUniforms.push_back(ShaderUniforms{ tessEval.DescriptorSets[0], ShaderType::TESS_EVALUATION });
        
    }

    void GraphicShader::Init(const ShaderData& vertex, const ShaderData& fragment)
    {
        auto& vulkan = GetCurrentVulkanContext();

        vk::ShaderModuleCreateInfo vertexShaderInfo;
        vertexShaderInfo.setCode(vertex.Bytecode);
        this->vertexShader = vulkan.GetDevice().createShaderModule(vertexShaderInfo);

        vk::ShaderModuleCreateInfo fragmentShaderInfo;
        fragmentShaderInfo.setCode(fragment.Bytecode);
        this->fragmentShader = vulkan.GetDevice().createShaderModule(fragmentShaderInfo);

        this->inputAttributes = vertex.InputAttributes;
        // TODO: support multiple descriptor sets
        assert(vertex.DescriptorSets.size() < 2);
        assert(fragment.DescriptorSets.size() < 2);
        this->shaderUniforms = std::vector{
            ShaderUniforms{ vertex.DescriptorSets[0], ShaderType::VERTEX },
            ShaderUniforms{ fragment.DescriptorSets[0], ShaderType::FRAGMENT },
        };
    }

    GraphicShader::GraphicShader(GraphicShader&& other) noexcept
    {
        this->vertexShader = other.vertexShader;
        this->fragmentShader = other.fragmentShader;
        this->inputAttributes = std::move(other.inputAttributes);
        this->shaderUniforms = std::move(other.shaderUniforms);

        other.vertexShader = vk::ShaderModule{ };
        other.fragmentShader = vk::ShaderModule{ };

        if((bool)other.tessControlShader)
        {
            this->tessControlShader = other.tessControlShader;
            this->tessEvalShader = other.tessEvalShader;
            other.tessControlShader = VkShaderModule{};
            other.tessEvalShader = VkShaderModule{};
        }
    }

    GraphicShader& GraphicShader::operator=(GraphicShader&& other) noexcept
    {
        this->Destroy();

        this->vertexShader = other.vertexShader;
        this->fragmentShader = other.fragmentShader;
        this->inputAttributes = std::move(other.inputAttributes);
        this->shaderUniforms = std::move(other.shaderUniforms);

        other.vertexShader = vk::ShaderModule{ };
        other.fragmentShader = vk::ShaderModule{ };

        if ((bool)other.tessControlShader)
        {
            this->tessControlShader = other.tessControlShader;
            this->tessEvalShader = other.tessEvalShader;
            other.tessControlShader = VkShaderModule{};
            other.tessEvalShader = VkShaderModule{};
        }

        return *this;
    }

    ArrayView<const TypeSPIRV> GraphicShader::GetInputAttributes() const
    {
        return this->inputAttributes;
    }

    ArrayView<const ShaderUniforms> GraphicShader::GetShaderUniforms() const
    {
        return this->shaderUniforms;
    }

    const vk::ShaderModule& GraphicShader::GetNativeShader(ShaderType type) const
    {
        switch (type)
        {
        case ShaderType::VERTEX:
            return this->vertexShader;
        case ShaderType::FRAGMENT:
            return this->fragmentShader;
        case ShaderType::TESS_CONTROL:
            return this->tessControlShader;
        case ShaderType::TESS_EVALUATION:
            return this->tessEvalShader;
        default:
            assert(false);
            return this->vertexShader;
        }
    }
}