#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "device.hpp"

namespace vx {

class Renderer;

class Texture {
public:
  Texture(vx::Renderer &render, const char *path);

  vk::raii::Image &image() { return image_; }
  vk::raii::DeviceMemory &mem() { return mem_; }
  vk::raii::ImageView &view() { return view_; }
  vk::raii::Sampler &sampler() { return sampler_; }

private:
  vk::raii::Image image_ = nullptr;
  vk::raii::DeviceMemory mem_ = nullptr;
  vk::raii::ImageView view_ = nullptr;
  vk::raii::Sampler sampler_ = nullptr;

  void create_image(vx::CommandPool &pool, const char *path);
  void create_view(vx::CommandPool &pool);
  void create_sampler(vx::CommandPool &pool);
};

}
