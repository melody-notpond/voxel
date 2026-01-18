#pragma once

#include "renderer.hpp"

#include <cstdint>
#include <vulkan/vulkan_raii.hpp>

namespace vx {

enum VoxelType : uint32_t {
  Empty = 0,
  Light = 1,
  Dark = 2,
};

struct ChunkUniforms {
  glm::mat4 model;
  glm::mat4 model_inv;
  uint32_t voxel_count;
};

class Chunk {
public:
  Chunk(Renderer &render, int x, int y, int z);

  static const int SIZE = 1;
  static const int COUNT = 8;

  void render(vx::Renderer &render);

  int x() { return x_; }
  int y() { return y_; }
  int z() { return z_; }

private:
  int x_, y_, z_;
  uint32_t voxels[COUNT][COUNT][COUNT];

  std::vector<vk::raii::DescriptorSet> descriptor_sets;
  UniformBuffer<ChunkUniforms> uniforms;
  vk::raii::Image image_ = nullptr;
  vk::raii::DeviceMemory mem_ = nullptr;
  vk::raii::ImageView view_ = nullptr;
};

}
