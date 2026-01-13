#include "texture.hpp"
#include "renderer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

using namespace vx;

Texture::Texture(vx::Renderer &render, const char *path) {
  create_image(render.pool(), path);
  create_view(render.pool());
  create_sampler(render.pool());
}

void Texture::create_image(vx::CommandPool &pool, const char *path) {
  // load raw pixel data
  int width, height, channels;
  stbi_uc *pixels = stbi_load(path, &width, &height, &channels,
    STBI_rgb_alpha);
  if (!pixels)
    throw std::runtime_error("could not open texture image :(");

  // create staging buffer
  vk::DeviceSize size = width * height * 4;
  vk::raii::Buffer staging = nullptr;
  vk::raii::DeviceMemory stagingMem = nullptr;
  pool.device().create_buffer(staging, stagingMem, size,
    vk::BufferUsageFlagBits::eTransferSrc,
    vk::MemoryPropertyFlagBits::eHostVisible |
    vk::MemoryPropertyFlagBits::eHostCoherent);

  // transfer pixel data into staging buffer
  void *data = stagingMem.mapMemory(0, size);
  memcpy(data, pixels, size);
  stagingMem.unmapMemory();
  stbi_image_free(pixels);

  // create image
  pool.device().create_image(image_, mem_, width, height, vk::Format::eR8G8B8A8Srgb,
    vk::ImageTiling::eOptimal,
    vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
    vk::MemoryPropertyFlagBits::eDeviceLocal);

  // transfer staging buffer to image
  pool.transition_image_layout(image_, vk::ImageLayout::eUndefined,
    vk::ImageLayout::eTransferDstOptimal);
  pool.copy_buffer_to_image(image_, staging, width, height);
  pool.transition_image_layout(image_, vk::ImageLayout::eTransferDstOptimal,
    vk::ImageLayout::eShaderReadOnlyOptimal);
}

void Texture::create_view(vx::CommandPool &pool) {
  view_ = pool.device().create_view(*image_, vk::Format::eR8G8B8A8Srgb,
    vk::ImageAspectFlagBits::eColor);
}

void Texture::create_sampler(vx::CommandPool &pool) {
  auto props = pool.device().physical().getProperties();
  vk::SamplerCreateInfo samplerInfo {
    .magFilter = vk::Filter::eLinear,
    .minFilter = vk::Filter::eLinear,
    .mipmapMode = vk::SamplerMipmapMode::eLinear,
    .addressModeU = vk::SamplerAddressMode::eRepeat,
    .addressModeV = vk::SamplerAddressMode::eRepeat,
    .addressModeW = vk::SamplerAddressMode::eRepeat,
    .mipLodBias = 0.,
    .anisotropyEnable = true,
    .maxAnisotropy = props.limits.maxSamplerAnisotropy,
    .compareEnable = false,
    .compareOp = vk::CompareOp::eAlways,
    .minLod = 0.,
    .maxLod = 0.,
    .borderColor = vk::BorderColor::eIntOpaqueBlack,
    .unnormalizedCoordinates = false,
  };

  sampler_ = vk::raii::Sampler(pool.device().device(), samplerInfo);
}
