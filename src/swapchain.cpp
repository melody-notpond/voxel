#include "swapchain.hpp"

#include "device.hpp"

using namespace vx;

Swapchain::Swapchain(Window &window, Device &device) {
  create_swapchain(window, device);
  create_views(device);
  create_depth_resources(device);
}

void Swapchain::recreate(Window &window, Device &device) {
  int width, height;
  window.fb_size(width, height);
  while (width == 0 || height == 0) {
    window.wait_events();
    window.fb_size(width, height);
  }

  device.device().waitIdle();

  swapchain_ = nullptr;
  image_views.clear();
  create_swapchain(window, device);
  create_views(device);
  create_depth_resources(device);
}

void Swapchain::create_swapchain(Window &window, Device &device) {
  vk::SurfaceCapabilitiesKHR caps;
  std::vector<vk::SurfaceFormatKHR> formats;
  std::vector<vk::PresentModeKHR> modes;
  device.surface_properties(caps, formats, modes);
  auto format = choose_format(formats);
  auto mode = choose_present_mode(modes);
  auto extent = choose_extent(window, caps);

  // triple buffering is a nice default if supported, but go with whatever
  // the gpu can support
  uint32_t image_count = std::max(3u, caps.minImageCount);
  if (caps.maxImageCount > 0 && image_count > caps.maxImageCount)
    image_count = caps.maxImageCount;

  vk::SwapchainCreateInfoKHR swapchain_info {
    .flags = vk::SwapchainCreateFlagsKHR(),
    .surface = device.surface(),
    .minImageCount = image_count,
    .imageFormat = format.format,
    .imageColorSpace = format.colorSpace,
    .imageExtent = extent,
    .imageArrayLayers = 1,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
    .imageSharingMode = vk::SharingMode::eExclusive,
    .queueFamilyIndexCount = 1,
    .pQueueFamilyIndices = device.queue_indices(),
    .preTransform = caps.currentTransform,
    .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
    .presentMode = mode,
    .clipped = vk::True,
    .oldSwapchain = nullptr
  };

  swapchain_ = vk::raii::SwapchainKHR(device.device(), swapchain_info);
  images = swapchain_.getImages();
  format_ = format.format;
  extent_ = extent;
}

vk::SurfaceFormatKHR Swapchain::choose_format(
  const std::vector<vk::SurfaceFormatKHR> &formats
) {
  for (auto format : formats) {
    if (format.format == vk::Format::eB8G8R8A8Srgb &&
      format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
      return format;
  }

  return formats[0];
}

vk::PresentModeKHR Swapchain::choose_present_mode(
  const std::vector<vk::PresentModeKHR> &availableModes
) {
  for (auto mode : availableModes) {
    if (mode == vk::PresentModeKHR::eMailbox)
      return mode;
  }

  return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Swapchain::choose_extent(
  Window &window,
  const vk::SurfaceCapabilitiesKHR &caps
) {
  // just get the size of the framebuffer lmao
  int width, height;
  window.fb_size(width, height);
  return {
    std::clamp<uint32_t>(width, caps.minImageExtent.width,
      caps.maxImageExtent.width),
    std::clamp<uint32_t>(height, caps.minImageExtent.height,
      caps.maxImageExtent.height)
  };
}

void Swapchain::create_views(Device &device) {
  vk::ImageViewCreateInfo view_info {
    .viewType = vk::ImageViewType::e2D,
    .format = format_,
    .subresourceRange = {
      .aspectMask = vk::ImageAspectFlagBits::eColor,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    }
  };

  for (auto image : images) {
    view_info.image = image;
    image_views.emplace_back(device.device(), view_info);
  }
}

void Swapchain::create_depth_resources(Device &device) {
  depth_format_ = find_depth_format(device);
  device.create_image(depth_image_, depth_mem_, extent_.width,
    extent_.height, 1, depth_format_, vk::ImageTiling::eOptimal,
    vk::ImageUsageFlagBits::eDepthStencilAttachment,
    vk::MemoryPropertyFlagBits::eDeviceLocal);
  depth_view_ = device.create_view(depth_image_,
    vk::ImageViewType::e2D, depth_format_, vk::ImageAspectFlagBits::eDepth);
}

bool Swapchain::has_stencil() {
  return depth_format_ == vk::Format::eD32SfloatS8Uint ||
    depth_format_ == vk::Format::eD24UnormS8Uint;
}

vk::Format Swapchain::find_depth_format(Device &device) {
  return device.find_supported_image_format({
    vk::Format::eD32Sfloat,
    vk::Format::eD32SfloatS8Uint,
    vk::Format::eD24UnormS8Uint
  }, vk::ImageTiling::eOptimal,
  vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}
