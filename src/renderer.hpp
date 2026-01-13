#pragma once

#include "device.hpp"
#include "swapchain.hpp"
#include "texture.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vx {

class Renderer;

struct UniformData {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

template <typename T>
class UniformBuffer {
public:
  UniformBuffer<T>(vx::Renderer &render);
  ~UniformBuffer<T>() {
    for (int i = 0; i < mems_.size(); i++) {
      mems_[i].unmapMemory();
    }
  }

  // no copy because memory mapped regions are affine
  UniformBuffer<T>(UniformBuffer<T> &that) = delete;
  UniformBuffer<T> &operator=(UniformBuffer<T> &that) = delete;

  UniformBuffer<T>(UniformBuffer<T> &&that) {
    that.swap(*this);
  }

  UniformBuffer<T> &operator=(UniformBuffer<T> &&that) {
    UniformBuffer<T> temp(std::move(that));
    temp.swap(*this);
    return *this;
  }

  vk::raii::Buffer &ubo(int frame_index) { return ubos_[frame_index]; }
  vk::raii::DeviceMemory &mem(int frame_index) { return mems_[frame_index]; }
  T *mapped(int frame_index) { return mapped_[frame_index]; }

private:
  std::vector<vk::raii::Buffer> ubos_;
  std::vector<vk::raii::DeviceMemory> mems_;
  std::vector<T *> mapped_;

  void swap(UniformBuffer<T> &that) {
    std::swap(ubos_, that.ubos_);
    std::swap(mems_, that.mems_);
    std::swap(mapped_, that.mapped_);
  }
};

struct ShaderData {
  // no copy because uniform buffers has no copy constructor
  Texture &texture;
  std::vector<vk::raii::DescriptorSet> descriptor_sets;
  vx::UniformBuffer<UniformData> uniforms;
};

class Renderer {
public:
  Renderer(vx::Window &window, vx::Device &device, uint32_t descriptor_count)
    : window_ (window)
    , device_ (device)
    , swapchain_ (window, device)
    , pool_ (device.create_command_pool()) {
    command_buffers = pool_.create_buffers(Swapchain::MAX_FRAMES_IN_FLIGHT);
    create_descriptor_layout();
    create_pipeline();
    create_descriptor_pool(descriptor_count);
    create_sync_objs();
  }

  ShaderData create_shader_data(Texture &texture);

  bool begin_frame();
  void bind_shader_data(ShaderData &data, UniformData &uniforms);
  void end_frame();

  float aspect_ratio() {
    return static_cast<float>(swapchain_.extent().width) /
      static_cast<float>(swapchain_.extent().height);
  }

  vx::Window &window() { return window_; }
  vx::Device &device() { return device_; }
  vx::Swapchain &swapchain() { return swapchain_; }
  vx::CommandPool &pool() { return pool_; }
  vk::raii::CommandBuffer &command_buffer() {
    return command_buffers[swapchain_.frame_index()];
  }

private:
  vx::Window &window_;
  vx::Device &device_;
  vx::Swapchain swapchain_;
  vx::CommandPool pool_;
  std::vector<vk::raii::CommandBuffer> command_buffers;

  vk::raii::DescriptorSetLayout descriptor_layout = nullptr;
  vk::raii::DescriptorPool descriptor_pool = nullptr;
  vk::raii::PipelineLayout pipeline_layout = nullptr;
  vk::raii::Pipeline pipeline = nullptr;

  std::vector<vk::raii::Semaphore> render_done_sems;
  std::vector<vk::raii::Semaphore> present_done_sems;
  std::vector<vk::raii::Fence> draw_fences;

  uint32_t image_index;

  void create_descriptor_layout();
  void create_pipeline();
  vk::raii::ShaderModule create_shader_module();
  void create_descriptor_pool(uint32_t descriptor_count);
  void create_sync_objs();

  void begin_recording(int frame_index);
  void transition_image_layout(
    vk::raii::CommandBuffer &commands,
    const vk::Image &image,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access,
    vk::AccessFlags2 dst_access,
    vk::PipelineStageFlags2 src_stage,
    vk::PipelineStageFlags2 dst_stage,
    vk::ImageAspectFlags aspect_mask);
};

}
