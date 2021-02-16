#pragma once

#include "VK/VkBase.h"

#include <string>
#include <vector>

#include "VK/Buffer.h"
#include "VK/Framebuffer.h"
#include "VK/Model.h"
#include "VK/Texture.h"
#include "View/Camera.h"

class ProtoSSAO : public VkBase {
public:
  void OnPostInit() override;
  void OnPreDestroy() override;
  void OnUpdate(float t) override;
  void OnUpdateUIOverlay() override;

  void LoadAssets();
  void PrepareOffscreenFramebuffer();
  void PrepareUniformBuffers();

  void UpdateUniformBuffers();
  void UpdateGBufferUniformBuffer();
  void UpdateSSAOUniformBuffer();
  void UpdateLightingUniformBuffer();

  void SetupDescriptorPool();
  void SetupDescriptorSet();
  void SetupPipelines();

  void BuildCommandBuffers() override;

  void ViewChanged() override;

private:
  static constexpr inline size_t KERNEL_SIZE = 64;
  static constexpr inline size_t ROT_TEX_SIZE = 4;

  VertexLayout vertexLayout{
      {
          VertexLayoutComponent::Position,
          VertexLayoutComponent::Normal,
          VertexLayoutComponent::Color,
          VertexLayoutComponent::UV,
      },
  };

  struct {
    Model teapot;
    Model floor;
  } models;

  struct {
    Texture2D floor;
    Texture2D wall;
    Texture2D noise;
  } textures;

  struct PushConstants {
    alignas(16) glm::mat4 model;
    alignas(4) int tex;
  } pushConsts;

  struct {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
  } uboGBuffer;

  struct {
    alignas(16) glm::vec4 kernel[KERNEL_SIZE];
    alignas(16) glm::mat4 proj;
    alignas(4) float radius;
    alignas(4) float bias;
  } uboSSAO;

  struct Light {
    alignas(16) glm::vec4 pos;
    alignas(16) glm::vec3 La;
    alignas(16) glm::vec3 Ld;
  };

  struct {
    Light lights[8];
    alignas(4) int lightsNum;
    alignas(4) int displayRenderTarget;
    alignas(4) bool useBlur;
    alignas(4) float ao;
  } uboLighting;

  struct {
    Buffer gBuffer;
    Buffer ssao;
    Buffer lighting;
  } uniformBuffers;

  struct {
    VkPipeline gBuffer;
    VkPipeline ssao;
    VkPipeline lighting;
  } pipelines;

  struct {
    VkPipelineLayout gBuffer;
    VkPipelineLayout ssao;
    VkPipelineLayout lighting;
  } pipelineLayouts;

  struct {
    VkDescriptorSet gBuffer;
    VkDescriptorSet ssao;
    VkDescriptorSet lighting;
    const uint32_t maxSets = 3;
  } descriptorSets;

  struct {
    VkDescriptorSetLayout gBuffer;
    VkDescriptorSetLayout ssao;
    VkDescriptorSetLayout lighting;
  } descriptorSetLayouts;

  struct {
    Framebuffer gBuffer;
    Framebuffer ssao;
  } frameBuffers;

  Camera camera{};
};
