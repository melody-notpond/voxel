#include "device.hpp"

#include <iostream>
#include <vulkan/vulkan_raii.hpp>

using namespace vx;

std::vector<vk::raii::CommandBuffer> CommandPool::create_buffers(uint32_t count)
{
  vk::CommandBufferAllocateInfo alloc_info {
    .commandPool = pool_,
    .level = vk::CommandBufferLevel::ePrimary,
    .commandBufferCount = count
  };

  return vk::raii::CommandBuffers(device_.device(), alloc_info);
}

vk::raii::CommandBuffer CommandPool::create_buffer() {
  return std::move(create_buffers(1).front());
}

[[nodiscard]] vx::SingleTimeCommands CommandPool::single_time_commands() {
  vk::CommandBufferAllocateInfo info {
    .commandPool = pool_,
    .level = vk::CommandBufferLevel::ePrimary,
    .commandBufferCount = 1,
  };

  auto buffer = std::move(vk::raii::CommandBuffers(
    device_.device(), info).front());
  buffer.begin(vk::CommandBufferBeginInfo {
    .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
  });
  return SingleTimeCommands(device_.queue(), device_.device(), pool_);
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_cb(
  vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
  vk::DebugUtilsMessageTypeFlagsEXT type,
  const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
  void *pUserData
) {
  // just print the validation layer's debug message
  std::cerr << "validation layer: type " << to_string(type) << " msg: " <<
    pCallbackData->pMessage << std::endl;
  return vk::False;
}

Device::Device(Window &window) {
  create_instance();
  setup_debug();
  create_surface(window);
  pick_physical();
  create_logical();
}

void Device::create_instance() {
  // get required layers
  std::vector<const char *> required_layers;
  if (enable_validation_layers)
    required_layers.assign(validation_layers.begin(), validation_layers.end());

  // check that all required layers are supported
  auto layer_props = context_.enumerateInstanceLayerProperties();
  if (std::ranges::any_of(required_layers, [&](auto const &layer) {
      return std::ranges::none_of(layer_props, [&](auto const &prop) {
        return !strcmp(prop.layerName, layer);
      });
  })) {
    throw std::runtime_error("some required layers are unsupported");
  }

  // get required glfw extensions
  uint32_t ext_count = 0;
  auto glfw_exts = glfwGetRequiredInstanceExtensions(&ext_count);
  std::vector required_exts(glfw_exts, glfw_exts + ext_count);
  if (enable_validation_layers)
    required_exts.push_back(vk::EXTDebugUtilsExtensionName);

  // check that all required extensions are supported
  auto ext_props = context_.enumerateInstanceExtensionProperties();
  if (std::ranges::any_of(required_exts, [&](auto const &ext) {
      return std::ranges::none_of(ext_props, [&](auto const &prop) {
        return !strcmp(prop.extensionName, ext);
      });
  })) {
    throw std::runtime_error("some required extensions are unsupported");
  }

  // create instance
  constexpr vk::ApplicationInfo app_info {
    .pApplicationName = "voxels",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "No Engine",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = vk::ApiVersion14,
  };

  vk::InstanceCreateInfo instance_info {
    .pApplicationInfo = &app_info,
    .enabledLayerCount = static_cast<uint32_t>(required_layers.size()),
    .ppEnabledLayerNames = required_layers.data(),
    .enabledExtensionCount = static_cast<uint32_t>(required_exts.size()),
    .ppEnabledExtensionNames = required_exts.data(),
  };

  instance_ = vk::raii::Instance(context_, instance_info);
}

void Device::setup_debug() {
  if (!enable_validation_layers)
    return;

  vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
    // vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo    |
    vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError 
  );
  vk::DebugUtilsMessageTypeFlagsEXT messageTypes(
    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral     |
    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
  );
  vk::DebugUtilsMessengerCreateInfoEXT debugInfo {
    .messageSeverity = severityFlags,
    .messageType = messageTypes,
    .pfnUserCallback = &debug_cb
  };

  debug_messenger = instance_.createDebugUtilsMessengerEXT(debugInfo);
}

void Device::create_surface(Window &window) {
  VkSurfaceKHR _surface;
  if (glfwCreateWindowSurface(*instance_, window.window(), nullptr, &_surface))
    throw std::runtime_error("failed to create surface");
  surface_ = vk::raii::SurfaceKHR(instance_, _surface);
}

void Device::pick_physical() {
  auto devices = instance_.enumeratePhysicalDevices();
  if (devices.empty()) {
    throw std::runtime_error("no vulkan compatible devices found! :(");
  }

  for (const auto &device : devices) {
    if (is_suitable(device)) {
      physical_ = device;
      return;
    }
  }

  throw std::runtime_error("found no suitable vulkan device :(");
}

bool Device::is_suitable(vk::raii::PhysicalDevice device) {
  // check vulkan api
  if (device.getProperties().apiVersion < VK_API_VERSION_1_3)
    return false;

  // check queue families
  auto indices = find_queue_fams(device);
  if (!indices)
    return false;

  // check extensions
  auto extensions = device.enumerateDeviceExtensionProperties();
  for (auto const &ext : device_exts) {
    if (std::ranges::none_of(extensions, [&](auto const &device_ext) {
      return !strcmp(ext, device_ext.extensionName);
    })) return false;
  }

  // check features
  auto features = device.getFeatures2<vk::PhysicalDeviceFeatures2,
    vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan13Features,
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
  if (!features.get<vk::PhysicalDeviceFeatures2>()
        .features.samplerAnisotropy ||
      !features.get<vk::PhysicalDeviceVulkan11Features>()
        .shaderDrawParameters ||
      !features.get<vk::PhysicalDeviceVulkan13Features>()
        .dynamicRendering ||
      !features.get<vk::PhysicalDeviceVulkan13Features>()
        .synchronization2 ||
      !features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>()
        .extendedDynamicState)
    return false;
  return true;
}

std::optional<uint32_t>
Device::find_queue_fams(vk::raii::PhysicalDevice device) {
  // we want a queue family that can support both graphics and presenting to
  // a window surface
  auto queue_fams = device.getQueueFamilyProperties();
  for (uint32_t i = 0; i < queue_fams.size(); i++) {
    if (device.getSurfaceSupportKHR(i, surface_) &&
      (queue_fams[i].queueFlags & vk::QueueFlagBits::eGraphics))
      return i;
  }

  return std::nullopt;
}

void Device::create_logical() {
  // set up queue
  auto qfp = physical_.getQueueFamilyProperties();
  qindex = find_queue_fams(physical_).value();
  float queuePriority = 1;
  vk::DeviceQueueCreateInfo queueCreateInfo {
    .queueFamilyIndex = qindex,
    .queueCount = 1,
    .pQueuePriorities = &queuePriority
  };

  // set up device features
  vk::StructureChain features {
    vk::PhysicalDeviceFeatures2 { .features = { .samplerAnisotropy = true } },
    vk::PhysicalDeviceVulkan13Features {
      .synchronization2 = true,
      .dynamicRendering = true
    },
    vk::PhysicalDeviceVulkan11Features { .shaderDrawParameters = true },
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT {
      .extendedDynamicState = true
    }
  };

  // create device and queue
  vk::DeviceCreateInfo deviceCreateInfo {
    .pNext = features.get<vk::PhysicalDeviceFeatures2>(),
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = &queueCreateInfo,
    .enabledExtensionCount = static_cast<uint32_t>(device_exts.size()),
    .ppEnabledExtensionNames = device_exts.data()
  };

  device_ = vk::raii::Device(physical_, deviceCreateInfo);
  queue_ = vk::raii::Queue(device_, qindex, 0);
}

void Device::create_image(
  vk::raii::Image &image,
  vk::raii::DeviceMemory &mem,
  uint32_t width,
  uint32_t height,
  uint32_t depth,
  vk::Format format,
  vk::ImageTiling tiling,
  vk::ImageUsageFlags usage,
  vk::MemoryPropertyFlags props
) {
  vk::ImageCreateInfo imageInfo {
    .imageType = depth == 1 ? vk::ImageType::e2D : vk::ImageType::e3D,
    .format = format,
    .extent = {width, height, depth},
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = vk::SampleCountFlagBits::e1,
    .tiling = tiling,
    .usage = usage,
    .sharingMode = vk::SharingMode::eExclusive
  };

  image = vk::raii::Image(device_, imageInfo);

  vk::MemoryRequirements reqs = image.getMemoryRequirements();
  vk::MemoryAllocateInfo allocInfo {
    .allocationSize = reqs.size,
    .memoryTypeIndex = find_mem_type(reqs.memoryTypeBits, props)
  };

  mem = vk::raii::DeviceMemory(device_, allocInfo);
  image.bindMemory(mem, 0);
}

vk::raii::ImageView Device::create_view(
  const vk::Image &image,
  vk::ImageViewType dim,
  vk::Format format,
  vk::ImageAspectFlags aspectMask
) {
  vk::ImageViewCreateInfo imageViewInfo {
    .image = image,
    .viewType = dim,
    .format = format,
    .subresourceRange = {
      .aspectMask = aspectMask,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    }
  };

  return vk::raii::ImageView(device_, imageViewInfo);
}

uint32_t Device::find_mem_type(
  uint32_t typeFilter,
  vk::MemoryPropertyFlags propFilter
) {
  auto props = physical_.getMemoryProperties();

  for (uint32_t i = 0; i < props.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
      (props.memoryTypes[i].propertyFlags & propFilter) == propFilter)
      return i;
  }

  throw std::runtime_error("could not find suitable device memory");
}

vk::Format Device::find_supported_image_format(
  const std::vector<vk::Format> &formats,
  vk::ImageTiling tiling,
  vk::FormatFeatureFlags flags
) {
  for (const auto format : formats) {
    auto props = physical_.getFormatProperties(format);
    if (tiling == vk::ImageTiling::eLinear &&
      (props.linearTilingFeatures & flags) == flags)
      return format;
    if (tiling == vk::ImageTiling::eOptimal &&
      (props.optimalTilingFeatures & flags) == flags)
      return format;
  }

  throw std::runtime_error("failed to find supported format");
}

CommandPool Device::create_command_pool() {
  vk::CommandPoolCreateInfo poolInfo {
    .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
    .queueFamilyIndex = qindex
  };

  return CommandPool(*this, poolInfo);
}

void CommandPool::copy_to_image_staged(
  vk::raii::Image &image,
  void *data,
  uint32_t width,
  uint32_t height,
  uint32_t depth,
  float elem_size
) {
  // create staging buffer
  vk::DeviceSize size = width * height * depth * elem_size;
  vk::raii::Buffer staging = nullptr;
  vk::raii::DeviceMemory stagingMem = nullptr;
  device_.create_buffer(staging, stagingMem, size,
    vk::BufferUsageFlagBits::eTransferSrc,
    vk::MemoryPropertyFlagBits::eHostVisible |
    vk::MemoryPropertyFlagBits::eHostCoherent);

  // transfer pixel data into staging buffer
  void *ptr = stagingMem.mapMemory(0, size);
  memcpy(ptr, data, size);
  stagingMem.unmapMemory();

  // transfer staging buffer to image
  transition_image_layout(image, vk::ImageLayout::eUndefined,
    vk::ImageLayout::eTransferDstOptimal);
  copy_buffer_to_image(image, staging, width, height, depth);
  transition_image_layout(image, vk::ImageLayout::eTransferDstOptimal,
    vk::ImageLayout::eShaderReadOnlyOptimal);
}

void CommandPool::copy_to_buffer_staged(
  vk::raii::Buffer &buffer,
  vk::raii::DeviceMemory &mem,
  vk::BufferUsageFlags usage,
  const void *data,
  vk::DeviceSize size
) {
  // create buffer
  if (buffer == nullptr || mem == nullptr)
    device_.create_buffer(buffer, mem, size,
      usage | vk::BufferUsageFlagBits::eTransferDst,
      vk::MemoryPropertyFlagBits::eDeviceLocal);

  // create staging buffer
  vk::raii::Buffer staging = nullptr;
  vk::raii::DeviceMemory staging_mem = nullptr;
  device_.create_buffer(staging, staging_mem, size,
    vk::BufferUsageFlagBits::eTransferSrc,
    vk::MemoryPropertyFlagBits::eHostVisible |
    vk::MemoryPropertyFlagBits::eHostCoherent);

  // write our data to staging buffer
  void *ptr = staging_mem.mapMemory(0, size);
  memcpy(ptr, data, size);
  staging_mem.unmapMemory();

  // copy staging buffer to buffer
  copy_buffer(buffer, staging, size);
}

void Device::create_buffer(
  vk::raii::Buffer &buffer,
  vk::raii::DeviceMemory &mem,
  vk::DeviceSize size,
  vk::BufferUsageFlags usage,
  vk::MemoryPropertyFlags props
) {
  // create buffer
  vk::BufferCreateInfo buffer_info {
    .size = size,
    .usage = usage,
    .sharingMode = vk::SharingMode::eExclusive
  };

  buffer = vk::raii::Buffer(device_, buffer_info);

  // allocate buffer in memory
  vk::MemoryRequirements reqs = buffer.getMemoryRequirements();
  uint32_t memIndex = find_mem_type(reqs.memoryTypeBits, props);
  vk::MemoryAllocateInfo allocInfo {
    .allocationSize = reqs.size,
    .memoryTypeIndex = memIndex
  };

  mem = vk::raii::DeviceMemory(device_, allocInfo);

  // bind the memory we just allocated to the buffer
  buffer.bindMemory(mem, 0);
}

void CommandPool::copy_buffer(
  vk::raii::Buffer &dst,
  vk::raii::Buffer &src,
  vk::DeviceSize size
) {
  auto copy_buffer = single_time_commands();
  copy_buffer->copyBuffer(src, dst, vk::BufferCopy(0, 0, size));
}

void CommandPool::copy_buffer_to_image(
  const vk::raii::Image &image,
  const vk::raii::Buffer &buffer,
  uint32_t width,
  uint32_t height,
  uint32_t depth
) {
  auto copy = single_time_commands();
  vk::BufferImageCopy region {
    .bufferOffset = 0,
    .bufferRowLength = 0,
    .bufferImageHeight = 0,
    .imageSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
    .imageOffset = {0, 0, 0},
    .imageExtent = {width, height, depth}
  };

  copy->copyBufferToImage(buffer, image,
    vk::ImageLayout::eTransferDstOptimal, region);
}

void CommandPool::transition_image_layout(
  const vk::raii::Image &image,
  vk::ImageLayout oldLayout,
  vk::ImageLayout newLayout
) {
  auto transition = single_time_commands();

  // pipeline barriers sync access to resources so that writes finish before
  // reads start, but also used to transition image layouts and transfer
  // exclusive resources between queues
  vk::ImageMemoryBarrier barrier {
    .oldLayout = oldLayout,
    .newLayout = newLayout,
    .image = image,
    .subresourceRange = {
      .aspectMask = vk::ImageAspectFlagBits::eColor,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    },
  };

  vk::PipelineStageFlags srcStage;
  vk::PipelineStageFlags dstStage;
  if (oldLayout == vk::ImageLayout::eUndefined &&
      newLayout == vk::ImageLayout::eTransferDstOptimal) {
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
    srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
    dstStage = vk::PipelineStageFlagBits::eTransfer;
  } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
      newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    srcStage = vk::PipelineStageFlagBits::eTransfer;
    dstStage = vk::PipelineStageFlagBits::eFragmentShader;
  } else throw std::invalid_argument("unsupported layout transition");

  transition->pipelineBarrier(srcStage, dstStage, {}, {}, nullptr, barrier);
}
