#pragma once

#include "VK/VkBase.h"

#include <string>
#include <vector>

#include "VK/Buffer.h"
#include "VK/Model.h"
#include "VK/Texture.h"
#include "View/Camera.h"

enum struct MetalColor : std::uint32_t {
  Nil,
  Iron,
  Silver,
  Aluminum,
  Gold,
  Copper,
  Chromium,
  Nickel,
  Titanium,
  Cobalt,
  Platinum,
};

class PBRBasic : public VkBase {
public:
  void OnPostInit() override;
  void OnPreDestroy() override;
  void OnUpdate(float t) override;
  void OnUpdateUIOverlay() override;

  void PrepareCamera();
  void LoadAssets();
  void PrepareUniformBuffers();
  void UpdateUniformBuffers();
  void UpdateLights();

  void SetupDescriptorSetLayout();
  void SetupPipelines();
  void SetupDescriptorPool();
  void SetupDescriptorSet();

  void BuildCommandBuffers() override;

  void ViewChanged() override;

private:
  struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
  } ubo;

  struct Light {
    alignas(16) glm::vec4 pos;
    alignas(4) float intensity;
  };
  struct UniformBufferObjectShared {
    alignas(16) glm::vec3 eye;
    Light lights[8];
    alignas(4) int lightsNum;
  } uboParams;

  struct Material {
    float rough;
    float metal;
    float reflect;
    float r;
    float g;
    float b;
  };
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;

  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

  Camera camera{};

  VertexLayout vertexLayout{{
      VertexLayoutComponent::Position,
      VertexLayoutComponent::Normal,
  }};
  Model model;
  struct {
    Buffer object{};
    Buffer params{};
  } uniformBuffers;

  float prevTime = 0.0f;
  float lightAngle = 0.0f;

  struct Settings {
    glm::vec3 metalSpecular{1.0f, 0.71f, 0.29f};
    float metalRough = 0.5f;
    MetalColor metalColor = MetalColor::Nil;

    glm::vec3 dielectricBaseColor{0.2f, 0.33f, 0.17f};
    float dielectricRough = 0.5f;
    float dielectricReflectance = 0.5f;
  } settings;
};
