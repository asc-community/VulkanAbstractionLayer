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

#include "ShaderLoader.h"
#include "DescriptorCache.h"

namespace VulkanAbstractionLayer
{
    class VulkanContext;

    class GraphicShader
    {
        vk::ShaderModule vertexShader;
        vk::ShaderModule fragmentShader;

        std::vector<TypeSPIRV> vertexAttributes;
        vk::DescriptorSetLayout descriptorSetLayout;

        void Destroy();
    public:
        GraphicShader() = default;
        GraphicShader(const ShaderData& vertex, const ShaderData& fragment);

        void Init(const ShaderData& vertex, const ShaderData& fragment);

        GraphicShader(const GraphicShader& other) = delete;
        GraphicShader(GraphicShader&& other) noexcept;
        GraphicShader& operator=(const GraphicShader& other) = delete;
        GraphicShader& operator=(GraphicShader&& other) noexcept;
        ~GraphicShader();

        const auto& GetVertexAttributes() const { return this->vertexAttributes; }
        const auto& GetDescriptorSetLayout() const { return this->descriptorSetLayout; }

        const vk::ShaderModule& GetNativeShader(ShaderType type) const
        {
            switch (type)
            {
            case ShaderType::VERTEX:
                return this->vertexShader;
            case ShaderType::FRAGMENT:
                return this->fragmentShader;
            default:
                assert(false);
                return *(vk::ShaderModule*)nullptr;
            }
        }
    };
}