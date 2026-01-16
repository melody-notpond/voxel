#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "window.hpp"

namespace vx {

class CommandPool;

class SingleTimeCommands {
  SingleTimeCommands(const SingleTimeCommands &c) = delete;
  SingleTimeCommands &operator=(const SingleTimeCommands &c) = delete;
  SingleTimeCommands(const SingleTimeCommands &&c) = delete;
  SingleTimeCommands &operator=(const SingleTimeCommands &&c) = delete;

  ~SingleTimeCommands() {
    buffer.end();
    queue.submit(vk::SubmitInfo {
      .commandBufferCount = 1,
      .pCommandBuffers = &*buffer
    }, nullptr);
    queue.waitIdle();
  }

  vk::raii::CommandBuffer &operator*() { return buffer; }
  vk::raii::CommandBuffer *operator->() { return &buffer; }
private:
  SingleTimeCommands(vk::raii::Queue &queue,
    vk::raii::Device &device,
    vk::raii::CommandPool &pool)
    : queue (queue) {
    vk::CommandBufferAllocateInfo allocInfo {
      .commandPool = pool,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = 1,
    };

    buffer = std::move(vk::raii::CommandBuffers(device, allocInfo).front());
    buffer.begin(vk::CommandBufferBeginInfo {
      .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    });
  }

  vk::raii::Queue &queue;
  vk::raii::CommandBuffer buffer = nullptr;

  friend class vx::CommandPool;
};

class Device {
public:
  Device(vx::Window &window);

  void wait() { device_.waitIdle(); }
  void qwait() { queue_.waitIdle(); }

  void surface_properties(
    vk::SurfaceCapabilitiesKHR &caps,
    std::vector<vk::SurfaceFormatKHR> &formats,
    std::vector<vk::PresentModeKHR> &modes
  ) {
    caps = physical_.getSurfaceCapabilitiesKHR(surface_);
    formats = physical_.getSurfaceFormatsKHR(surface_);
    modes = physical_.getSurfacePresentModesKHR(surface_);
  }

  void create_image(
    vk::raii::Image &image,
    vk::raii::DeviceMemory &mem,
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags props);

  vk::raii::ImageView create_view(
    const vk::Image &image,
  vk::ImageViewType dim,
    vk::Format format,
    vk::ImageAspectFlags aspectMask
  );

  vk::Format find_supported_image_format(
    const std::vector<vk::Format> &formats,
    vk::ImageTiling tiling,
    vk::FormatFeatureFlags flags
  );

  vx::CommandPool create_command_pool();

  void create_buffer(
    vk::raii::Buffer &buffer,
    vk::raii::DeviceMemory &mem,
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags props
  );

  vk::raii::PhysicalDevice &physical() { return physical_; }
  vk::raii::Device &device() { return device_; }
  vk::raii::Context &context() { return context_; }
  vk::raii::Instance &instance() { return instance_; }
  vk::raii::SurfaceKHR &surface() { return surface_; }
  uint32_t queue_index() { return qindex; }
  uint32_t *queue_indices() { return &qindex; }
  vk::raii::Queue &queue() { return queue_; }

private:
  #ifdef NDEBUG
  static constexpr bool enable_validation_layers = false;
  const std::vector<const char *> validation_layers;
  #else
  static constexpr bool enable_validation_layers = true;
  const std::vector<const char *> validation_layers {
    "VK_LAYER_KHRONOS_validation"
  };
  #endif

  const std::vector<const char *> device_exts = {
    vk::KHRSwapchainExtensionName,
    vk::KHRSpirv14ExtensionName,
    vk::KHRSynchronization2ExtensionName,
    vk::KHRCreateRenderpass2ExtensionName
  };

  vk::raii::Context context_;
  vk::raii::Instance instance_ = nullptr;
  vk::raii::DebugUtilsMessengerEXT debug_messenger = nullptr;
  vk::raii::SurfaceKHR surface_ = nullptr;
  vk::raii::PhysicalDevice physical_ = nullptr;
  vk::raii::Device device_ = nullptr;

  uint32_t qindex;
  vk::raii::Queue queue_ = nullptr;

  void create_instance();
  void setup_debug();
  void create_surface(vx::Window &window);
  void pick_physical();
  void create_logical();

  bool is_suitable(vk::raii::PhysicalDevice device);
  std::optional<uint32_t> find_queue_fams(vk::raii::PhysicalDevice device);

  uint32_t find_mem_type(uint32_t typeFilter,
    vk::MemoryPropertyFlags propFilter);
};

class CommandPool {
public:
  std::vector<vk::raii::CommandBuffer> create_buffers(uint32_t count);
  vk::raii::CommandBuffer create_buffer();
  [[nodiscard]] vx::SingleTimeCommands single_time_commands();

  void copy_to_image_staged(
    vk::raii::Image &image,
    void *data,
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    float elem_size
  );

  void copy_to_buffer_staged(
    vk::raii::Buffer &buffer,
    vk::raii::DeviceMemory &mem,
    vk::BufferUsageFlags usage,
    const void *data,
    vk::DeviceSize size);

  void copy_buffer(
    vk::raii::Buffer &dst,
    vk::raii::Buffer &src,
    vk::DeviceSize size);

  void copy_buffer_to_image(
    const vk::raii::Image &image,
    const vk::raii::Buffer &buffer,
    uint32_t width,
    uint32_t height,
    uint32_t depth
  );

  void transition_image_layout(
    const vk::raii::Image &image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout
  );

  vx::Device &device() { return device_; }
  vk::raii::CommandPool &pool() { return pool_; }
private:
  CommandPool(vx::Device &device, vk::CommandPoolCreateInfo info)
    : device_ (device)
    , pool_ (device.device(), info) { }

  vx::Device &device_;
  vk::raii::CommandPool pool_ = nullptr;

  friend class vx::Device;
};

}
