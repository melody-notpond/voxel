#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "device.hpp"
#include "window.hpp"

namespace vx {

class Swapchain {
public:
  Swapchain(Window &window, Device &device);

  static const int MAX_FRAMES_IN_FLIGHT = 2;

  bool has_stencil();

  vk::raii::SwapchainKHR &swapchain() { return swapchain_; }
  uint32_t image_count() { return images.size(); }
  vk::Image &image(int i) { return images[i]; }
  vk::raii::ImageView &image_view(int i) { return image_views[i]; }
  vk::Format &format() { return format_; }
  vk::Extent2D &extent() { return extent_; }
  uint32_t frame_index() { return findex; }
  vk::raii::Image &depth_image() { return depth_image_; }
  vk::raii::DeviceMemory &depth_mem() { return depth_mem_; }
  vk::raii::ImageView &depth_view() { return depth_view_; }
  vk::Format &depth_format() { return depth_format_; }

  void next_frame() { findex = (findex + 1) % MAX_FRAMES_IN_FLIGHT; }

  void recreate(vx::Window &window, vx::Device &device);
private:
  vk::raii::SwapchainKHR swapchain_ = nullptr;
  std::vector<vk::Image> images;
  std::vector<vk::raii::ImageView> image_views;
  vk::Format format_;
  vk::Extent2D extent_;
  uint32_t findex = 0;

  vk::raii::Image depth_image_ = nullptr;
  vk::raii::DeviceMemory depth_mem_ = nullptr;
  vk::raii::ImageView depth_view_ = nullptr;
  vk::Format depth_format_;

  void create_swapchain(vx::Window &window, vx::Device &device);
  void create_views(Device &device);
  void create_depth_resources(Device &device);

  vk::SurfaceFormatKHR choose_format(
    const std::vector<vk::SurfaceFormatKHR> &formats);
  vk::PresentModeKHR choose_present_mode(
    const std::vector<vk::PresentModeKHR> &modes);
  vk::Extent2D choose_extent(Window &window,
    const vk::SurfaceCapabilitiesKHR &caps);
  vk::Format find_depth_format(Device &device);
};

}
