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
  void OnRender() override;
  void OnUpdate(float t) override;
  void OnUpdateUIOverlay() override;

  void LoadAssets();
  void PrepareOffscreenFramebuffer();
  void PrepareUniformBuffers();

  void UpdateUniformBuffers();
  void UpdateOffscreenUniformBuffers();
  void UpdateCompositionUniformBuffers();

  void SetupDescriptorSetLayout();
  void SetupPipelines();
  void SetupDescriptorPool();
  void SetupDescriptorSet();

  void BuildCommandBuffers() override;

  void BuildDeferredCommandBuffer();

  void ViewChanged() override;

private:
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
  } textures;

  struct PushConstants {
    alignas(16) glm::mat4 model;
    alignas(4) int tex;
  } pushConsts;

  struct {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
  } uboOffscreenVS;

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
  } uboComposition;

  struct {
    Buffer gBuffer;
    Buffer lighting;
  } uniformBuffers;

  struct {
    VkPipeline gBuffer;
    VkPipeline lighting;
  } pipelines;
  VkPipelineLayout pipelineLayout;

  struct {
    VkDescriptorSet gBuffer;
    VkDescriptorSet lighting;
  } descriptorSets;
  VkDescriptorSetLayout descriptorSetLayout;

  Framebuffer offscreenFramebuffer;

  VkCommandBuffer offscreenCmdBuffer = VK_NULL_HANDLE;
  VkSemaphore offscreenSemaphore = VK_NULL_HANDLE;

  Camera camera{};

  float prevTime = 0.0f;
  float camAngle = 0.0f;

  struct Settings {
    int dispRenderTarget = 0;
  } settings;
};
