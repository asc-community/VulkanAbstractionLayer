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
#include "VulkanMemoryAllocator.h"

namespace VulkanAbstractionLayer
{
    template<>
    VertexAttribute VertexAttribute::OfType<float>()
    {
        return { VertexAttribute::Type::FLOAT, sizeof(float) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<Vector2>()
    {
        return { VertexAttribute::Type::FLOAT_VEC2, sizeof(Vector2) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<Vector3>()
    {
        return { VertexAttribute::Type::FLOAT_VEC3, sizeof(Vector3) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<Vector4>()
    {
        return { VertexAttribute::Type::FLOAT_VEC4, sizeof(Vector4) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<int32_t>()
    {
        return { VertexAttribute::Type::INT, sizeof(int32_t) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<VectorInt2>()
    {
        return { VertexAttribute::Type::INT_VEC2, sizeof(VectorInt2) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<VectorInt3>()
    {
        return { VertexAttribute::Type::INT_VEC3, sizeof(VectorInt3) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<VectorInt4>()
    {
        return { VertexAttribute::Type::INT_VEC4, sizeof(VectorInt4) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<Matrix2x2>()
    {
        return { VertexAttribute::Type::MAT2, sizeof(Matrix2x2) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<Matrix3x3>()
    {
        return { VertexAttribute::Type::MAT3, sizeof(Matrix3x3) };
    }

    template<>
    VertexAttribute VertexAttribute::OfType<Matrix4x4>()
    {
        return { VertexAttribute::Type::MAT4, sizeof(Matrix4x4) };
    }

    void GraphicShader::Destroy()
    {
        if((bool)this->vertexShader) this->device.destroyShaderModule(this->vertexShader);
        if ((bool)this->fragmentShader) this->device.destroyShaderModule(this->fragmentShader);

        this->vertexShader = vk::ShaderModule{ };
        this->fragmentShader = vk::ShaderModule{ };
    }

    GraphicShader::GraphicShader(const VulkanContext& context)
        : device(ContextGetDevice(context))
    {
    }

    GraphicShader::GraphicShader(std::vector<uint32_t> vertexBytecode, std::vector<uint32_t> fragmentBytecode, std::vector<VertexAttribute> vertexAttributes, vk::DescriptorSetLayout descriptorSetLayout, const VulkanContext& context)
        : GraphicShader(context)
    {
        this->Init(std::move(vertexBytecode), std::move(fragmentBytecode), std::move(vertexAttributes), std::move(descriptorSetLayout));
    }

    void GraphicShader::Init(std::vector<uint32_t> vertexBytecode, std::vector<uint32_t> fragmentBytecode, std::vector<VertexAttribute> vertexAttributes, vk::DescriptorSetLayout descriptorSetLayout)
    {
        vk::ShaderModuleCreateInfo vertexShaderInfo;
        vertexShaderInfo.setCode(vertexBytecode);
        this->vertexShader = this->device.createShaderModule(vertexShaderInfo);

        vk::ShaderModuleCreateInfo fragmentShaderInfo;
        fragmentShaderInfo.setCode(fragmentBytecode);
        this->fragmentShader = this->device.createShaderModule(fragmentShaderInfo);

        this->vertexAttributes = std::move(vertexAttributes);
        this->descriptorSetLayout = std::move(descriptorSetLayout);
    }

    GraphicShader::GraphicShader(GraphicShader&& other) noexcept
    {
        this->device = other.device;
        this->vertexShader = other.vertexShader;
        this->fragmentShader = other.fragmentShader;
        this->vertexAttributes = std::move(other.vertexAttributes);
        this->descriptorSetLayout = other.descriptorSetLayout;

        other.vertexShader = vk::ShaderModule{ };
        other.fragmentShader = vk::ShaderModule{ };
    }

    GraphicShader& GraphicShader::operator=(GraphicShader&& other) noexcept
    {
        this->Destroy();

        this->device = other.device;
        this->vertexShader = other.vertexShader;
        this->fragmentShader = other.fragmentShader;
        this->vertexAttributes = std::move(other.vertexAttributes);
        this->descriptorSetLayout = other.descriptorSetLayout;

        other.vertexShader = vk::ShaderModule{ };
        other.fragmentShader = vk::ShaderModule{ };

        return *this;
    }

    GraphicShader::~GraphicShader()
    {
        this->Destroy();
    }
}