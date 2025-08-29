/*
 * Copyright (C) 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * Modifications copyright (C) 2025 William Henning
 * Changes: Add virtual framebuffer implementation. Drop non-X wsi's.
 */

#include "swapchain.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#include <unordered_set>
#include <algorithm>

#include "callback_swapchain.h"
#include "present_callback.h"
#include "tinylog.h"

namespace swapchain {
namespace {

// VkExtent2D get_surface_extent(const CallbackSurface& surface) {
//   // Query xcb to get the window's renderable size.
//   // Return that size.
//   xcb_get_geometry_reply_t* g =
//           xcb_get_geometry_reply(surface.connection,
//                                 xcb_get_geometry(surface.connection, surface.window),
//                                 NULL);
//   VkExtent2D extent;
//   extent.width = g->width;
//   extent.height = g->height;
//   free(g);
//   return extent;
// }

}

void RegisterInstance(VkInstance instance, InstanceData& data) {
  uint32_t num_devices = 0;
  data.vkEnumeratePhysicalDevices(instance, &num_devices, nullptr);

  data.physical_devices_.resize(num_devices);
  data.vkEnumeratePhysicalDevices(instance, &num_devices,
                                  data.physical_devices_.data());

  auto physical_device_map = GetGlobalContext().GetPhysicalDeviceMap();

  for (VkPhysicalDevice physical_device : data.physical_devices_) {
    PhysicalDeviceData dat{instance};
    data.vkGetPhysicalDeviceMemoryProperties(physical_device,
                                             &dat.memory_properties_);
    data.vkGetPhysicalDeviceProperties(physical_device,
                                       &dat.physical_device_properties_);
    (*physical_device_map)[physical_device] = dat;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateXlibSurfaceKHR(
    VkInstance instance, const VkXlibSurfaceCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) {
  return CreateSurface(instance,
                       pCreateInfo,
                       VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
                       pAllocator,
                       pSurface);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateXcbSurfaceKHR(
    VkInstance instance, const VkXcbSurfaceCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) {
  return CreateSurface(instance,
                       pCreateInfo,
                       VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
                       pAllocator,
                       pSurface);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSurface(
    VkInstance instance, const void* pCreateInfo, VkStructureType createType,
    const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) {
  const auto instance_dat = *GetGlobalContext().GetInstanceData(instance);

  std::string window_name;
  xcb_window_t window;
  xcb_connection_t* connection;
  VkSurfaceKHR backing_surface;

  if (createType == VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR) {
    const VkXlibSurfaceCreateInfoKHR& info = *(VkXlibSurfaceCreateInfoKHR*)pCreateInfo;
    window = (xcb_window_t)info.window;
    connection = XGetXCBConnection(info.dpy);
    instance_dat.vkCreateXlibSurfaceKHR(instance, (VkXlibSurfaceCreateInfoKHR*)pCreateInfo, pAllocator, &backing_surface);
  }
  if (createType == VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR) {
    const VkXcbSurfaceCreateInfoKHR& info = *(VkXcbSurfaceCreateInfoKHR*)pCreateInfo;
    window = info.window;
    connection = info.connection;
    instance_dat.vkCreateXcbSurfaceKHR(instance, (VkXcbSurfaceCreateInfoKHR*)pCreateInfo, pAllocator, &backing_surface);
  }

  std::ostringstream oss;
  oss << "0x" << std::hex << std::setw(8) << std::setfill('0') << static_cast<std::uint32_t>(window);
  window_name = oss.str();


  *pSurface = reinterpret_cast<VkSurfaceKHR>(
          new CallbackSurface{.window_name=window_name,
                              .window=window,
                              .connection=connection,
                              .backing_surface=backing_surface});
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceSupportKHR(
    VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex,
    VkSurfaceKHR surface, VkBool32* pSupported) {
  const auto instance_dat = *GetGlobalContext().GetInstanceData(
      GetGlobalContext().GetPhysicalDeviceData(physicalDevice)->instance_);

  for (uint32_t i = 0; i <= queueFamilyIndex; ++i) {
    uint32_t property_count = 0;
    instance_dat.vkGetPhysicalDeviceQueueFamilyProperties(
        physicalDevice, &property_count, nullptr);
    assert(property_count > queueFamilyIndex);

    std::vector<VkQueueFamilyProperties> properties(property_count);
    instance_dat.vkGetPhysicalDeviceQueueFamilyProperties(
        physicalDevice, &property_count, properties.data());

    if (properties[queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      *pSupported = (i == queueFamilyIndex);
      return VK_SUCCESS;
    }
  }

  // For now only support the FIRST graphics queue. It looks like all of
  // the commands we will have to run are transfer commands, so
  // we can probably get away with ANY queue (other than
  // SPARSE_BINDING).
  *pSupported = false;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
    VkSurfaceCapabilitiesKHR* pSurfaceCapabilities) {
  const auto instance_dat = *GetGlobalContext().GetInstanceData(
      GetGlobalContext().GetPhysicalDeviceData(physicalDevice)->instance_);

  // It would be illegal for the program to call VkDestroyInstance here.
  // We do not need to lock the map for the whole time, just
  // long enough to get the data out. unordered_map guarantees that
  // even if re-hashing occurs, references remain valid.
  VkPhysicalDeviceProperties& properties =
      GetGlobalContext()
          .GetPhysicalDeviceData(physicalDevice)
          ->physical_device_properties_;
  CallbackSurface& callback_surface = *reinterpret_cast<CallbackSurface*>(surface);
  VkSurfaceKHR backing_surface = callback_surface.backing_surface;
  VkSurfaceCapabilitiesKHR real_cap;
  instance_dat.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, backing_surface, &real_cap);


  pSurfaceCapabilities->minImageCount = 1;
  pSurfaceCapabilities->maxImageCount = 0;
  pSurfaceCapabilities->currentExtent = real_cap.currentExtent;
  pSurfaceCapabilities->minImageExtent = real_cap.minImageExtent;
  pSurfaceCapabilities->maxImageExtent = real_cap.maxImageExtent;
  pSurfaceCapabilities->maxImageArrayLayers =
      properties.limits.maxImageArrayLayers;
  pSurfaceCapabilities->supportedTransforms =
      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  // TODO(awoloszyn): Handle all of the transforms eventually
  pSurfaceCapabilities->currentTransform =
      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  pSurfaceCapabilities->supportedCompositeAlpha =
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  // TODO(awoloszyn): Handle all of the composite types.

  pSurfaceCapabilities->supportedUsageFlags =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  // TODO(awoloszyn): Find a good set of formats that we can use
  // for rendering.

  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilities2KHR(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo,
    VkSurfaceCapabilities2KHR* pSurfaceCapabilities) {
  pSurfaceCapabilities->sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;
  pSurfaceCapabilities->pNext = nullptr;
  return swapchain::vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice,
                                                              pSurfaceInfo->surface,
                                                              &pSurfaceCapabilities->surfaceCapabilities);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceFormatsKHR(
    VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
    uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats) {
  if (!pSurfaceFormats) {
    *pSurfaceFormatCount = 1;
    return VK_SUCCESS;
  }
  if (*pSurfaceFormatCount < 1) {
    return VK_INCOMPLETE;
  }
  *pSurfaceFormatCount = 1;

  // TODO(awoloszyn): Handle more different formats.
  pSurfaceFormats->format = VK_FORMAT_R8G8B8A8_UNORM;
  pSurfaceFormats->colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceFormats2KHR(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo,
    uint32_t* pSurfaceFormatCount, VkSurfaceFormat2KHR* pSurfaceFormats) {
  pSurfaceFormats->sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
  pSurfaceFormats->pNext = nullptr;
  return swapchain::vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice,
                                                         pSurfaceInfo->surface,
                                                         pSurfaceFormatCount,
                                                         &pSurfaceFormats->surfaceFormat);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfacePresentModesKHR(
    VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
    uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes) {
  if (!pPresentModes) {
    *pPresentModeCount = 1;
    return VK_SUCCESS;
  }
  if (*pPresentModeCount < 1) {
    return VK_INCOMPLETE;
  }
  // TODO(awoloszyn): Add more present modes. we MUST support
  // VK_PRESENT_MODE_FIFO_KHR.
  *pPresentModes = VK_PRESENT_MODE_FIFO_KHR;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSwapchainKHR(
    VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain) {
  LOGF(kLogLayer, "Creating swapchain with dimensions: %dx%d\n",
    pCreateInfo->imageExtent.width,
    pCreateInfo->imageExtent.height);

  if (pCreateInfo->oldSwapchain != VK_NULL_HANDLE) {
    CallbackSwapchain& old_swapchain = *reinterpret_cast<CallbackSwapchain*>(pCreateInfo->oldSwapchain);
    std::lock_guard<std::mutex> lock = old_swapchain.GetRetireLock();
    old_swapchain.ClearCallbackAndData();
    LOGF(kLogLayer, "Retiring swapchain: %p\n", &old_swapchain);
  }

  DeviceData& dev_dat = *GetGlobalContext().GetDeviceData(device);
  PhysicalDeviceData& pdd =
      *GetGlobalContext().GetPhysicalDeviceData(dev_dat.physicalDevice);
  InstanceData& inst_dat = *GetGlobalContext().GetInstanceData(pdd.instance_);

  uint32_t property_count = 0;
  inst_dat.vkGetPhysicalDeviceQueueFamilyProperties(dev_dat.physicalDevice,
                                                    &property_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_properties(property_count);
  inst_dat.vkGetPhysicalDeviceQueueFamilyProperties(
      dev_dat.physicalDevice, &property_count, queue_properties.data());

  uint32_t queue = 0;
  for (; queue < static_cast<uint32_t>(queue_properties.size()); ++queue) {
    if (queue_properties[queue].queueFlags & VK_QUEUE_GRAPHICS_BIT) break;
  }

  assert(queue < queue_properties.size());

  CallbackSwapchain* swapchain = new CallbackSwapchain(
      device, queue, &pdd.physical_device_properties_, &pdd.memory_properties_,
      &dev_dat, pCreateInfo, pAllocator);
  *pSwapchain = reinterpret_cast<VkSwapchainKHR>(swapchain);

  VkSurfaceKHR vk_surface = pCreateInfo->surface;
  CallbackSurface& surface = *reinterpret_cast<CallbackSurface*>(vk_surface);
  const uint32_t w = pCreateInfo->imageExtent.width;
  const uint32_t h = pCreateInfo->imageExtent.height;
  const VkCompositeAlphaFlagBitsKHR composite_mode = pCreateInfo->compositeAlpha;
  generic_unique_ptr present_data =
      make_generic_unique(new SwapchainData(w, h, surface.window_name, composite_mode));
  swapchain->SetCallback(present_callback, std::move(present_data));

  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL
vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain,
                      const VkAllocationCallbacks* pAllocator) {
  CallbackSwapchain* swp = reinterpret_cast<CallbackSwapchain*>(swapchain);

  std::vector<VkImage> swp_images = swp->AllImages();
  std::vector<VkImage>& all_swp_images = *GetGlobalContext().SwapchainImages(device);
  std::unordered_set<VkImage> to_remove(swp_images.begin(), swp_images.end());
  all_swp_images.erase(
    std::remove_if(all_swp_images.begin(), all_swp_images.end(), [&](VkImage img) {
      return to_remove.count(img) > 0;
    }), all_swp_images.end());

  swp->Destroy(pAllocator);
  delete swp;
}

VKAPI_ATTR void VKAPI_CALL
vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface,
                    const VkAllocationCallbacks* pAllocator) {
  CallbackSurface* callback_surface = reinterpret_cast<CallbackSurface*>(surface);
  delete callback_surface;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainImagesKHR(
    VkDevice device, VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount,
    VkImage* pSwapchainImages) {
  CallbackSwapchain* swp = reinterpret_cast<CallbackSwapchain*>(swapchain);
  const auto images =
      swp->GetImages(*pSwapchainImageCount, pSwapchainImages != nullptr);
  if (!pSwapchainImages) {
    *pSwapchainImageCount = static_cast<uint32_t>(images.size());
    return VK_SUCCESS;
  }

  VkResult res = VK_INCOMPLETE;
  if (*pSwapchainImageCount >= images.size()) {
    *pSwapchainImageCount = static_cast<uint32_t>(images.size());
    res = VK_SUCCESS;
  }

  for (size_t i = 0; i < *pSwapchainImageCount; ++i) {
    pSwapchainImages[i] = images[i];
  }

  return res;
}


// We actually have to be able to submit data to the Queue right now.
// The user can supply either a semaphore, or a fence or both to this function.
// Because of this, once the image is available we have to submit
// a command to the queue to signal these.
VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImageKHR(
    VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout,
    VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex) {
  CallbackSwapchain* swp = reinterpret_cast<CallbackSwapchain*>(swapchain);
  if (!swp->GetImage(timeout, pImageIndex)) {
    return timeout == 0 ? VK_NOT_READY : VK_TIMEOUT;
  }

  // It is important that we do not keep the lock here.
  // *GetGlobalContext().GetDeviceData() only holds the lock
  // for the duration of the call, if we instead do something like
  // auto dat = GetGlobalContext().GetDeviceData(device),
  // then the lock will be let go when dat is destroyed, which is
  // AFTER swapchain::vkQueueSubmit, this would be a priority
  // inversion on the locks.
  DeviceData& dat = *GetGlobalContext().GetDeviceData(device);
  VkQueue q;

  dat.vkGetDeviceQueue(device, swp->DeviceQueue(), 0, &q);

  bool has_semaphore = semaphore != VK_NULL_HANDLE;

  VkSubmitInfo info{VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
                    nullptr,                        // pNext
                    0,                              // waitSemaphoreCount
                    nullptr,                        // waitSemaphores
                    nullptr,                        // waitDstStageMask
                    0,                              // commandBufferCount
                    nullptr,                        // pCommandBuffers
                    (has_semaphore ? 1u : 0u),      // waitSemaphoreCount
                    (has_semaphore ? &semaphore : nullptr)};
  VkResult ret = swapchain::vkQueueSubmit(q, 1, &info, fence);
  return ret;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo) {
  // We submit to the queue the commands set up by the callback swapchain.
  // This will start a copy operation from the image to the swapchain
  // buffers.
  uint32_t res = VK_SUCCESS;
  std::vector<VkPipelineStageFlags> pipeline_stages(
      pPresentInfo->waitSemaphoreCount, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
  for (size_t i = 0; i < pPresentInfo->swapchainCount; ++i) {
    uint32_t image_index = pPresentInfo->pImageIndices[i];
    CallbackSwapchain* swp =
        reinterpret_cast<CallbackSwapchain*>(pPresentInfo->pSwapchains[i]);

    VkSubmitInfo submitInfo{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,                     // sType
        nullptr,                                           // nullptr
        i == 0 ? pPresentInfo->waitSemaphoreCount : 0,     // waitSemaphoreCount
        i == 0 ? pPresentInfo->pWaitSemaphores : nullptr,  // pWaitSemaphores
        i == 0 ? pipeline_stages.data() : nullptr,         // pWaitDstStageMask
        1,                                                 // commandBufferCount
        &swp->GetCommandBuffer(image_index),               // pCommandBuffers
        0,                                                 // semaphoreCount
        nullptr                                            // pSemaphores
    };

    res |= GetGlobalContext().GetQueueData(queue)->vkQueueSubmit(
        queue, 1, &submitInfo, swp->GetFence(image_index));
    swp->NotifySubmitted(image_index);
  }

  return VkResult(res);
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(VkQueue queue,
                                             uint32_t submitCount,
                                             const VkSubmitInfo* pSubmits,
                                             VkFence fence) {
  // We actually DO have to lock here, we may share this queue with
  // vkAcquireNextImageKHR, which is not externally synchronized on Queue.
  return GetGlobalContext().GetQueueData(queue)->vkQueueSubmit(
      queue, submitCount, pSubmits, fence);
}

// The following 3 functions are special. We would normally not have to
// handle them, but since we cannot rely on there being an internal swapchain
// mechanism, we cannot allow VK_IMAGE_LAYOUT_PRESENT_SRC_KHR to be passed
// to the driver. In this case any time a user uses a layout that is
// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR we replace that with
// VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, which is what we need an image to be
// set up as when we have to copy anyway.
VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier(
    VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
    uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
    uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers,
    uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier* pImageMemoryBarriers) {
  std::vector<VkImageMemoryBarrier> imageBarriers(imageMemoryBarrierCount);
  for (size_t i = 0; i < imageMemoryBarrierCount; ++i) {
    imageBarriers[i] = pImageMemoryBarriers[i];
    if (imageBarriers[i].oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
      imageBarriers[i].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      imageBarriers[i].srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
    }
    if (imageBarriers[i].newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
      imageBarriers[i].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      imageBarriers[i].dstAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
    }
  }
  PFN_vkCmdPipelineBarrier func = GetGlobalContext()
                                      .GetCommandBufferData(commandBuffer)
                                      ->vkCmdPipelineBarrier;

  return func(commandBuffer, srcStageMask, dstStageMask, dependencyFlags,
              memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount,
              pBufferMemoryBarriers, imageMemoryBarrierCount,
              imageBarriers.data());
}

VKAPI_ATTR void VKAPI_CALL vkCmdWaitEvents(
    VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent* pEvents,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
    uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers,
    uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier* pImageMemoryBarriers) {
  std::vector<VkImageMemoryBarrier> imageBarriers(imageMemoryBarrierCount);
  for (size_t i = 0; i < imageMemoryBarrierCount; ++i) {
    imageBarriers[i] = pImageMemoryBarriers[i];
    if (imageBarriers[i].oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
      imageBarriers[i].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      imageBarriers[i].srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
    }
    if (imageBarriers[i].newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
      imageBarriers[i].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      imageBarriers[i].dstAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
    }
  }
  PFN_vkCmdWaitEvents func =
      GetGlobalContext().GetCommandBufferData(commandBuffer)->vkCmdWaitEvents;

  func(commandBuffer, eventCount, pEvents, srcStageMask, dstStageMask,
       memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount,
       pBufferMemoryBarriers, imageMemoryBarrierCount, imageBarriers.data());
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateRenderPass(
    VkDevice device, const VkRenderPassCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass) {
  VkRenderPassCreateInfo intercepted = *pCreateInfo;
  std::vector<VkAttachmentDescription> attachments(
      pCreateInfo->attachmentCount);
  intercepted.pAttachments = attachments.data();

  for (size_t i = 0; i < pCreateInfo->attachmentCount; ++i) {
    attachments[i] = pCreateInfo->pAttachments[i];
    if (attachments[i].initialLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
      attachments[i].initialLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    }
    if (attachments[i].finalLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
      attachments[i].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    }
  }
  PFN_vkCreateRenderPass func =
      GetGlobalContext().GetDeviceData(device)->vkCreateRenderPass;

  return func(device, &intercepted, pAllocator, pRenderPass);
}
}  // namespace swapchain
