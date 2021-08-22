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

#include "ComputeShader.h"
#include "VulkanContext.h"

namespace VulkanAbstractionLayer
{
    ArrayView<const TypeSPIRV> ComputeShader::GetInputAttributes() const
    {
        return { };
    }

    ArrayView<const ShaderUniforms> ComputeShader::GetShaderUniforms() const
    {
        return this->shaderUniforms;
    }

    const vk::ShaderModule& ComputeShader::GetNativeShader(ShaderType type) const
    {
        assert(type == ShaderType::COMPUTE);
        return this->computeShader;
    }

    void ComputeShader::Destroy()
    {
        auto& vulkan = GetCurrentVulkanContext();
        auto& device = vulkan.GetDevice();
        if ((bool)this->computeShader) device.destroyShaderModule(this->computeShader);

        this->computeShader = vk::ShaderModule{ };
    }

    ComputeShader::ComputeShader(const ShaderData& computeData)
    {
        this->Init(computeData);
    }

    ComputeShader::~ComputeShader()
    {
        this->Destroy();
    }

    void ComputeShader::Init(const ShaderData& computeData)
    {
        auto& vulkan = GetCurrentVulkanContext();

        vk::ShaderModuleCreateInfo computeShaderInfo;
        computeShaderInfo.setCode(computeData.Bytecode);
        this->computeShader = vulkan.GetDevice().createShaderModule(computeShaderInfo);

        // TODO: support multiple descriptor sets
        assert(computeData.DescriptorSets.size() < 2);
        this->shaderUniforms = std::vector{
            ShaderUniforms{ computeData.DescriptorSets[0], ShaderType::COMPUTE }
        };
    }

    ComputeShader::ComputeShader(ComputeShader&& other) noexcept
    {
        this->computeShader = other.computeShader;
        this->shaderUniforms = std::move(other.shaderUniforms);

        other.computeShader = vk::ShaderModule{ };
    }

    ComputeShader& ComputeShader::operator=(ComputeShader&& other) noexcept
    {
        this->Destroy();

        this->computeShader = other.computeShader;
        this->shaderUniforms = std::move(other.shaderUniforms);

        other.computeShader = vk::ShaderModule{ };

        return *this;
    }
}