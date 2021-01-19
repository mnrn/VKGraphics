#include "Mesh/TriangleMesh.h"

void TriangleMesh::Destroy(const Instance &instance) {
  uniformBuffer.Destroy(instance);
  indexBuffer.Destroy(instance);
  vertexBuffer.Destroy(instance);
}

void TriangleMesh::Draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout) {
  VkDeviceSize offsets[] = {0};
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                          0, 1, &descriptorSet, 0, nullptr);
  vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer.buffer, offsets);
  vkCmdBindIndexBuffer(cmd, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(cmd, indexCount, 1, 0, 0, 1);
}
