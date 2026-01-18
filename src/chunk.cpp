#include "chunk.hpp"
#include "vulkan/vulkan.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <glm/matrix.hpp>

using namespace vx;

Chunk::Chunk(Renderer &render, int x, int y, int z)
  : x_ (x)
  , y_ (y)
  , z_ (z)
  , descriptor_sets (render.new_descriptor_set())
  , uniforms (render) {
  // generate a sphere
  float radius_sq = (float) COUNT / 2. * (float) COUNT / 2.;
  float c = (float) COUNT / 2.;

  for (int i = 0; i < COUNT; i++) {
    for (int j = 0; j < COUNT; j++) {
      for (int k = 0; k < COUNT; k++) {
        float i_ = (float) i;
        float j_ = (float) j;
        float k_ = (float) k;
        float dist = (i_ - c) * (i_ - c) + (j_ - c) * (j_ - c) +
          (k_ - c) * (k_ - c);
        if (dist < radius_sq) {
          if ((i + j + k) & 1)
            voxels[i][j][k] = VoxelType::Light;
          else voxels[i][j][k] = VoxelType::Dark;
        } else voxels[i][j][k] = VoxelType::Empty;
      }
    }
  }

  for (int i = 0; i < COUNT; i++) {
    if (i & 1)
      voxels[i][0][0] = VoxelType::Light;
    else voxels[i][0][0] = VoxelType::Dark;
  }

  // create image
  render.device().create_image(image_, mem_, COUNT, COUNT, COUNT,
    vk::Format::eR32Uint, vk::ImageTiling::eLinear,
    vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
    vk::MemoryPropertyFlagBits::eDeviceLocal);

  // copy voxels to image
  render.pool().copy_to_image_staged(image_, voxels, COUNT, COUNT, COUNT, 4);

  // create image view
  view_ = render.device().create_view(*image_, vk::ImageViewType::e3D,
    vk::Format::eR32Uint, vk::ImageAspectFlagBits::eColor);

  // put uniforms and image in descriptor
  vk::DescriptorImageInfo image_info {
    .imageView = view_,
    .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
  };

  for (int i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++) {
    vk::DescriptorBufferInfo buffer_info {
      .buffer = uniforms.ubo(i),
      .offset = 0,
      .range = sizeof(ChunkUniforms)
    };

    std::array write_sets {
      vk::WriteDescriptorSet {
        .dstSet = descriptor_sets[i],
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pBufferInfo = &buffer_info
      },
      vk::WriteDescriptorSet {
        .dstSet = descriptor_sets[i],
        .dstBinding = 2,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eSampledImage,
        .pImageInfo = &image_info
      },
    };

    render.device().device().updateDescriptorSets(write_sets, {});
  }
}

void Chunk::render(vx::Renderer &render) {
  glm::mat4 model = glm::scale(glm::mat4(1.), {SIZE, SIZE, SIZE});
  model = glm::translate(model, {x_, y_, z_});
  uniforms.upload(render.swapchain().frame_index(), {
    .model = model,
    .model_inv = glm::inverse(model),
    .voxel_count = COUNT
  });

  render.bind_descriptor(descriptor_sets);
  render.command_buffer().draw(36, 1, 0, 0);
}
