#include "texture.hpp"
#include "renderer.hpp"
#include "vulkan/vulkan.hpp"

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

  // create image
  pool.device().create_image(image_, mem_, width, height, 1, vk::Format::eR8G8B8A8Srgb,
    vk::ImageTiling::eOptimal,
    vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
    vk::MemoryPropertyFlagBits::eDeviceLocal);

  // copy to image
  pool.copy_to_image_staged(image_, pixels, width, height, 1, 16);
  stbi_image_free(pixels);
}

void Texture::create_view(vx::CommandPool &pool) {
  view_ = pool.device().create_view(*image_, vk::ImageViewType::e2D,
    vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
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
