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

#include "ShaderLoader.h"
#include "VectorMath.h"
#include "VulkanContext.h"

#include <ShaderLang.h>
#include <GlslangToSpv.h>
#include <spirv_reflect.h>

#include <fstream>

namespace VulkanAbstractionLayer
{
    EShLanguage ShaderTypeTable[] = {
        EShLangVertex,
        EShLangTessControl,
        EShLangTessEvaluation,
        EShLangGeometry,
        EShLangFragment,
        EShLangCompute,
        EShLangRayGen,
        EShLangIntersect,
        EShLangAnyHit,
        EShLangClosestHit,
        EShLangMiss,
        EShLangCallable,
        EShLangTaskNV,
        EShLangMeshNV,
    };

    glslang::EShSource ShaderLanguageTable[] = {
        glslang::EShSource::EShSourceGlsl,
        glslang::EShSource::EShSourceHlsl,
    };

    constexpr auto GetResourceLimits()
    {
        return TBuiltInResource {
            /* .MaxLights = */ 32,
            /* .MaxClipPlanes = */ 6,
            /* .MaxTextureUnits = */ 32,
            /* .MaxTextureCoords = */ 32,
            /* .MaxVertexAttribs = */ 64,
            /* .MaxVertexUniformComponents = */ 4096,
            /* .MaxVaryingFloats = */ 64,
            /* .MaxVertexTextureImageUnits = */ 32,
            /* .MaxCombinedTextureImageUnits = */ 80,
            /* .MaxTextureImageUnits = */ 32,
            /* .MaxFragmentUniformComponents = */ 4096,
            /* .MaxDrawBuffers = */ 32,
            /* .MaxVertexUniformVectors = */ 128,
            /* .MaxVaryingVectors = */ 8,
            /* .MaxFragmentUniformVectors = */ 16,
            /* .MaxVertexOutputVectors = */ 16,
            /* .MaxFragmentInputVectors = */ 15,
            /* .MinProgramTexelOffset = */ -8,
            /* .MaxProgramTexelOffset = */ 7,
            /* .MaxClipDistances = */ 8,
            /* .MaxComputeWorkGroupCountX = */ 65535,
            /* .MaxComputeWorkGroupCountY = */ 65535,
            /* .MaxComputeWorkGroupCountZ = */ 65535,
            /* .MaxComputeWorkGroupSizeX = */ 1024,
            /* .MaxComputeWorkGroupSizeY = */ 1024,
            /* .MaxComputeWorkGroupSizeZ = */ 64,
            /* .MaxComputeUniformComponents = */ 1024,
            /* .MaxComputeTextureImageUnits = */ 16,
            /* .MaxComputeImageUniforms = */ 8,
            /* .MaxComputeAtomicCounters = */ 8,
            /* .MaxComputeAtomicCounterBuffers = */ 1,
            /* .MaxVaryingComponents = */ 60,
            /* .MaxVertexOutputComponents = */ 64,
            /* .MaxGeometryInputComponents = */ 64,
            /* .MaxGeometryOutputComponents = */ 128,
            /* .MaxFragmentInputComponents = */ 128,
            /* .MaxImageUnits = */ 8,
            /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
            /* .MaxCombinedShaderOutputResources = */ 8,
            /* .MaxImageSamples = */ 0,
            /* .MaxVertexImageUniforms = */ 0,
            /* .MaxTessControlImageUniforms = */ 0,
            /* .MaxTessEvaluationImageUniforms = */ 0,
            /* .MaxGeometryImageUniforms = */ 0,
            /* .MaxFragmentImageUniforms = */ 8,
            /* .MaxCombinedImageUniforms = */ 8,
            /* .MaxGeometryTextureImageUnits = */ 16,
            /* .MaxGeometryOutputVertices = */ 256,
            /* .MaxGeometryTotalOutputComponents = */ 1024,
            /* .MaxGeometryUniformComponents = */ 1024,
            /* .MaxGeometryVaryingComponents = */ 64,
            /* .MaxTessControlInputComponents = */ 128,
            /* .MaxTessControlOutputComponents = */ 128,
            /* .MaxTessControlTextureImageUnits = */ 16,
            /* .MaxTessControlUniformComponents = */ 1024,
            /* .MaxTessControlTotalOutputComponents = */ 4096,
            /* .MaxTessEvaluationInputComponents = */ 128,
            /* .MaxTessEvaluationOutputComponents = */ 128,
            /* .MaxTessEvaluationTextureImageUnits = */ 16,
            /* .MaxTessEvaluationUniformComponents = */ 1024,
            /* .MaxTessPatchComponents = */ 120,
            /* .MaxPatchVertices = */ 32,
            /* .MaxTessGenLevel = */ 64,
            /* .MaxViewports = */ 16,
            /* .MaxVertexAtomicCounters = */ 0,
            /* .MaxTessControlAtomicCounters = */ 0,
            /* .MaxTessEvaluationAtomicCounters = */ 0,
            /* .MaxGeometryAtomicCounters = */ 0,
            /* .MaxFragmentAtomicCounters = */ 8,
            /* .MaxCombinedAtomicCounters = */ 8,
            /* .MaxAtomicCounterBindings = */ 1,
            /* .MaxVertexAtomicCounterBuffers = */ 0,
            /* .MaxTessControlAtomicCounterBuffers = */ 0,
            /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
            /* .MaxGeometryAtomicCounterBuffers = */ 0,
            /* .MaxFragmentAtomicCounterBuffers = */ 1,
            /* .MaxCombinedAtomicCounterBuffers = */ 1,
            /* .MaxAtomicCounterBufferSize = */ 16384,
            /* .MaxTransformFeedbackBuffers = */ 4,
            /* .MaxTransformFeedbackInterleavedComponents = */ 64,
            /* .MaxCullDistances = */ 8,
            /* .MaxCombinedClipAndCullDistances = */ 8,
            /* .MaxSamples = */ 4,
            /* .maxMeshOutputVerticesNV = */ 256,
            /* .maxMeshOutputPrimitivesNV = */ 512,
            /* .maxMeshWorkGroupSizeX_NV = */ 32,
            /* .maxMeshWorkGroupSizeY_NV = */ 1,
            /* .maxMeshWorkGroupSizeZ_NV = */ 1,
            /* .maxTaskWorkGroupSizeX_NV = */ 32,
            /* .maxTaskWorkGroupSizeY_NV = */ 1,
            /* .maxTaskWorkGroupSizeZ_NV = */ 1,
            /* .maxMeshViewCountNV = */ 4,
            /* .maxDualSourceDrawBuffersEXT = */ 1,

            /* .limits = */ {
                /* .nonInductiveForLoops = */ 1,
                /* .whileLoops = */ 1,
                /* .doWhileLoops = */ 1,
                /* .generalUniformIndexing = */ 1,
                /* .generalAttributeMatrixVectorIndexing = */ 1,
                /* .generalVaryingIndexing = */ 1,
                /* .generalSamplerIndexing = */ 1,
                /* .generalVariableIndexing = */ 1,
                /* .generalConstantMatrixVectorIndexing = */ 1,
            } 
        };
    }

    TypeSPIRV GetTypeByReflection(const SpvReflectTypeDescription& type)
    {
        Format format = Format::UNDEFINED;
        if (type.type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT)
        {
            if (type.traits.numeric.vector.component_count <  2 || type.traits.numeric.matrix.column_count <  2)
                format = Format::R32_SFLOAT;
            if (type.traits.numeric.vector.component_count == 2 || type.traits.numeric.matrix.column_count == 2)
                format = Format::R32G32_SFLOAT;
            if (type.traits.numeric.vector.component_count == 3 || type.traits.numeric.matrix.column_count == 3)
                format = Format::R32G32B32_SFLOAT;
            if (type.traits.numeric.vector.component_count == 4 || type.traits.numeric.matrix.column_count == 4)
                format = Format::R32G32B32A32_SFLOAT;
        }
        if (type.type_flags & SPV_REFLECT_TYPE_FLAG_INT)
        {
            if (type.traits.numeric.vector.component_count <  2 || type.traits.numeric.matrix.column_count <  2)
                format = type.traits.numeric.scalar.signedness ? Format::R32_SINT : Format::R32_UINT;
            if (type.traits.numeric.vector.component_count == 2 || type.traits.numeric.matrix.column_count == 2)
                format = type.traits.numeric.scalar.signedness ? Format::R32G32_SINT : Format::R32G32_UINT;
            if (type.traits.numeric.vector.component_count == 3 || type.traits.numeric.matrix.column_count == 3)
                format = type.traits.numeric.scalar.signedness ? Format::R32G32B32_SINT : Format::R32G32B32_UINT;
            if (type.traits.numeric.vector.component_count == 4 || type.traits.numeric.matrix.column_count == 4)
                format = type.traits.numeric.scalar.signedness ? Format::R32G32B32A32_SINT : Format::R32G32B32A32_UINT;
        }
        assert(format != Format::UNDEFINED);

        int32_t byteSize = type.traits.numeric.scalar.width / 8;
        int32_t componentCount = 1;

        if (type.traits.numeric.vector.component_count > 0)
            byteSize *= type.traits.numeric.vector.component_count;
        else if (type.traits.numeric.matrix.row_count > 0)
            byteSize *= type.traits.numeric.matrix.row_count;

        if (type.traits.numeric.matrix.column_count > 0)
            componentCount = type.traits.numeric.matrix.column_count;

        return TypeSPIRV{ format, componentCount, byteSize };
    }

    void RecursiveUniformVisit(std::vector<TypeSPIRV>& uniformVariables, const SpvReflectTypeDescription& type)
    {
        if (type.member_count > 0)
        {
            for (uint32_t i = 0; i < type.member_count; i++)
                RecursiveUniformVisit(uniformVariables, type.members[i]);
        }
        else
        {
            if(type.type_flags & (SPV_REFLECT_TYPE_FLAG_INT | SPV_REFLECT_TYPE_FLAG_FLOAT))
                uniformVariables.push_back(GetTypeByReflection(type));
        }
    }

    ShaderData ShaderLoader::LoadFromBinaryFile(const std::string& filepath)
    {
        std::vector<uint32_t> bytecode;
        std::ifstream file(filepath, std::ios_base::binary);
        auto binaryData = std::vector<char>(std::istreambuf_iterator(file), std::istreambuf_iterator<char>());
        bytecode.resize(binaryData.size() / sizeof(uint32_t));
        std::copy((uint32_t*)binaryData.data(), (uint32_t*)(binaryData.data() + binaryData.size()), bytecode.begin());
        return ShaderLoader::LoadFromBinary(std::move(bytecode));
    }

    ShaderData ShaderLoader::LoadFromSourceFile(const std::string& filepath, ShaderType type, ShaderLanguage language)
    {
        std::ifstream file(filepath);
        std::string source{ std::istreambuf_iterator(file), std::istreambuf_iterator<char>() };
        return ShaderLoader::LoadFromSource(source, type, language);
    }

    ShaderData ShaderLoader::LoadFromSource(const std::string& code, ShaderType type, ShaderLanguage language)
    {
        const char* rawSource = code.c_str();
        constexpr static auto ResourceLimits = GetResourceLimits();

        glslang::TShader shader{ ShaderTypeTable[(size_t)type] };
        shader.setStrings(&rawSource, 1);
        shader.setEnvInput(ShaderLanguageTable[(size_t)language], ShaderTypeTable[(size_t)type], glslang::EShClient::EShClientVulkan, 460);
        shader.setEnvClient(glslang::EShClient::EShClientVulkan, (glslang::EShTargetClientVersion)GetCurrentVulkanContext().GetAPIVersion());
        shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_5);
        bool isParsed = shader.parse(&ResourceLimits, 460, false, EShMessages::EShMsgDefault);
        if (!isParsed) return ShaderData{ };

        glslang::TProgram program;
        program.addShader(&shader);
        bool isLinked = program.link(EShMessages::EShMsgDefault);
        if (!isLinked) return ShaderData{ };

        auto intermediate = program.getIntermediate(ShaderTypeTable[(size_t)type]);
        std::vector<uint32_t> bytecode;
        glslang::GlslangToSpv(*intermediate, bytecode);

        return ShaderLoader::LoadFromBinary(std::move(bytecode));
    }

    ShaderData ShaderLoader::LoadFromBinary(std::vector<uint32_t> bytecode)
    {
        ShaderData result;
        result.Bytecode = std::move(bytecode);

        SpvReflectResult spvResult;
        SpvReflectShaderModule reflectedShader;
        spvResult = spvReflectCreateShaderModule(result.Bytecode.size() * sizeof(uint32_t), (const void*)result.Bytecode.data(), &reflectedShader);
        assert(spvResult == SPV_REFLECT_RESULT_SUCCESS);

        uint32_t inputAttributeCount = 0;
        spvResult = spvReflectEnumerateInputVariables(&reflectedShader, &inputAttributeCount, nullptr);
        assert(spvResult == SPV_REFLECT_RESULT_SUCCESS);
        std::vector<SpvReflectInterfaceVariable*> inputAttributes(inputAttributeCount);
        spvResult = spvReflectEnumerateInputVariables(&reflectedShader, &inputAttributeCount, inputAttributes.data());
        assert(spvResult == SPV_REFLECT_RESULT_SUCCESS);

        // sort in location order
        std::sort(inputAttributes.begin(), inputAttributes.end(), [](const auto& v1, const auto& v2) { return v1->location < v2->location; });
        for (const auto& inputAttribute : inputAttributes)
        {
            if (inputAttribute->built_in == (SpvBuiltIn)-1) // ignore build-ins
            {
                result.InputAttributes.push_back(GetTypeByReflection(*inputAttribute->type_description));
            }
        }

        uint32_t descriptorBindingCount = 0;
        spvResult = spvReflectEnumerateDescriptorBindings(&reflectedShader, &descriptorBindingCount, nullptr);
        assert(spvResult == SPV_REFLECT_RESULT_SUCCESS);
        std::vector<SpvReflectDescriptorBinding*> descriptorBindings(descriptorBindingCount);
        spvResult = spvReflectEnumerateDescriptorBindings(&reflectedShader, &descriptorBindingCount, descriptorBindings.data());
        assert(spvResult == SPV_REFLECT_RESULT_SUCCESS);

        for (const auto& descriptorBinding : descriptorBindings)
        {
            if (result.DescriptorSets.size() < (size_t)descriptorBinding->set + 1)
                result.DescriptorSets.resize((size_t)descriptorBinding->set + 1);

            auto& uniformBlock = result.DescriptorSets[descriptorBinding->set];

            std::vector<TypeSPIRV> uniformVariables;
            RecursiveUniformVisit(uniformVariables, *descriptorBinding->type_description);

            uniformBlock.push_back(Uniform { 
                std::move(uniformVariables),
                FromNative((vk::DescriptorType)descriptorBinding->descriptor_type),
                descriptorBinding->binding,
                descriptorBinding->count
            });
        }
        if (result.DescriptorSets.empty()) 
            result.DescriptorSets.emplace_back(); // insert empty descriptor set

        spvReflectDestroyShaderModule(&reflectedShader);

        return result;
    }
}