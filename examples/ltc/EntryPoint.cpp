#include <filesystem>
#include <iostream>
#include <map>

#include "VulkanAbstractionLayer/Window.h"
#include "VulkanAbstractionLayer/VulkanContext.h"
#include "VulkanAbstractionLayer/ImGuiContext.h"
#include "VulkanAbstractionLayer/RenderGraphBuilder.h"
#include "VulkanAbstractionLayer/ShaderLoader.h"
#include "VulkanAbstractionLayer/ModelLoader.h"
#include "VulkanAbstractionLayer/ImageLoader.h"
#include "VulkanAbstractionLayer/ImGuiRenderPass.h"
#include "VulkanAbstractionLayer/GraphicShader.h"

using namespace VulkanAbstractionLayer;

void VulkanInfoCallback(const std::string& message)
{
    std::cout << "[INFO Vulkan]: " << message << std::endl;
}

void VulkanErrorCallback(const std::string& message)
{
    std::cout << "[ERROR Vulkan]: " << message << std::endl;
}

void WindowErrorCallback(const std::string& message)
{
    std::cerr << "[ERROR Window]: " << message << std::endl;
}

constexpr size_t MaxLightCount = 4;
constexpr size_t MaxMaterialCount = 256;

struct Mesh
{
    struct Material
    {
        uint32_t AlbedoIndex;
        uint32_t NormalIndex;
        uint32_t MetallicRoughnessIndex;
        float RoughnessScale;
    };

    struct Submesh
    {
        Buffer VertexBuffer;
        uint32_t MaterialIndex;
    };

    std::vector<Submesh> Submeshes;
    std::vector<Material> Materials;
    std::vector<Image> Textures;
};

struct CameraUniformData
{
    Matrix4x4 Matrix;
    Vector3 Position;
};

struct ModelUniformData
{
    Matrix3x4 Matrix;
};

struct LightUniformData
{
    Matrix3x4 Rotation;
    Vector3 Position;
    float Width;
    Vector3 Color;
    float Height;
    uint32_t TextureIndex;
    uint32_t Padding[3];
};

struct SharedResources
{
    Buffer CameraUniformBuffer;
    Buffer ModelUniformBuffer;
    Buffer MaterialUniformBuffer;
    Buffer LightUniformBuffer;
    Mesh Sponza;
    Image LookupLTCMatrix;
    Image LookupLTCAmplitude;
    std::vector<Image> LightTextures;
    CameraUniformData CameraUniform;
    ModelUniformData ModelUniform;
    std::array<LightUniformData, MaxLightCount> LightUniformArray;
};

void LoadImage(CommandBuffer& commandBuffer, Image& image, const ImageData& imageData, ImageOptions::Value options)
{
    auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();

    image.Init(
        imageData.Width,
        imageData.Height,
        imageData.ImageFormat,
        ImageUsage::SHADER_READ | ImageUsage::TRANSFER_SOURCE | ImageUsage::TRANSFER_DISTINATION,
        MemoryUsage::GPU_ONLY,
        options
    );
    auto allocation = stageBuffer.Submit(MakeView(imageData.ByteData));
    commandBuffer.CopyBufferToImage(
        BufferInfo{ stageBuffer.GetBuffer(), allocation.Offset }, 
        ImageInfo{ image, ImageUsage::UNKNOWN, 0, 0 }
    );
    if (options & ImageOptions::MIPMAPS)
    {
        if (imageData.MipLevels.empty())
        {
            commandBuffer.GenerateMipLevels(image, ImageUsage::TRANSFER_DISTINATION, BlitFilter::LINEAR);
        }
        else
        {
            uint32_t mipLevel = 1;
            for (const auto& mipData : imageData.MipLevels)
            {
                auto allocation = stageBuffer.Submit(MakeView(mipData));
                commandBuffer.CopyBufferToImage(
                    BufferInfo{ stageBuffer.GetBuffer(), allocation.Offset },
                    ImageInfo{ image, ImageUsage::TRANSFER_DISTINATION, mipLevel, 0 }
                );
                mipLevel++;
            }
        }
    }
}

void LoadImage(Image& image, const std::string& filepath, ImageOptions::Value options)
{
    auto& commandBuffer = GetCurrentVulkanContext().GetCurrentCommandBuffer();
    auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();
    commandBuffer.Begin();

    LoadImage(commandBuffer, image, ImageLoader::LoadImageFromFile(filepath), options);

    stageBuffer.Flush();
    commandBuffer.End();
    GetCurrentVulkanContext().SubmitCommandsImmediate(commandBuffer);
    stageBuffer.Reset();
}

void LoadModelGLTF(Mesh& mesh, const std::string& filepath)
{
    auto model = ModelLoader::LoadFromGltf(filepath);
   
    auto& commandBuffer = GetCurrentVulkanContext().GetCurrentCommandBuffer();
    auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();

    commandBuffer.Begin();

    for (const auto& shape : model.Shapes)
    {
        auto& submesh = mesh.Submeshes.emplace_back();
        submesh.VertexBuffer.Init(
            shape.Vertices.size() * sizeof(ModelData::Vertex),
            BufferUsage::VERTEX_BUFFER | BufferUsage::TRANSFER_DESTINATION, 
            MemoryUsage::GPU_ONLY
        );

        auto allocation = stageBuffer.Submit(MakeView(shape.Vertices));
        commandBuffer.CopyBuffer(
            BufferInfo{ stageBuffer.GetBuffer(), allocation.Offset }, 
            BufferInfo{ submesh.VertexBuffer, 0 }, 
            allocation.Size
        );

        submesh.MaterialIndex = shape.MaterialIndex;
    }

    stageBuffer.Flush();
    commandBuffer.End();
    GetCurrentVulkanContext().SubmitCommandsImmediate(commandBuffer);
    stageBuffer.Reset();

    uint32_t textureIndex = 0;
    for (const auto& material : model.Materials)
    {
        commandBuffer.Begin();

        LoadImage(commandBuffer, mesh.Textures.emplace_back(), material.AlbedoTexture, ImageOptions::MIPMAPS);
        LoadImage(commandBuffer, mesh.Textures.emplace_back(), material.NormalTexture, ImageOptions::MIPMAPS);
        LoadImage(commandBuffer, mesh.Textures.emplace_back(), material.MetallicRoughness, ImageOptions::MIPMAPS);

        constexpr float AppliedRoughnessScale = 0.5f;
        mesh.Materials.push_back(Mesh::Material{ textureIndex, textureIndex + 1, textureIndex + 2, AppliedRoughnessScale * material.RoughnessScale });
        textureIndex += 3;

        stageBuffer.Flush();
        commandBuffer.End();
        GetCurrentVulkanContext().SubmitCommandsImmediate(commandBuffer);
        stageBuffer.Reset();
    }
}

class UniformSubmitRenderPass : public RenderPass
{
    SharedResources& sharedResources;

public:
    UniformSubmitRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {

    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.DeclareBuffer(this->sharedResources.CameraUniformBuffer);
        pipeline.DeclareBuffer(this->sharedResources.ModelUniformBuffer);
        pipeline.DeclareBuffer(this->sharedResources.LightUniformBuffer);
        pipeline.DeclareBuffer(this->sharedResources.MaterialUniformBuffer);
    }

    virtual void SetupDependencies(DependencyState depedencies) override
    {
        depedencies.AddBuffer(this->sharedResources.CameraUniformBuffer,   BufferUsage::TRANSFER_DESTINATION);
        depedencies.AddBuffer(this->sharedResources.ModelUniformBuffer,    BufferUsage::TRANSFER_DESTINATION);
        depedencies.AddBuffer(this->sharedResources.LightUniformBuffer,    BufferUsage::TRANSFER_DESTINATION);
        depedencies.AddBuffer(this->sharedResources.MaterialUniformBuffer, BufferUsage::TRANSFER_DESTINATION);
    }

    virtual void OnRender(RenderPassState state) override
    {
        auto FillUniform = [&state](const auto& uniformData, const auto& uniformBuffer) mutable
        {
            auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();
            auto uniformAllocation = stageBuffer.Submit(&uniformData);
            state.Commands.CopyBuffer(
                BufferInfo{ stageBuffer.GetBuffer(), uniformAllocation.Offset }, 
                BufferInfo{ uniformBuffer, 0 },
                uniformAllocation.Size
            );
        };
        auto FillUniformArray = [&state](const auto& uniformArray, const auto& uniformBuffer) mutable
        {
            auto& stageBuffer = GetCurrentVulkanContext().GetCurrentStageBuffer();
            auto uniformAllocation = stageBuffer.Submit(MakeView(uniformArray));
            state.Commands.CopyBuffer(
                BufferInfo{ stageBuffer.GetBuffer(), uniformAllocation.Offset }, 
                BufferInfo{ uniformBuffer, 0 }, 
                uniformAllocation.Size
            );
        };

        FillUniform(this->sharedResources.CameraUniform, this->sharedResources.CameraUniformBuffer);
        FillUniform(this->sharedResources.ModelUniform, this->sharedResources.ModelUniformBuffer);
        FillUniformArray(this->sharedResources.LightUniformArray, this->sharedResources.LightUniformBuffer);
        FillUniformArray(this->sharedResources.Sponza.Materials, this->sharedResources.MaterialUniformBuffer);
    }
};

class ClothRenderPass : public RenderPass
{    
    SharedResources& sharedResources;
    std::vector<ImageReference> textureArray;
public:
    Sampler TextureSampler;

    ClothRenderPass(SharedResources& sharedResources)
        : sharedResources(sharedResources)
    {
        this->TextureSampler.Init(Sampler::MinFilter::LINEAR, Sampler::MagFilter::LINEAR, Sampler::AddressMode::REPEAT, Sampler::MipFilter::LINEAR);

        for (const auto& texture : this->sharedResources.Sponza.Textures)
        {
            this->textureArray.push_back(std::ref(texture));
        }
    }

    virtual void SetupPipeline(PipelineState pipeline) override
    {
        pipeline.Shader = std::make_unique<GraphicShader>(
            ShaderLoader::LoadFromSourceFile("main_vertex.glsl", ShaderType::VERTEX, ShaderLanguage::GLSL),
            ShaderLoader::LoadFromSourceFile("main_fragment.glsl", ShaderType::FRAGMENT, ShaderLanguage::GLSL)
        );

        pipeline.VertexBindings = {
            VertexBinding{
                VertexBinding::Rate::PER_VERTEX,
                VertexBinding::BindingRangeAll
            }
        };

        pipeline.DeclareAttachment("Output"_id, Format::R8G8B8A8_UNORM);
        pipeline.DeclareAttachment("OutputDepth"_id, Format::D32_SFLOAT_S8_UINT);

        pipeline.DeclareImages(this->textureArray, ImageUsage::TRANSFER_DISTINATION);
        pipeline.DeclareImage(this->sharedResources.LookupLTCMatrix, ImageUsage::TRANSFER_DISTINATION);
        pipeline.DeclareImage(this->sharedResources.LookupLTCAmplitude, ImageUsage::TRANSFER_DISTINATION);
        pipeline.DeclareImages(this->sharedResources.LightTextures, ImageUsage::TRANSFER_DISTINATION);

        pipeline.DescriptorBindings
            .Bind(0, this->sharedResources.CameraUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(1, this->sharedResources.ModelUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(2, this->sharedResources.MaterialUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(3, this->sharedResources.LightUniformBuffer, UniformType::UNIFORM_BUFFER)
            .Bind(4, this->textureArray, UniformType::SAMPLED_IMAGE)
            .Bind(5, this->TextureSampler, UniformType::SAMPLER)
            .Bind(6, this->sharedResources.LookupLTCMatrix, this->TextureSampler, UniformType::COMBINED_IMAGE_SAMPLER)
            .Bind(7, this->sharedResources.LookupLTCAmplitude, this->TextureSampler, UniformType::COMBINED_IMAGE_SAMPLER)
            .Bind(8, this->sharedResources.LightTextures, UniformType::SAMPLED_IMAGE);
    }

    virtual void SetupDependencies(DependencyState depedencies) override
    {
        depedencies.AddAttachment("Output"_id, ClearColor{ 0.05f, 0.0f, 0.1f, 1.0f });
        depedencies.AddAttachment("OutputDepth"_id, ClearDepthStencil{ });
    }
    
    virtual void OnRender(RenderPassState state) override
    {
        auto& output = state.GetAttachment("Output"_id);
        state.Commands.SetRenderArea(output);

        for (const auto& submesh : this->sharedResources.Sponza.Submeshes)
        {
            size_t vertexCount = submesh.VertexBuffer.GetByteSize() / sizeof(ModelData::Vertex);
            state.Commands.PushConstants(state.Pass, &submesh.MaterialIndex);
            state.Commands.BindVertexBuffers(submesh.VertexBuffer);
            state.Commands.Draw((uint32_t)vertexCount, 1);
        }
    }
};

auto CreateRenderGraph(SharedResources& resources, RenderGraphOptions::Value options)
{
    RenderGraphBuilder renderGraphBuilder;
    renderGraphBuilder
        .AddRenderPass("UniformSubmitPass"_id, std::make_unique<UniformSubmitRenderPass>(resources))
        .AddRenderPass("OpaquePass"_id, std::make_unique<ClothRenderPass>(resources))
        .AddRenderPass("ImGuiPass"_id, std::make_unique<ImGuiRenderPass>("Output"_id))
        .SetOptions(options)
        .SetOutputName("Output"_id);

    return renderGraphBuilder.Build();
}

struct Camera
{
    Vector3 Position{ 40.0f, 200.0f, -90.0f };
    Vector2 Rotation{ Pi, 0.0f };
    float Fov = 65.0f;
    float MovementSpeed = 250.0f;
    float RotationMovementSpeed = 2.5f;
    float AspectRatio = 16.0f / 9.0f;
    float ZNear = 0.1f;
    float ZFar = 5000.0f;

    void Rotate(const Vector2& delta)
    {
        this->Rotation += this->RotationMovementSpeed * delta;

        constexpr float MaxAngleY = HalfPi - 0.001f;
        constexpr float MaxAngleX = TwoPi;
        this->Rotation.y = std::clamp(this->Rotation.y, -MaxAngleY, MaxAngleY);
        this->Rotation.x = std::fmod(this->Rotation.x, MaxAngleX);
    }
    
    void Move(const Vector3& direction)
    {
        Matrix3x3 view{
            std::sin(Rotation.x), 0.0f, std::cos(Rotation.x), // forward
            0.0f, 1.0f, 0.0f, // up
            std::sin(Rotation.x - HalfPi), 0.0f, std::cos(Rotation.x - HalfPi) // right
        };

        this->Position += this->MovementSpeed * (view * direction);
    }
    
    Matrix4x4 GetViewMatrix() const
    {
        Vector3 direction{
            std::cos(this->Rotation.y) * std::sin(this->Rotation.x),
            std::sin(this->Rotation.y),
            std::cos(this->Rotation.y) * std::cos(this->Rotation.x)
        };
        return MakeLookAtMatrix(this->Position, direction, Vector3{0.0f, 1.0f, 0.0f});
    }

    Matrix4x4 GetProjectionMatrix() const
    {
        return MakePerspectiveMatrix(ToRadians(this->Fov), this->AspectRatio, this->ZNear, this->ZFar);
    }

    Matrix4x4 GetMatrix()
    {
        return this->GetProjectionMatrix() * this->GetViewMatrix();
    }
};

int main()
{
    if (std::filesystem::exists(APPLICATION_WORKING_DIRECTORY))
        std::filesystem::current_path(APPLICATION_WORKING_DIRECTORY);

    WindowCreateOptions windowOptions;
    windowOptions.Position = { 100.0f, 100.0f };
    windowOptions.Size = { 1728.0f, 972.0f };
    windowOptions.ErrorCallback = WindowErrorCallback;

    Window window(windowOptions);

    VulkanContextCreateOptions vulkanOptions;
    vulkanOptions.VulkanApiMajorVersion = 1;
    vulkanOptions.VulkanApiMinorVersion = 2;
    vulkanOptions.Extensions = window.GetRequiredExtensions();
    vulkanOptions.Layers = { "VK_LAYER_KHRONOS_validation" };
    vulkanOptions.ErrorCallback = VulkanErrorCallback;
    vulkanOptions.InfoCallback = VulkanInfoCallback;

    VulkanContext Vulkan(vulkanOptions);
    SetCurrentVulkanContext(Vulkan);

    ContextInitializeOptions deviceOptions;
    deviceOptions.PreferredDeviceType = DeviceType::DISCRETE_GPU;
    deviceOptions.ErrorCallback = VulkanErrorCallback;
    deviceOptions.InfoCallback = VulkanInfoCallback;

    Vulkan.InitializeContext(window.CreateWindowSurface(Vulkan), deviceOptions);

    SharedResources sharedResources{
        Buffer{ sizeof(CameraUniformData), BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        Buffer{ sizeof(ModelUniformData), BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        Buffer{ sizeof(Mesh::Material) * MaxMaterialCount, BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        Buffer{ sizeof(LightUniformData) * MaxLightCount, BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DESTINATION, MemoryUsage::GPU_ONLY },
        { }, // sponza
        { }, // ltc matrix lookup
        { }, // ltc amplitude lookup
    };

    LoadModelGLTF(sharedResources.Sponza, "resources/Sponza/glTF/Sponza.gltf");
    Sampler ImGuiImageSampler(Sampler::MinFilter::LINEAR, Sampler::MagFilter::LINEAR, Sampler::AddressMode::REPEAT, Sampler::MipFilter::LINEAR);
    LoadImage(sharedResources.LookupLTCMatrix, "resources/lookup/ltc_matrix.dds", ImageOptions::DEFAULT);
    LoadImage(sharedResources.LookupLTCAmplitude, "resources/lookup/ltc_amplitude.dds", ImageOptions::DEFAULT);
    LoadImage(sharedResources.LightTextures.emplace_back(), "resources/textures/white_filtered.dds", ImageOptions::MIPMAPS);
    LoadImage(sharedResources.LightTextures.emplace_back(), "resources/textures/stained_glass_filtered.dds", ImageOptions::MIPMAPS);

    std::unique_ptr<RenderGraph> renderGraph = CreateRenderGraph(sharedResources, RenderGraphOptions::Value{ });

    Camera camera;
    Vector3 modelRotation{ 0.0f, HalfPi, 0.0f };

    auto& lightArray = sharedResources.LightUniformArray;

    lightArray[0].Color = Vector3{ 1.0f, 1.0f, 1.0f };
    lightArray[0].Rotation = MakeRotationMatrix(Vector3{ 0.0f, 0.0f, 0.0f });
    lightArray[0].Position = Vector3{ -400.0f, 200.0f, 0.0f };
    lightArray[0].Height = 300.0f;
    lightArray[0].Width = 50.0f;
    lightArray[0].TextureIndex = 0;

    lightArray[1].Color = Vector3{ 1.0f, 0.0f, 0.0f };
    lightArray[1].Rotation = MakeRotationMatrix(Vector3{ 0.0f, 0.0f, 0.0f });
    lightArray[1].Position = Vector3{ -45.0f, 200.0f, 1100.0f };
    lightArray[1].Height = 200.0f;
    lightArray[1].Width = 200.0f;
    lightArray[1].TextureIndex = 0;

    lightArray[2].Color = Vector3{ 0.0f, 0.0f, 1.0f };
    lightArray[2].Rotation = MakeRotationMatrix(Vector3{ 0.0f, 0.0f, 0.0f });
    lightArray[2].Position = Vector3{ -45.0f, 200.0f, 1000.0f };
    lightArray[2].Height = 200.0f;
    lightArray[2].Width = 200.0f;
    lightArray[2].TextureIndex = 0;

    lightArray[3].Color = Vector3{ 1.0f, 1.0f, 1.0f };
    lightArray[3].Rotation = MakeRotationMatrix(Vector3{ 0.0f, 0.0f, 0.0f });
    lightArray[3].Position = Vector3{ -45.0f, 200.0f, -1000.0f };
    lightArray[3].Height = 300.0f;
    lightArray[3].Width = 300.0f;
    lightArray[3].TextureIndex = 1;

    window.OnResize([&Vulkan, &sharedResources, &renderGraph, &camera](Window& window, Vector2 size) mutable
    { 
        Vulkan.RecreateSwapchain((uint32_t)size.x, (uint32_t)size.y); 
        renderGraph = CreateRenderGraph(sharedResources, RenderGraphOptions::ON_SWAPCHAIN_RESIZE);
        camera.AspectRatio = size.x / size.y;
    });
    
    ImGuiVulkanContext::Init(window, renderGraph->GetNodeByName("ImGuiPass"_id).PassNative.RenderPassHandle);

    std::map<size_t, ImTextureID> ImGuiRegisteredImages;
    for (const auto& material : sharedResources.Sponza.Materials)
    {
        if (ImGuiRegisteredImages.find(material.AlbedoIndex) == ImGuiRegisteredImages.end())
            ImGuiRegisteredImages.emplace(
                material.AlbedoIndex,
                ImGuiVulkanContext::RegisterImage(sharedResources.Sponza.Textures[material.AlbedoIndex], ImGuiImageSampler)
            );
        if (ImGuiRegisteredImages.find(material.NormalIndex) == ImGuiRegisteredImages.end())
            ImGuiRegisteredImages.emplace(
                material.NormalIndex,
                ImGuiVulkanContext::RegisterImage(sharedResources.Sponza.Textures[material.NormalIndex], ImGuiImageSampler)
            );
        if (ImGuiRegisteredImages.find(material.MetallicRoughnessIndex) == ImGuiRegisteredImages.end())
            ImGuiRegisteredImages.emplace(
                material.MetallicRoughnessIndex,
                ImGuiVulkanContext::RegisterImage(sharedResources.Sponza.Textures[material.MetallicRoughnessIndex], ImGuiImageSampler)
            );
    }

    while (!window.ShouldClose())
    {
        window.PollEvents();

        if (Vulkan.IsRenderingEnabled())
        {
            Vulkan.StartFrame();
            ImGuiVulkanContext::StartFrame();

            auto dt = ImGui::GetIO().DeltaTime;

            auto mouseMovement = ImGui::GetMouseDragDelta(MouseButton::RIGHT, 0.0f);
            ImGui::ResetMouseDragDelta(MouseButton::RIGHT);
            camera.Rotate(Vector2{ -mouseMovement.x, -mouseMovement.y } * dt);

            Vector3 movementDirection{ 0.0f };
            if (ImGui::IsKeyDown(KeyCode::W))
                movementDirection += Vector3{  1.0f,  0.0f,  0.0f };
            if (ImGui::IsKeyDown(KeyCode::A))
                movementDirection += Vector3{  0.0f,  0.0f, -1.0f };
            if (ImGui::IsKeyDown(KeyCode::S))
                movementDirection += Vector3{ -1.0f,  0.0f,  0.0f };
            if (ImGui::IsKeyDown(KeyCode::D))
                movementDirection += Vector3{  0.0f,  0.0f,  1.0f };
            if (ImGui::IsKeyDown(KeyCode::SPACE))
                movementDirection += Vector3{  0.0f,  1.0f,  0.0f };
            if (ImGui::IsKeyDown(KeyCode::LEFT_SHIFT))
                movementDirection += Vector3{  0.0f, -1.0f,  0.0f };
            if (movementDirection != Vector3{ 0.0f }) movementDirection = Normalize(movementDirection);
            camera.Move(movementDirection * dt);

            ImGui::Begin("Camera");
            ImGui::DragFloat("movement speed", &camera.MovementSpeed, 0.1f);
            ImGui::DragFloat("rotation movement speed", &camera.RotationMovementSpeed, 0.1f);
            ImGui::DragFloat3("position", &camera.Position[0]);
            ImGui::DragFloat2("rotation", &camera.Rotation[0], 0.01f);
            ImGui::DragFloat("fov", &camera.Fov);
            ImGui::End();

            sharedResources.CameraUniform.Matrix = camera.GetMatrix();
            sharedResources.CameraUniform.Position = camera.Position;


            ImGui::Begin("Model");
            ImGui::DragFloat3("rotation", &modelRotation[0], 0.01f);
            ImGui::End();

            sharedResources.ModelUniform.Matrix = MakeRotationMatrix(modelRotation);

            int lightIndex = 0;
            ImGui::Begin("Lights");

            for (auto& lightUniform : sharedResources.LightUniformArray)
            {
                ImGui::PushID(lightIndex++);
                if (ImGui::TreeNode(("light_" + std::to_string(lightIndex)).c_str()))
                {
                    auto rotation = MakeRotationAngles((Matrix4x4)lightUniform.Rotation);

                    ImGui::ColorEdit3("color", &lightUniform.Color[0], ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
                    ImGui::DragFloat3("rotation", &rotation[0], 0.01f);
                    ImGui::DragFloat3("position", &lightUniform.Position[0]);
                    ImGui::DragFloat("width", &lightUniform.Width);
                    ImGui::DragFloat("height", &lightUniform.Height);
                    ImGui::TreePop();

                    lightUniform.Rotation = MakeRotationMatrix(rotation);
                }
                ImGui::PopID();
            }
            ImGui::End();

            ImGui::Begin("Performace");
            ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
            ImGui::End();

            int materialIndex = 0;
            ImGui::Begin("Sponza materials");
            for (auto& material : sharedResources.Sponza.Materials)
            {
                ImGui::PushID(materialIndex++);

                ImGui::BeginTable(("material_" + std::to_string(materialIndex)).c_str(), 4);

                ImGui::TableSetupColumn("roughness");
                ImGui::TableSetupColumn("albedo image");
                ImGui::TableSetupColumn("normal image");
                ImGui::TableSetupColumn("metallic-roughness image");
                ImGui::TableHeadersRow();

                ImGui::TableNextColumn();
                ImGui::DragFloat("scale", &material.RoughnessScale, 0.01f, 0.0f, 1.0f);
                ImGui::TableNextColumn();
                ImGui::Image(ImGuiRegisteredImages.at(material.AlbedoIndex), { 128.0f, 128.0f });
                ImGui::TableNextColumn();
                ImGui::Image(ImGuiRegisteredImages.at(material.NormalIndex), { 128.0f, 128.0f });
                ImGui::TableNextColumn();
                ImGui::Image(ImGuiRegisteredImages.at(material.MetallicRoughnessIndex), { 128.0f, 128.0f });

                ImGui::EndTable();

                ImGui::Separator();
                ImGui::PopID();
            }
            ImGui::End();

            renderGraph->Execute(Vulkan.GetCurrentCommandBuffer());
            renderGraph->Present(Vulkan.GetCurrentCommandBuffer(), Vulkan.AcquireCurrentSwapchainImage(ImageUsage::TRANSFER_DISTINATION));

            ImGuiVulkanContext::EndFrame();
            Vulkan.EndFrame();
        }
    }

    ImGuiVulkanContext::Destroy();

    return 0;
}
