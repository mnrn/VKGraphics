#include "VK/Primitive/Vertex.h"

std::vector<VkVertexInputBindingDescription> Vertex::Bindings() {
  std::vector<VkVertexInputBindingDescription> bind{};

  // binding = 0
  bind[0].binding = 0;
  bind[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  bind[0].stride = sizeof(Vertex);

  return bind;
}

std::vector<VkVertexInputAttributeDescription> Vertex::Attributes() {
  // NOTE:
  // 以下で使用するoffsetof演算子は構造体のレイアウトがコンパイラによって違う場合に対応するために使用する必要があります。
  std::vector<VkVertexInputAttributeDescription> attr(4);

  // binding = 0, location = 0
  attr[0].binding = 0;
  attr[0].location = 0;
  attr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attr[0].offset = offsetof(Vertex, position);

  // binding = 0, location = 1
  attr[1].binding = 0;
  attr[1].location = 1;
  attr[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attr[1].offset = offsetof(Vertex, normal);

  // binding = 0, location = 2
  attr[2].binding = 0;
  attr[2].location = 2;
  attr[2].format = VK_FORMAT_R32G32B32_SFLOAT;
  attr[2].offset = offsetof(Vertex, color);

  // binding = 0, location = 3
  attr[3].binding = 0;
  attr[3].location = 3;
  attr[3].format = VK_FORMAT_R32G32_SFLOAT;
  attr[3].offset = offsetof(Vertex, texCoord);

  return attr;
}
