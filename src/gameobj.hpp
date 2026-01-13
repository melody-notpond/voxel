#pragma once

#include "model.hpp"
#include "renderer.hpp"
#include "texture.hpp"

namespace vx {

struct GameObj {
  Model &model;
  vx::ShaderData shader_data;

  vx::UniformData uniforms;

  std::vector<vk::raii::DescriptorSet> descriptor_sets;

  void render(vx::Renderer &render);
};

}
