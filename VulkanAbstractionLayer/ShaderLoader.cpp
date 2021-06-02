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

    VertexAttribute GetVertexAttributeByReflectedType(const SpvReflectInterfaceVariable* type)
    {
        int32_t byteSize = type->numeric.scalar.width / 8;
        int32_t componentCount = 1;
        if (type->numeric.vector.component_count > 0) byteSize *= type->numeric.vector.component_count;
        if (type->numeric.matrix.column_count > 0) componentCount = type->numeric.matrix.column_count;
        return VertexAttribute{ FromNativeFormat((vk::Format)type->format), componentCount, byteSize };
    }

    std::vector<uint32_t> ShaderLoader::LoadFromSource(const std::string& filepath, ShaderType type, ShaderLanguage language, uint32_t vulkanVersion)
    {
        return ShaderLoader::LoadFromSourceWithReflection(filepath, type, language, vulkanVersion).Data;
    }

    std::vector<uint32_t> ShaderLoader::LoadFromBinary(const std::string& filepath)
    {
        std::vector<uint32_t> result;
        std::ifstream file(filepath, std::ios_base::binary);
        auto binaryData = std::vector<char>(std::istreambuf_iterator(file), std::istreambuf_iterator<char>());
        result.resize(binaryData.size() / sizeof(uint32_t));
        std::copy((uint32_t*)binaryData.data(), (uint32_t*)(binaryData.data() + binaryData.size()), result.begin());
        return result;
    }

    ShaderLoader::LoadedShader ShaderLoader::LoadFromSourceWithReflection(const std::string& filepath, ShaderType type, ShaderLanguage language, uint32_t vulkanVersion)
    {
        std::ifstream file(filepath);
        std::string source{ std::istreambuf_iterator(file), std::istreambuf_iterator<char>() };
        const char* rawSource = source.c_str();

        constexpr static auto ResourceLimits = GetResourceLimits();

        glslang::TShader shader{ ShaderTypeTable[(size_t)type] };
        shader.setStrings(&rawSource, 1);
        shader.setEnvInput(ShaderLanguageTable[(size_t)language], ShaderTypeTable[(size_t)type], glslang::EShClient::EShClientVulkan, 460);
        shader.setEnvClient(glslang::EShClient::EShClientVulkan, (glslang::EShTargetClientVersion)vulkanVersion);
        shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_5);
        bool isParsed = shader.parse(&ResourceLimits, 100, false, EShMessages::EShMsgDefault);
        if (!isParsed) return LoadedShader{ };

        glslang::TProgram program;
        program.addShader(&shader);
        bool isLinked = program.link(EShMessages::EShMsgDefault);
        if (!isLinked) return LoadedShader{ };

        auto intermediate = program.getIntermediate(ShaderTypeTable[(size_t)type]);
        std::vector<uint32_t> bytecode;
        glslang::GlslangToSpv(*intermediate, bytecode);

        return ShaderLoader::LoadFromMemoryWithReflection(bytecode);
    }

    ShaderLoader::LoadedShader ShaderLoader::LoadFromBinaryWithReflection(const std::string& filepath)
    {
        auto bytecode = ShaderLoader::LoadFromBinary(filepath);
        return ShaderLoader::LoadFromMemoryWithReflection(bytecode);
    }

    ShaderLoader::LoadedShader ShaderLoader::LoadFromMemoryWithReflection(std::vector<uint32_t> bytecode)
    {
        LoadedShader result;
        result.Data = std::move(bytecode);

        SpvReflectResult spvResult;
        SpvReflectShaderModule reflectedShader;
        spvResult = spvReflectCreateShaderModule(result.Data.size() * sizeof(uint32_t), (const void*)result.Data.data(), &reflectedShader);
        assert(spvResult == SPV_REFLECT_RESULT_SUCCESS);

        uint32_t inputAttributeCount = 0;
        spvResult = spvReflectEnumerateInputVariables(&reflectedShader, &inputAttributeCount, NULL);
        assert(spvResult == SPV_REFLECT_RESULT_SUCCESS);
        std::vector< SpvReflectInterfaceVariable*> inputAttributes(inputAttributeCount);
        spvResult = spvReflectEnumerateInputVariables(&reflectedShader, &inputAttributeCount, inputAttributes.data());
        assert(spvResult == SPV_REFLECT_RESULT_SUCCESS);

        // sort in location order
        std::sort(inputAttributes.begin(), inputAttributes.end(), [](const auto& v1, const auto& v2) { return v1->location < v2->location; });
        for (const auto& inputAttribute : inputAttributes)
        {
            result.VertexAttributes.push_back(GetVertexAttributeByReflectedType(inputAttribute));
        }

        spvReflectDestroyShaderModule(&reflectedShader);

        return result;
    }
}