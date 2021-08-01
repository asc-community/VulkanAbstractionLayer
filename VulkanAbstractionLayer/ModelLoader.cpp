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

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

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
        return ImageData{ std::vector{ r, g, b, a }, 1, 1, 4, sizeof(uint8_t) };
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
}