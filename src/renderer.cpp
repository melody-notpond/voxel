#include "renderer.hpp"

#include "model.hpp"
#include "shaders.h"

using namespace vx;

template<typename T>
UniformBuffer<T>::UniformBuffer(Renderer &render) {
  vk::DeviceSize size = sizeof(T);
  for (int i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++) {
    ubos_.push_back(nullptr);
    mems_.push_back(nullptr);
  }

  // each frame in flight gets its own uniform buffer
  for (int i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++) {
    render.device().create_buffer(ubos_[i], mems_[i], size,
      vk::BufferUsageFlagBits::eUniformBuffer,
      vk::MemoryPropertyFlagBits::eHostVisible |
      vk::MemoryPropertyFlagBits::eHostCoherent);
    void *data = mems_[i].mapMemory(0, size, {});
    mapped_.push_back(reinterpret_cast<T *>(data));
  }
}

void Renderer::create_descriptor_layout() {
   std::array bindings {
    vk::DescriptorSetLayoutBinding {
      .binding = 0,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eVertex
    },
    vk::DescriptorSetLayoutBinding {
      .binding = 1,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eFragment
    }
  };

  vk::DescriptorSetLayoutCreateInfo info {
    .bindingCount = bindings.size(),
    .pBindings = bindings.data(),
  };

  descriptor_layout = vk::raii::DescriptorSetLayout(device_.device(), info);
}

void Renderer::create_pipeline() {
  // load shader module
  auto shaders = create_shader_module();
  vk::PipelineShaderStageCreateInfo vert_shader {
    .stage = vk::ShaderStageFlagBits::eVertex,
    .module = shaders,
    .pName = "vert_main"
  };

  vk::PipelineShaderStageCreateInfo frag_shader {
    .stage = vk::ShaderStageFlagBits::eFragment,
    .module = shaders,
    .pName = "frag_main"
  };

  vk::PipelineShaderStageCreateInfo shader_stages[] {
    vert_shader, frag_shader
  };

  // describe vertex buffers
  auto bindings = Vertex::getBindingDesc();
  auto attrs = Vertex::getAttrsDescs();
  vk::PipelineVertexInputStateCreateInfo vertex_input_info {
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &bindings,
    .vertexAttributeDescriptionCount = attrs.size(),
    .pVertexAttributeDescriptions = attrs.data()
  };

  // we want our viewport and scissor to be able to dynamically change so we
  // can resize the window (scissor is basically what portion of the viewport
  // is visible)
  std::vector dynamic_states = {
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor
  };

  vk::PipelineDynamicStateCreateInfo dynamic_info {
    .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
    .pDynamicStates = dynamic_states.data()
  };

  vk::PipelineViewportStateCreateInfo viewport_state {
    .viewportCount = 1,
    .scissorCount = 1
  };

  // how do we want our vertices to be assembled
  vk::PipelineInputAssemblyStateCreateInfo input_assembly {
    .topology = vk::PrimitiveTopology::eTriangleList,
  };

  // how do we want to rasterise our geometry
  vk::PipelineRasterizationStateCreateInfo rasteriser {
    .depthClampEnable = false,
    .rasterizerDiscardEnable = false,
    .polygonMode = vk::PolygonMode::eFill,
    .cullMode = vk::CullModeFlagBits::eBack,
    .frontFace = vk::FrontFace::eCounterClockwise,
    .depthBiasEnable = false,
    .depthBiasSlopeFactor = 1.0,
    .lineWidth = 1.0
  };

  // how do we want to treat multisampling
  vk::PipelineMultisampleStateCreateInfo multisampling {
    .rasterizationSamples = vk::SampleCountFlagBits::e1,
    .sampleShadingEnable = false
  };

  // how do we blend colours per swapchain
  vk::PipelineColorBlendAttachmentState color_blend_attachment {
    .blendEnable = false,
    .colorWriteMask = vk::ColorComponentFlagBits::eR
                    | vk::ColorComponentFlagBits::eG
                    | vk::ColorComponentFlagBits::eB
                    | vk::ColorComponentFlagBits::eA
  };

  // how do we blend colours globally
  vk::PipelineColorBlendStateCreateInfo color_blending {
    .logicOpEnable = false,
    .logicOp = vk::LogicOp::eCopy,
    .attachmentCount = 1,
    .pAttachments = &color_blend_attachment
  };

  // what descriptor sets we can provide to the shaders
  vk::PipelineLayoutCreateInfo layout_info {
    .setLayoutCount = 1,
    .pSetLayouts = &*descriptor_layout,
    .pushConstantRangeCount = 0
  };

  pipeline_layout = vk::raii::PipelineLayout(device_.device(), layout_info);

  // depth and stencil info
  vk::PipelineDepthStencilStateCreateInfo depth_info {
    .depthTestEnable = true,
    .depthWriteEnable = true,
    .depthCompareOp = vk::CompareOp::eLess,
    .depthBoundsTestEnable = false,
    .stencilTestEnable = false
  };

  vk::StructureChain pipeline_info {
    vk::GraphicsPipelineCreateInfo {
      .stageCount = 2,
      .pStages = shader_stages,
      .pVertexInputState = &vertex_input_info,
      .pInputAssemblyState = &input_assembly,
      .pViewportState = &viewport_state,
      .pRasterizationState = &rasteriser,
      .pMultisampleState = &multisampling,
      .pDepthStencilState = &depth_info,
      .pColorBlendState = &color_blending,
      .pDynamicState = &dynamic_info,
      .layout = pipeline_layout,
      .renderPass = nullptr
    },
    vk::PipelineRenderingCreateInfo {
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &swapchain_.format(),
      .depthAttachmentFormat = swapchain_.depth_format(),
    }
  };

  pipeline = vk::raii::Pipeline(device_.device(), nullptr, pipeline_info.get());
}

vk::raii::ShaderModule Renderer::create_shader_module() {
  vk::ShaderModuleCreateInfo shader_info {
    .codeSize = shaders_len,
    .pCode = (const uint32_t *) &shaders,
  };

  return vk::raii::ShaderModule(device_.device(), shader_info);
}

void Renderer::create_descriptor_pool(uint32_t descriptor_count) {
  uint32_t count = descriptor_count * Swapchain::MAX_FRAMES_IN_FLIGHT;
  std::array pool_sizes {
    vk::DescriptorPoolSize {
      .type = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = count
    },
    vk::DescriptorPoolSize {
      .type = vk::DescriptorType::eCombinedImageSampler,
      .descriptorCount = count
    }
  };

  vk::DescriptorPoolCreateInfo pool_info {
    .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
    .maxSets = count,
    .poolSizeCount = pool_sizes.size(),
    .pPoolSizes = pool_sizes.data()
  };

  descriptor_pool = vk::raii::DescriptorPool(device_.device(), pool_info);
}

ShaderData Renderer::create_shader_data(Texture &texture) {
  // create uniform buffers
  ShaderData result {
    .texture = texture,
    .uniforms = UniformBuffer<UniformData>(*this)
  };

  std::vector<vk::DescriptorSetLayout> layouts (Swapchain::MAX_FRAMES_IN_FLIGHT,
    descriptor_layout);
  vk::DescriptorSetAllocateInfo alloc_info {
    .descriptorPool = descriptor_pool,
    .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
    .pSetLayouts = layouts.data()
  };

  result.descriptor_sets = device_.device().allocateDescriptorSets(alloc_info);

  // set up each descriptor set to point to the right uniform buffer
  for (int i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++) {
    vk::DescriptorBufferInfo buffer_info {
      .buffer = result.uniforms.ubo(i),
      .offset = 0,
      .range = sizeof(UniformData)
    };

    vk::DescriptorImageInfo image_info {
      .sampler = texture.sampler(),
      .imageView = texture.view(),
      .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };

    std::array write_sets {
      vk::WriteDescriptorSet {
        .dstSet = result.descriptor_sets[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pBufferInfo = &buffer_info
      },
      vk::WriteDescriptorSet {
        .dstSet = result.descriptor_sets[i],
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &image_info
      }
    };

    device_.device().updateDescriptorSets(write_sets, {});
  }

  return result;
}

void Renderer::create_sync_objs() {
  // one render semaphore per imageview
  for (int i = 0; i < swapchain_.image_count(); i++) {
    render_done_sems.emplace_back(device_.device(), vk::SemaphoreCreateInfo {});
  }

  // one presentation semaphore and one draw fence per frame in flight
  for (int i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++) {
    present_done_sems.emplace_back(device_.device(),
      vk::SemaphoreCreateInfo {});
    draw_fences.emplace_back(device_.device(), vk::FenceCreateInfo {
      .flags = vk::FenceCreateFlagBits::eSignaled
    });
  }
}

bool Renderer::begin_frame() {
  auto frame_index = swapchain_.frame_index();

  // wait on the cpu for the frame in flight to be available
  while (vk::Result::eTimeout == device_.device().waitForFences(
    *draw_fences[frame_index], true, UINT64_MAX));

  // acquire the next image and signals the presentation semaphore when its
  // ready to render to
  auto [result, iindex] = swapchain_.swapchain().acquireNextImage(
    UINT64_MAX, *present_done_sems[frame_index], nullptr);
  image_index = iindex;

  // check validity of swapchain
  if (result == vk::Result::eErrorOutOfDateKHR) {
    swapchain_.recreate(window_, device_);
    return false;
  } else if (result != vk::Result::eSuccess &&
      result != vk::Result::eSuboptimalKHR)
    throw std::runtime_error("failed to acquire swapchain!");

  // reset fence for our frame in flight and start drawing
  device_.device().resetFences(*draw_fences[frame_index]);
  command_buffers[frame_index].reset();
  begin_recording(frame_index);
  return true;
}

void Renderer::begin_recording(int frame_index) {
  auto &commands = command_buffers[frame_index];
  commands.begin({});

  // prepare the image buffer for rendering colour to it
  transition_image_layout(
    commands,
    swapchain_.image(image_index),
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eColorAttachmentOptimal,
    {},
    vk::AccessFlagBits2::eColorAttachmentWrite,
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::ImageAspectFlagBits::eColor);

  // prepare the depth buffer too
  transition_image_layout(
    commands,
    swapchain_.depth_image(),
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eDepthAttachmentOptimal,
    vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
    vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
    vk::PipelineStageFlagBits2::eEarlyFragmentTests |
      vk::PipelineStageFlagBits2::eLateFragmentTests,
    vk::PipelineStageFlagBits2::eEarlyFragmentTests |
      vk::PipelineStageFlagBits2::eLateFragmentTests,
    vk::ImageAspectFlagBits::eDepth);

  // rendering settings relating to how to render
  vk::ClearValue clear_color = vk::ClearColorValue { 0.f, 0.f, 0.f, 1.f };
  vk::ClearValue clear_depth = vk::ClearDepthStencilValue { 1.f, 0 };
  vk::RenderingAttachmentInfo color_attachment_info {
    .imageView = swapchain_.image_view(image_index),
    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
    .loadOp = vk::AttachmentLoadOp::eClear,
    .storeOp = vk::AttachmentStoreOp::eStore,
    .clearValue = clear_color
  };
  vk::RenderingAttachmentInfo depth_attachment_info {
    .imageView = swapchain_.depth_view(),
    .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
    .loadOp = vk::AttachmentLoadOp::eClear,
    .storeOp = vk::AttachmentStoreOp::eDontCare,
    .clearValue = clear_depth
  };

  // rendering settings relating to where to render to
  auto extent = swapchain_.extent();
  vk::RenderingInfo rendering_info = {
    .renderArea = {
      .offset = { 0, 0 },
      .extent = extent
    },
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &color_attachment_info,
    .pDepthAttachment = &depth_attachment_info
  };

  // our viewport is just the whole image
  vk::Viewport viewport {
    .x =  0.0f,
    .y = 0.0f,
    .width = static_cast<float>(extent.width),
    .height = static_cast<float>(extent.height),
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };

  // yippee yippee yay yay rendering!
  commands.beginRendering(rendering_info);
  commands.setViewport(0, viewport);
  commands.setScissor(0,
    vk::Rect2D(vk::Offset2D(0, 0), extent));
  commands.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
}

void Renderer::transition_image_layout(
  vk::raii::CommandBuffer &commands,
  const vk::Image &image,
  vk::ImageLayout old_layout,
  vk::ImageLayout new_layout,
  vk::AccessFlags2 src_access,
  vk::AccessFlags2 dst_access,
  vk::PipelineStageFlags2 src_stage,
  vk::PipelineStageFlags2 dst_stage,
  vk::ImageAspectFlags aspect_mask
) {
  vk::ImageMemoryBarrier2 barrier {
    .srcStageMask = src_stage,
    .srcAccessMask = src_access,
    .dstStageMask = dst_stage,
    .dstAccessMask = dst_access,
    .oldLayout = old_layout,
    .newLayout = new_layout,
    .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
    .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
    .image = image,
    .subresourceRange = {
      .aspectMask = aspect_mask,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    }
  };

  vk::DependencyInfo info {
    .dependencyFlags = {},
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers = &barrier
  };

  commands.pipelineBarrier2(info);
}

void Renderer::bind_shader_data(ShaderData &data, UniformData &uniforms) {
  auto frame_index = swapchain_.frame_index();
  auto &commands = command_buffers[frame_index];
  commands.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
    pipeline_layout, 0, *data.descriptor_sets[frame_index], nullptr);
  memcpy(data.uniforms.mapped(frame_index), &uniforms, sizeof(uniforms));
}

void Renderer::end_frame() {
  auto frame_index = swapchain_.frame_index();
  auto &commands = command_buffers[frame_index];

  commands.endRendering();

  // now we want the image buffer to be ready to present
  transition_image_layout(
    commands,
    swapchain_.image(image_index),
    vk::ImageLayout::eColorAttachmentOptimal,
    vk::ImageLayout::ePresentSrcKHR,
    vk::AccessFlagBits2::eColorAttachmentWrite,
    {},
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::PipelineStageFlagBits2::eBottomOfPipe,
    vk::ImageAspectFlagBits::eColor
  );

  commands.end();

  vk::PipelineStageFlags wait_dst_stage {
    vk::PipelineStageFlagBits::eColorAttachmentOutput
  };

  // - submit a command on the queue that:
  //   - waits for the presentation semaphore
  //   - renders to the current buffer in the swapchain
  //   - signals the rendered semaphore
  //   - signals the draw fence
  vk::SubmitInfo submit_info {
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &*present_done_sems[frame_index],
    .pWaitDstStageMask = &wait_dst_stage,
    .commandBufferCount = 1,
    .pCommandBuffers = &*commands,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &*render_done_sems[image_index]
  };

  device_.queue().submit(submit_info, draw_fences[frame_index]);

  try {
    // - submit a command on the queue that:
    //   - waits for the rendered semaphore
    //   - presents the current buffer in the swapchain to the framebuffer
    vk::PresentInfoKHR present_info {
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &*render_done_sems[image_index],
      .swapchainCount = 1,
      .pSwapchains = &*swapchain_.swapchain(),
      .pImageIndices = &image_index
    };

    auto result = device_.queue().presentKHR(present_info);

    // check validity of swapchain
    if (result == vk::Result::eErrorOutOfDateKHR ||
        result == vk::Result::eSuboptimalKHR ||
        window_.has_framebuffer_resized()) {
      swapchain_.recreate(window_, device_);
    } else if (result != vk::Result::eSuccess)
       throw std::runtime_error("could not present to swapchain image");
  } catch (const vk::SystemError &e) {
    if (e.code().value() == static_cast<int>(vk::Result::eErrorOutOfDateKHR))
    {
      swapchain_.recreate(window_, device_);
      return;
    } else throw;
  }

  // next frame in flight
  swapchain_.next_frame();
}
