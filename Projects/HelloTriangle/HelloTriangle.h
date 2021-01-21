#pragma once

#include "VK/VkBase.h"

class HelloTriangle : public VkBase {
public:
  void OnPostInit() override;
  void OnPreDestroy() override;

  void PrepareVertices();
  void PrepareUniformBuffers();
  void UpdateUniformBuffers();

  void SetupDescriptorSetLayout();
  void PreparePipelines();
  void SetupDescriptorPool();
  void SetupDescriptorSet();

  void BuildCommandBuffers() override;

private:
  // パイプラインレイアウトは記述子セットにアクセスするためにパイプラインによって使用されます。
  // パイプラインで使用されるシェーダーステージとシェーダーリソース間のインターフェースを(実際のデータをバインドせずに)定義します。
  // パイプラインレイアウトはインターフェースが一致する限り、複数のパイプライン間で共有できます。
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

  // パイプラインは、パイプラインに影響を与えるすべてのステートを事前処理するために使用されます。
  // OpenGLでは、すべてのステートをほぼいつでも変更できますが、Vulkanではグラフィックス(コンピュート)パイプラインのステートを事前にレイアウトする必要があります。
  // したがって、非動的パイプラインステートの組み合わせごとに、新しいパイプラインが必要です。(いくつか例外はあります)
  VkPipeline pipeline = VK_NULL_HANDLE;

  // 記述子セットレイアウトは、シェーダーバインディングレイアウトを記述します。(実際には記述子を参照しません)
  // パイプラインレイアウトと同様に、それらはほとんど設計図であり、レイアウトが一致する限り、さまざまな記述子セットで使用できます。
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

  // 記述子セットはバインディングポイントにバインドされたリソースをシェーダーに格納します。
  // さまざまなシェーダーのバインディングポイントを、それらのバインディングに使用されるバッファーとイメージに接続します。
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

  struct {
    VkDeviceMemory memory;
    VkBuffer buffer;
  } vertices;

  struct {
    VkDeviceMemory memory;
    VkBuffer buffer;
    uint32_t count;
  } indices;

  struct {
    VkDeviceMemory memory;
    VkBuffer buffer;
    VkDescriptorBufferInfo descriptor;
  } uniform;
};
