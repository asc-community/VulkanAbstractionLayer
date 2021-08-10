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

#include "ModelLoader.h"
#include "ArrayUtils.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"

#include <filesystem>

namespace VulkanAbstractionLayer
{
    static std::string GetAbsolutePathToObjResource(const std::string& objPath, const std::string& relativePath)
    {
        const auto parentPath = std::filesystem::path{ objPath }.parent_path();
        const auto absolutePath = parentPath / relativePath;
        return absolutePath.string();
    }

    static ImageData CreateStubTexture(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        return ImageData{ std::vector{ r, g, b, a }, Format::R8G8B8A8_UNORM, 1, 1 };
    }

    static auto ComputeTangentsBitangents(const tinyobj::mesh_t& mesh, const tinyobj::attrib_t& attrib)
    {
        std::vector<std::pair<Vector3, Vector3>> tangentsBitangents;
        tangentsBitangents.resize(attrib.normals.size(), { Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 0.0f, 0.0f } });

        size_t indexOffset = 0;
        for (size_t faceIndex : mesh.num_face_vertices)
        {
            assert(faceIndex == 3);

            tinyobj::index_t idx1 = mesh.indices[indexOffset + 0];
            tinyobj::index_t idx2 = mesh.indices[indexOffset + 1];
            tinyobj::index_t idx3 = mesh.indices[indexOffset + 2];

            Vector3 position1, position2, position3;
            Vector2 texCoord1, texCoord2, texCoord3;

            position1.x = attrib.vertices[3 * size_t(idx1.vertex_index) + 0];
            position1.y = attrib.vertices[3 * size_t(idx1.vertex_index) + 1];
            position1.z = attrib.vertices[3 * size_t(idx1.vertex_index) + 2];

            position2.x = attrib.vertices[3 * size_t(idx2.vertex_index) + 0];
            position2.y = attrib.vertices[3 * size_t(idx2.vertex_index) + 1];
            position2.z = attrib.vertices[3 * size_t(idx2.vertex_index) + 2];

            position3.x = attrib.vertices[3 * size_t(idx3.vertex_index) + 0];
            position3.y = attrib.vertices[3 * size_t(idx3.vertex_index) + 1];
            position3.z = attrib.vertices[3 * size_t(idx3.vertex_index) + 2];

            texCoord1.x = attrib.texcoords[2 * size_t(idx1.texcoord_index) + 0];
            texCoord1.y = attrib.texcoords[2 * size_t(idx1.texcoord_index) + 1];

            texCoord2.x = attrib.texcoords[2 * size_t(idx2.texcoord_index) + 0];
            texCoord2.y = attrib.texcoords[2 * size_t(idx2.texcoord_index) + 1];

            texCoord3.x = attrib.texcoords[2 * size_t(idx3.texcoord_index) + 0];
            texCoord3.y = attrib.texcoords[2 * size_t(idx3.texcoord_index) + 1];

            auto tangentBitangent = ComputeTangentSpace(
                position1, position2, position3,
                texCoord1, texCoord2, texCoord3
            );

            tangentsBitangents[idx1.vertex_index].first  += tangentBitangent.first;
            tangentsBitangents[idx1.vertex_index].second += tangentBitangent.second;

            tangentsBitangents[idx2.vertex_index].first  += tangentBitangent.first;
            tangentsBitangents[idx2.vertex_index].second += tangentBitangent.second;

            tangentsBitangents[idx3.vertex_index].first  += tangentBitangent.first;
            tangentsBitangents[idx3.vertex_index].second += tangentBitangent.second;

            indexOffset += faceIndex;
        }

        for (auto& [tangent, bitangent] : tangentsBitangents)
        {
            if(Dot(tangent, tangent)     > 0.0f) tangent   = Normalize(tangent);
            if(Dot(bitangent, bitangent) > 0.0f) bitangent = Normalize(bitangent);
        }
        return tangentsBitangents;
    }

    static auto ComputeTangentsBitangents(ArrayView<const uint16_t> indicies, ArrayView<const Vector3> positions, ArrayView<const Vector2> texCoords)
    {
        std::vector<std::pair<Vector3, Vector3>> tangentsBitangents;
        tangentsBitangents.resize(positions.size(), { Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 0.0f, 0.0f } });

        assert(indicies.size() % 3 == 0);
        for (size_t i = 0; i < indicies.size(); i += 3)
        {
            const auto& position1 = positions[indicies[i + 0]];
            const auto& position2 = positions[indicies[i + 1]];
            const auto& position3 = positions[indicies[i + 2]];

            const auto& texCoord1 = texCoords[indicies[i + 0]];
            const auto& texCoord2 = texCoords[indicies[i + 1]];
            const auto& texCoord3 = texCoords[indicies[i + 2]];

            auto tangentBitangent = ComputeTangentSpace(
                position1, position2, position3,
                texCoord1, texCoord2, texCoord3
            );

            tangentsBitangents[indicies[i + 0]].first += tangentBitangent.first;
            tangentsBitangents[indicies[i + 0]].second += tangentBitangent.second;

            tangentsBitangents[indicies[i + 1]].first += tangentBitangent.first;
            tangentsBitangents[indicies[i + 1]].second += tangentBitangent.second;

            tangentsBitangents[indicies[i + 2]].first += tangentBitangent.first;
            tangentsBitangents[indicies[i + 2]].second += tangentBitangent.second;
        }

        for (auto& [tangent, bitangent] : tangentsBitangents)
        {
            if (Dot(tangent, tangent) > 0.0f) tangent = Normalize(tangent);
            if (Dot(bitangent, bitangent) > 0.0f) bitangent = Normalize(bitangent);
        }
        return tangentsBitangents;
    }

    ModelData ModelLoader::LoadFromObj(const std::string& filepath)
    {
        ModelData result;

        tinyobj::ObjReaderConfig reader_config;
        tinyobj::ObjReader reader;

        if (!reader.ParseFromFile(filepath, reader_config))
        {
            return result;
        }

        auto& attrib = reader.GetAttrib();
        auto& shapes = reader.GetShapes();
        auto& materials = reader.GetMaterials();

        result.Materials.reserve(materials.size());
        for (const auto& material : materials)
        {
            auto& resultMaterial = result.Materials.emplace_back();

            resultMaterial.Name = material.name;

            if (!material.diffuse_texname.empty())
                resultMaterial.AlbedoTexture = ImageLoader::LoadFromFile(GetAbsolutePathToObjResource(filepath, material.diffuse_texname));
            else
                resultMaterial.AlbedoTexture = CreateStubTexture(255, 255, 255, 255);

            if (!material.normal_texname.empty())
                resultMaterial.NormalTexture = ImageLoader::LoadFromFile(GetAbsolutePathToObjResource(filepath, material.normal_texname));
            else
                resultMaterial.NormalTexture = CreateStubTexture(127, 127, 255, 255);

            // not supported by obj format
            resultMaterial.MetallicRoughness = CreateStubTexture(0, 255, 0, 255);
            resultMaterial.RoughnessScale = 1.0f;
        }

        result.Shapes.reserve(shapes.size());
        for (const auto& shape : shapes)
        {
            auto& resultShape = result.Shapes.emplace_back();

            resultShape.Name = shape.name;

            if (!shape.mesh.material_ids.empty())
                resultShape.MaterialIndex = shape.mesh.material_ids.front();

            std::vector<std::pair<Vector3, Vector3>> tangentsBitangents;
            if (!attrib.normals.empty() && !attrib.texcoords.empty())
                tangentsBitangents = ComputeTangentsBitangents(shape.mesh, attrib);

            size_t index_offset = 0;
            for (size_t faceIndex : shape.mesh.num_face_vertices)
            {
                assert(faceIndex == 3);
                for (size_t v = 0; v < faceIndex; v++)
                {
                    tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                    auto& vertex = resultShape.Vertices.emplace_back();

                    vertex.Position.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                    vertex.Position.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                    vertex.Position.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

                    vertex.TexCoord.x = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    vertex.TexCoord.y = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];

                    vertex.Normal.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    vertex.Normal.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    vertex.Normal.z = attrib.normals[3 * size_t(idx.normal_index) + 2];

                    if (!tangentsBitangents.empty())
                    {
                        vertex.Tangent   = tangentsBitangents[idx.vertex_index].first;
                        vertex.Bitangent = tangentsBitangents[idx.vertex_index].second;
                    }
                }
                index_offset += faceIndex;
            }
        }

        return result;
    }

    ModelData ModelLoader::LoadFromGltf(const std::string& filepath)
    {
        ModelData result;

        tinygltf::TinyGLTF loader;
        tinygltf::Model model;
        std::string errorMessage, warningMessage;

        bool res = loader.LoadASCIIFromFile(&model, &errorMessage, &warningMessage, filepath);
        if (!res) return result;

        result.Materials.reserve(model.materials.size());
        for (const auto& material : model.materials)
        {
            auto& resultMaterial = result.Materials.emplace_back();
            
            resultMaterial.Name = material.name;
            resultMaterial.RoughnessScale = material.pbrMetallicRoughness.roughnessFactor;

            if (material.pbrMetallicRoughness.baseColorTexture.index != -1)
            {
                const auto& albedoTexture = model.images[model.textures[material.pbrMetallicRoughness.baseColorTexture.index].source];
                resultMaterial.AlbedoTexture = ImageData{ 
                    albedoTexture.image, 
                    Format::R8G8B8A8_UNORM,
                    (uint32_t)albedoTexture.width, 
                    (uint32_t)albedoTexture.height, 
                };
            }
            else
            {
                resultMaterial.AlbedoTexture = CreateStubTexture(255, 255, 255, 255);
            }

            if (material.normalTexture.index != -1)
            {
                const auto& normalTexture = model.images[model.textures[material.normalTexture.index].source];
                resultMaterial.NormalTexture = ImageData{ 
                    normalTexture.image, 
                    Format::R8G8B8A8_UNORM,
                    (uint32_t)normalTexture.width, 
                    (uint32_t)normalTexture.height, 
                };
            }
            else
            {
                resultMaterial.NormalTexture = CreateStubTexture(127, 127, 255, 255);
            }

            if (material.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
            {
                const auto& metallicRoughnessTexture = model.images[model.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index].source];
                resultMaterial.MetallicRoughness = ImageData{
                    metallicRoughnessTexture.image,
                    Format::R8G8B8A8_UNORM,
                    (uint32_t)metallicRoughnessTexture.width,
                    (uint32_t)metallicRoughnessTexture.height,
                };
            }
            else
            {
                resultMaterial.MetallicRoughness = CreateStubTexture(0, 255, 0, 255);
            }
        }

        for (const auto& mesh : model.meshes)
        {
            result.Shapes.reserve(result.Shapes.size() + mesh.primitives.size());
            for (const auto& primitive : mesh.primitives)
            {
                auto& resultShape = result.Shapes.emplace_back();
                resultShape.Name = "shape_" + std::to_string(result.Shapes.size());
                resultShape.MaterialIndex = (uint32_t)primitive.material;

                const auto& indexAccessor = model.accessors[primitive.indices];
                auto& indexBuffer = model.bufferViews[indexAccessor.bufferView];
                const uint8_t* indexBegin = model.buffers[indexBuffer.buffer].data.data() + indexBuffer.byteOffset;
                ArrayView<const uint16_t> indicies((const uint16_t*)indexBegin, (const uint16_t*)(indexBegin + indexBuffer.byteLength));

                const auto& positionAccessor = model.accessors[primitive.attributes.at("POSITION")];
                const auto& texCoordAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                const auto& normalAccessor = model.accessors[primitive.attributes.at("NORMAL")];

                const auto& positionBuffer = model.bufferViews[positionAccessor.bufferView];
                const auto& texCoordBuffer = model.bufferViews[texCoordAccessor.bufferView];
                const auto& normalBuffer = model.bufferViews[normalAccessor.bufferView];

                const uint8_t* positionBegin = model.buffers[positionBuffer.buffer].data.data() + positionBuffer.byteOffset;
                const uint8_t* texCoordBegin = model.buffers[texCoordBuffer.buffer].data.data() + texCoordBuffer.byteOffset;
                const uint8_t* normalBegin = model.buffers[normalBuffer.buffer].data.data() + normalBuffer.byteOffset;

                ArrayView<const Vector3> positions((const Vector3*)positionBegin, (const Vector3*)(positionBegin + positionBuffer.byteLength));
                ArrayView<const Vector2> texCoords((const Vector2*)texCoordBegin, (const Vector2*)(texCoordBegin + texCoordBuffer.byteLength));
                ArrayView<const Vector3> normals((const Vector3*)normalBegin, (const Vector3*)(normalBegin + normalBuffer.byteLength));

                auto tangentsBitangents = ComputeTangentsBitangents(indicies, positions, texCoords);

                resultShape.Vertices.reserve(indicies.size());
                for (auto& index : indicies)
                {
                    auto& vertex = resultShape.Vertices.emplace_back();
                    vertex.Position = positions[index];
                    vertex.TexCoord = texCoords[index];
                    vertex.Normal = normals[index];
                    vertex.Tangent = tangentsBitangents[index].first;
                    vertex.Bitangent = tangentsBitangents[index].second;
                }
            }
        }

        return result;
    }
}