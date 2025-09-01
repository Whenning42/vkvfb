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
 * 
 * 
 * Modifications copyright (C) 2025 William Henning
 * Changes: See header.
 */

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan.h>
#include <X11/Xlib.h>
#include <xcb/xcb.h>
#include <vulkan/vulkan_xlib.h>
#include <vulkan/vulkan_xcb.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "logger.h"
#include "swapchain.h"

#define LAYER_NAME "CallbackSwapchain"

#define LAYER_NAME_FUNCTION(fn) CallbackSwapchain##fn

#if defined(_WIN32)
#define CALLBACK_LAYER_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#define CALLBACK_LAYER_EXPORT __attribute__((visibility("default")))
#else
#define CALLBACK_LAYER_EXPORT
#endif

namespace swapchain {
namespace {
bool layer_disabled() {
  bool enabled = false;
  const char* enable_env = std::getenv("VKVFB");
  const char* disable_env = std::getenv("VKVFB_DISABLE");

  if (enable_env && std::string(enable_env) == "1") {
    enabled = true;
  }
  if (disable_env && std::string(disable_env) == "1") {
    enabled = false;
  }
  return !enabled;
}

bool swapchain_disabled() {
  const char* disable_env = std::getenv("DISABLE_SWAPCHAIN");
  return disable_env && std::string(disable_env) == "1";
}
}

Context& GetGlobalContext() {
  // To avoid bad cleanup, we never free this.
  // If we don't do this, we could race in multithreaded applications.
  static Context* kContext = new Context();
  return *kContext;
}

namespace {

template <typename T>
struct link_info_traits {
  const static bool is_instance =
      std::is_same<T, const VkInstanceCreateInfo>::value;
  using layer_info_type =
      typename std::conditional<is_instance, VkLayerInstanceCreateInfo,
                                VkLayerDeviceCreateInfo>::type;
  const static VkStructureType sType =
      is_instance ? VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO
                  : VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO;
};

// Get layer_specific data for this layer.
// Will return either VkLayerInstanceCreateInfo or
// VkLayerDeviceCreateInfo depending on the type of the pCreateInfo
// passed in.
template <typename T>
typename link_info_traits<T>::layer_info_type* get_layer_link_info(
    T* pCreateInfo) {
  using layer_info_type = typename link_info_traits<T>::layer_info_type;

  auto layer_info = const_cast<layer_info_type*>(
      static_cast<const layer_info_type*>(pCreateInfo->pNext));

  while (layer_info) {
    if (layer_info->sType == link_info_traits<T>::sType &&
        layer_info->function == VK_LAYER_LINK_INFO) {
      return layer_info;
    }
    layer_info = const_cast<layer_info_type*>(
        static_cast<const layer_info_type*>(layer_info->pNext));
  }
  return layer_info;
}
}  // namespace

// Overload vkCreateInstance. It is all book-keeping
// and passthrough to the next layer (or ICD) in the chain.
VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkInstance* pInstance) {
  if (!layer_disabled()) {
    std::cout << "Vkvfb loaded." << std::endl;
  } else {
    std::cout << "Vkvfb disabled by env vars." << std::endl;
  }

  VkLayerInstanceCreateInfo* layer_info = get_layer_link_info(pCreateInfo);

  // Grab the pointer to the next vkGetInstanceProcAddr in the chain.
  PFN_vkGetInstanceProcAddr get_instance_proc_addr =
      layer_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;

  // From that get the next vkCreateInstance function.
  PFN_vkCreateInstance create_instance = reinterpret_cast<PFN_vkCreateInstance>(
      get_instance_proc_addr(NULL, "vkCreateInstance"));

  if (create_instance == NULL) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  // The next layer may read from layer_info,
  // so advance the pointer for it.
  layer_info->u.pLayerInfo = layer_info->u.pLayerInfo->pNext;

  // Call vkCreateInstance, and keep track of the result.
  VkResult result = create_instance(pCreateInfo, pAllocator, pInstance);
  if (result != VK_SUCCESS) return result;

  InstanceData data;

#define GET_PROC(name) \
  data.name =          \
      reinterpret_cast<PFN_##name>(get_instance_proc_addr(*pInstance, #name))
  GET_PROC(vkGetInstanceProcAddr);
  GET_PROC(vkDestroyInstance);
  GET_PROC(vkEnumeratePhysicalDevices);
  GET_PROC(vkEnumerateDeviceExtensionProperties);
  GET_PROC(vkCreateDevice);
  GET_PROC(vkGetPhysicalDeviceQueueFamilyProperties);
  GET_PROC(vkGetPhysicalDeviceProperties);
  GET_PROC(vkGetPhysicalDeviceMemoryProperties);
  GET_PROC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
  GET_PROC(vkCreateXlibSurfaceKHR);
  GET_PROC(vkCreateXcbSurfaceKHR);

#undef GET_PROC
  // Add this instance, along with the vkGetInstanceProcAddr to our
  // map. This way when someone calls vkGetInstanceProcAddr, we can forward
  // it to the correct "next" vkGetInstanceProcAddr.
  {
    auto instances = GetGlobalContext().GetInstanceMap();
    // The same instance was returned twice, this is a problem.
    if (instances->find(*pInstance) != instances->end()) {
      return VK_ERROR_INITIALIZATION_FAILED;
    }
    (*instances)[*pInstance] = data;
  }

  RegisterInstance(*pInstance,
                   (*GetGlobalContext().GetInstanceMap())[*pInstance]);
  return result;
}

// On vkDestroyInstance, printf("VkDestroyInstance") and clean up our
// tracking data.
VKAPI_ATTR void vkDestroyInstance(VkInstance instance,
                                  const VkAllocationCallbacks* pAllocator) {
  // First we have to find the function to chain to, then we have to
  // remove this instance from our list, then we forward the call.
  auto instance_map = GetGlobalContext().GetInstanceMap();
  auto it = instance_map->find(instance);
  it->second.vkDestroyInstance(instance, pAllocator);
  instance_map->erase(it);
}

// Overload vkCreateDevice. It is all book-keeping
// and passthrough to the next layer (or ICD) in the chain.
VKAPI_ATTR VkResult VKAPI_CALL
vkCreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo,
               const VkAllocationCallbacks* pAllocator, VkDevice* pDevice) {
  VkLayerDeviceCreateInfo* layer_info = get_layer_link_info(pCreateInfo);

  if (pCreateInfo->ppEnabledExtensionNames) {
    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i) {
      LOG(kLogLayer, "Vkvfb running w/ enabled extension: %s\n", pCreateInfo->ppEnabledExtensionNames[i]);
    }
  }

  // Grab the fpGetInstanceProcAddr from the layer_info. We will get
  // vkCreateDevice from this.
  // Note: we cannot use our instance_map because we do not have a
  // vkInstance here.
  PFN_vkGetInstanceProcAddr get_instance_proc_addr =
      layer_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;

  PFN_vkCreateDevice create_device = reinterpret_cast<PFN_vkCreateDevice>(
      get_instance_proc_addr(NULL, "vkCreateDevice"));
  if (!create_device) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  // We want to store off the next vkGetDeviceProcAddr so keep track of it now
  // before we advance the pointer.
  PFN_vkGetDeviceProcAddr get_device_proc_addr =
      layer_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;

  // The next layer may read from layer_info, so advance the pointer for it.
  layer_info->u.pLayerInfo = layer_info->u.pLayerInfo->pNext;

  VkResult result = create_device(gpu, pCreateInfo, pAllocator, pDevice);
  if (result != VK_SUCCESS) {
    return result;
  }

  DeviceData data{gpu};

#define GET_PROC(name) \
  data.name =          \
      reinterpret_cast<PFN_##name>(get_device_proc_addr(*pDevice, #name));

  GET_PROC(vkGetDeviceProcAddr);
  GET_PROC(vkGetDeviceQueue);

  GET_PROC(vkAllocateMemory);
  GET_PROC(vkFreeMemory);
  GET_PROC(vkMapMemory);
  GET_PROC(vkUnmapMemory);
  GET_PROC(vkInvalidateMappedMemoryRanges);

  GET_PROC(vkCreateFence);
  GET_PROC(vkGetFenceStatus);
  GET_PROC(vkWaitForFences);
  GET_PROC(vkDestroyFence);
  GET_PROC(vkResetFences);

  GET_PROC(vkCreateImage);
  GET_PROC(vkGetImageMemoryRequirements);
  GET_PROC(vkBindImageMemory);
  GET_PROC(vkDestroyImage);
  GET_PROC(vkCreateImageView);

  GET_PROC(vkCreateBuffer);
  GET_PROC(vkGetBufferMemoryRequirements);
  GET_PROC(vkBindBufferMemory);
  GET_PROC(vkDestroyBuffer);

  GET_PROC(vkCreateCommandPool);
  GET_PROC(vkDestroyCommandPool);
  GET_PROC(vkAllocateCommandBuffers);
  GET_PROC(vkFreeCommandBuffers);

  GET_PROC(vkBeginCommandBuffer);
  GET_PROC(vkEndCommandBuffer);

  GET_PROC(vkCmdCopyImageToBuffer);
  GET_PROC(vkCmdPipelineBarrier);
  GET_PROC(vkCmdWaitEvents);
  GET_PROC(vkCreateRenderPass);

  GET_PROC(vkQueueSubmit);
  GET_PROC(vkDestroyDevice);

#undef GET_PROC

  // Add this device, along with the vkGetDeviceProcAddr to our map.
  // This way when someone calls vkGetDeviceProcAddr, we can forward
  // it to the correct "next" vkGetDeviceProcAddr.
  {
    auto device_map = GetGlobalContext().GetDeviceMap();
    if (device_map->find(*pDevice) != device_map->end()) {
      return VK_ERROR_INITIALIZATION_FAILED;
    }
    (*device_map)[*pDevice] = data;
  }

  {
    auto queue_map = GetGlobalContext().GetQueueMap();
    for (size_t i = 0; i < pCreateInfo->queueCreateInfoCount; ++i) {
      auto queue_family_index =
          pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex;
      for (uint32_t j = 0; j < pCreateInfo->pQueueCreateInfos[i].queueCount;
           ++j) {
        VkQueue q;
        data.vkGetDeviceQueue(*pDevice, queue_family_index, j, &q);
        (*queue_map)[q] = {*pDevice, data.vkQueueSubmit};
      }
    }
  }

  {
    auto swp_map = GetGlobalContext().GetSwapchainImageMap();
    (*swp_map)[*pDevice] = {};
  }

  LOG(kLogLayer, "Created device: %p\n", *pDevice);
  return result;
}

// On vkDestroyDevice, clean up our tracking data.
VKAPI_ATTR void vkDestroyDevice(VkDevice device,
                                const VkAllocationCallbacks* pAllocator) {
  // First we have to find the function to chain to, then we have to
  // remove this instance from our list, then we forward the call.
  auto device_map = GetGlobalContext().GetDeviceMap();
  auto it = device_map->find(device);

  it->second.vkDestroyDevice(device, pAllocator);
  device_map->erase(it);
}

static const VkLayerProperties global_layer_properties[] = {{
    LAYER_NAME,
    VK_VERSION_MAJOR(1) | VK_VERSION_MINOR(0) | 5,
    1,
    "Callback Swapchain Layer",
}};

VkResult get_layer_properties(uint32_t* pPropertyCount,
                              VkLayerProperties* pProperties) {
  if (pProperties == NULL) {
    *pPropertyCount = 1;
    return VK_SUCCESS;
  }

  if (pPropertyCount == 0) {
    return VK_INCOMPLETE;
  }
  *pPropertyCount = 1;
  memcpy(pProperties, global_layer_properties, sizeof(global_layer_properties));
  return VK_SUCCESS;
}

CALLBACK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceLayerProperties(uint32_t* pPropertyCount,
                                   VkLayerProperties* pProperties) {
  return get_layer_properties(pPropertyCount, pProperties);
}

CALLBACK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateDeviceLayerProperties(VkPhysicalDevice, uint32_t* pPropertyCount,
                                 VkLayerProperties* pProperties) {
  return get_layer_properties(pPropertyCount, pProperties);
}

CALLBACK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceExtensionProperties(const char* /*pLayerName*/,
                                       uint32_t* pPropertyCount,
                                       VkExtensionProperties* /*pProperties*/) {
  *pPropertyCount = 0;
  return VK_SUCCESS;
}

// Overload EnumeratePhysicalDevices, this is entirely for
// book-keeping.
VKAPI_ATTR VkResult VKAPI_CALL
vkEnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount,
                           VkPhysicalDevice* pPhysicalDevices) {
  auto instance_data = GetGlobalContext().GetInstanceData(instance);
  if (instance_data->physical_devices_.empty()) {
    uint32_t count;
    VkResult res =
        instance_data->vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (res != VK_SUCCESS) {
      return res;
    }
    instance_data->physical_devices_.resize(count);
    if (VK_SUCCESS !=
        (res = instance_data->vkEnumeratePhysicalDevices(
             instance, &count, instance_data->physical_devices_.data()))) {
      instance_data->physical_devices_.clear();
      return res;
    }
  }

  uint32_t count =
      static_cast<uint32_t>(instance_data->physical_devices_.size());
  if (pPhysicalDevices) {
    if (*pPhysicalDeviceCount > count) *pPhysicalDeviceCount = count;
    memcpy(pPhysicalDevices, instance_data->physical_devices_.data(),
           *pPhysicalDeviceCount * sizeof(VkPhysicalDevice));
  } else {
    *pPhysicalDeviceCount = count;
  }
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(
    VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo,
    VkCommandBuffer* pCommandBuffers) {
  auto command_buffer_map = GetGlobalContext().GetCommandBufferMap();
  const auto device_data = GetGlobalContext().GetDeviceData(device);

  VkResult res = device_data->vkAllocateCommandBuffers(device, pAllocateInfo,
                                                       pCommandBuffers);
  if (res == VK_SUCCESS) {
    for (size_t i = 0; i < pAllocateInfo->commandBufferCount; ++i) {
      (*command_buffer_map)[pCommandBuffers[i]] = {
          device, device_data->vkCmdPipelineBarrier,
          device_data->vkCmdWaitEvents};
    }
  }
  return res;
}

// Overload vkEnumerateDeviceExtensionProperties
VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(
    VkPhysicalDevice physicalDevice, const char* pLayerName,
    uint32_t* pPropertyCount, VkExtensionProperties* pProperties) {
  if (!physicalDevice) {
    *pPropertyCount = 0;
    return VK_SUCCESS;
  }

  auto instance_data = GetGlobalContext().GetInstanceData(
      GetGlobalContext().GetPhysicalDeviceData(physicalDevice)->instance_);

  return instance_data->vkEnumerateDeviceExtensionProperties(
      physicalDevice, pLayerName, pPropertyCount, pProperties);
}

void vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool,
                          uint32_t commandBufferCount,
                          const VkCommandBuffer* pCommandBuffers) {
  auto command_buffer_map = GetGlobalContext().GetCommandBufferMap();
  for (size_t i = 0; i < commandBufferCount; ++i) {
    command_buffer_map->erase(pCommandBuffers[i]);
  }
  GetGlobalContext().GetDeviceData(device)->vkFreeCommandBuffers(
      device, commandPool, commandBufferCount, pCommandBuffers);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateImageView(VkDevice device,
                                                 const VkImageViewCreateInfo* pCreateInfo,
                                                 const VkAllocationCallbacks* pAllocator,
                                                 VkImageView* pView) {
  // Override base_mip_level to 0 for swapchain images as a workaround for factorio
  // passing in baseMipLevel = 1 for some swapchain image views.
  VkImageViewCreateInfo info = *pCreateInfo;

  VkImage image = pCreateInfo->image;
  const std::vector<VkImage>& swp_images = *GetGlobalContext().SwapchainImages(device);
  if (std::find(swp_images.begin(), swp_images.end(), image) != swp_images.end()) {
    uint32_t& base_mip_level = info.subresourceRange.baseMipLevel;
    if (base_mip_level == 1) {
      LOG(kLogLayer, "Overriding image basemiplevel to 0.\n");
      base_mip_level = 0;
    }
  }
  
  return GetGlobalContext().GetDeviceData(device)->vkCreateImageView(
      device, &info, pAllocator, pView);
}

// Overload GetInstanceProcAddr.
// It also provides the overloaded function for vkCreateDevice. This way we can
// also hook vkGetDeviceProcAddr.
// Lastly it provides vkDestroyInstance for book-keeping purposes.
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetInstanceProcAddr(VkInstance instance, const char* funcName) {
  if (layer_disabled()) {
    PFN_vkGetInstanceProcAddr instance_proc_addr =
        GetGlobalContext().GetInstanceData(instance)->vkGetInstanceProcAddr;
    return instance_proc_addr(instance, funcName);
  }

#define INTERCEPT(func)         \
  if (!strcmp(funcName, #func)) \
  return reinterpret_cast<PFN_vkVoidFunction>(func)

  INTERCEPT(vkGetInstanceProcAddr);

  INTERCEPT(vkCreateDevice);
  INTERCEPT(vkCreateInstance);
  INTERCEPT(vkDestroyInstance);
  INTERCEPT(vkEnumerateDeviceExtensionProperties);
  INTERCEPT(vkEnumerateDeviceLayerProperties);
  INTERCEPT(vkEnumerateInstanceExtensionProperties);
  INTERCEPT(vkEnumerateInstanceLayerProperties);
  INTERCEPT(vkEnumeratePhysicalDevices);

  // From here on down these are what is needed for
  // swapchain/surface support.
  if (!swapchain_disabled()) {
    INTERCEPT(vkDestroySurfaceKHR);

    INTERCEPT(vkGetPhysicalDeviceSurfaceSupportKHR);
    INTERCEPT(vkGetPhysicalDeviceSurfaceFormatsKHR);
    INTERCEPT(vkGetPhysicalDeviceSurfaceFormats2KHR);
    INTERCEPT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    INTERCEPT(vkGetPhysicalDeviceSurfaceCapabilities2KHR);
    INTERCEPT(vkGetPhysicalDeviceSurfacePresentModesKHR);

    // From here down it is just functions that have to be overriden for swapchain
    INTERCEPT(vkQueuePresentKHR);
    INTERCEPT(vkQueueSubmit);
    INTERCEPT(vkCmdPipelineBarrier);
    INTERCEPT(vkCmdWaitEvents);
    INTERCEPT(vkCreateRenderPass);

    INTERCEPT(vkCreateSwapchainKHR);
    INTERCEPT(vkDestroySwapchainKHR);
    INTERCEPT(vkGetSwapchainImagesKHR);
    INTERCEPT(vkAcquireNextImageKHR);

    INTERCEPT(vkAllocateCommandBuffers);
    INTERCEPT(vkFreeCommandBuffers);

    INTERCEPT(vkCreateXcbSurfaceKHR);
    INTERCEPT(vkCreateXlibSurfaceKHR);
  }
#undef INTERCEPT

  // If we are calling a non-overloaded function then we have to
  // return the "next" in the chain. On vkCreateInstance we stored this in
  // the map so we can call it here.
  PFN_vkGetInstanceProcAddr instance_proc_addr =
      GetGlobalContext().GetInstanceData(instance)->vkGetInstanceProcAddr;

  return instance_proc_addr(instance, funcName);
}

// Overload GetDeviceProcAddr.
// We provide an overload of vkDestroyDevice for book-keeping.
// The rest of the overloads are swapchain-specific.
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetDeviceProcAddr(VkDevice dev, const char* funcName) {
  if (layer_disabled()) {
    PFN_vkGetDeviceProcAddr device_proc_addr =
        GetGlobalContext().GetDeviceData(dev)->vkGetDeviceProcAddr;
    return device_proc_addr(dev, funcName);
  }

#define INTERCEPT(func)         \
  if (!strcmp(funcName, #func)) \
  return reinterpret_cast<PFN_vkVoidFunction>(func)

  INTERCEPT(vkGetDeviceProcAddr);
  INTERCEPT(vkDestroyDevice);
  INTERCEPT(vkCreateImageView);

  if (!swapchain_disabled()) {
    // From here down it is just functions that have to be overriden for swapchain
    INTERCEPT(vkQueuePresentKHR);
    INTERCEPT(vkQueueSubmit);
    INTERCEPT(vkCmdPipelineBarrier);
    INTERCEPT(vkCmdWaitEvents);
    INTERCEPT(vkCreateRenderPass);

    INTERCEPT(vkCreateSwapchainKHR);
    INTERCEPT(vkDestroySwapchainKHR);
    INTERCEPT(vkGetSwapchainImagesKHR);
    INTERCEPT(vkAcquireNextImageKHR);

    INTERCEPT(vkAllocateCommandBuffers);
    INTERCEPT(vkFreeCommandBuffers);
  }
#undef INTERCEPT

  // If we are calling a non-overloaded function then we have to
  // return the "next" in the chain. On vkCreateDevice we stored this in the
  // map so we can call it here.

  PFN_vkGetDeviceProcAddr device_proc_addr =
      GetGlobalContext().GetDeviceData(dev)->vkGetDeviceProcAddr;
  return device_proc_addr(dev, funcName);
}
}  // namespace swapchain

extern "C" {

// For this to function on Android the entry-point names for GetDeviceProcAddr
// and GetInstanceProcAddr must be ${layer_name}/Get*ProcAddr.
// This is a bit surprising given that we *MUST* also export
// vkEnumerate*Layers without any prefix.
CALLBACK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
LAYER_NAME_FUNCTION(GetDeviceProcAddr)(VkDevice dev, const char* funcName) {
  return swapchain::vkGetDeviceProcAddr(dev, funcName);
}

CALLBACK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
LAYER_NAME_FUNCTION(GetInstanceProcAddr)(VkInstance instance,
                                         const char* funcName) {
  return swapchain::vkGetInstanceProcAddr(instance, funcName);
}

// Documentation is sparse for Android, looking at libvulkan.so
// These 4 function must be defined in order for this to even
// be considered for loading.
#if defined(__ANDROID__)
CALLBACK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceLayerProperties(uint32_t* pPropertyCount,
                                   VkLayerProperties* pProperties) {
  return swapchain::vkEnumerateInstanceLayerProperties(pPropertyCount,
                                                       pProperties);
}

// On Android this must also be defined, even if we have 0
// layers to expose.
CALLBACK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceExtensionProperties(const char* pLayerName,
                                       uint32_t* pPropertyCount,
                                       VkExtensionProperties* pProperties) {
  return swapchain::vkEnumerateInstanceExtensionProperties(
      pLayerName, pPropertyCount, pProperties);
}

CALLBACK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice,
                                 uint32_t* pPropertyCount,
                                 VkLayerProperties* pProperties) {
  return swapchain::vkEnumerateDeviceLayerProperties(
      physicalDevice, pPropertyCount, pProperties);
}

// On Android this must also be defined, even if we have 0
// layers to expose.
CALLBACK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                     const char* pLayerName,
                                     uint32_t* pPropertyCount,
                                     VkExtensionProperties* pProperties) {
  return swapchain::vkEnumerateDeviceExtensionProperties(
      physicalDevice, pLayerName, pPropertyCount, pProperties);
}
#endif
}
