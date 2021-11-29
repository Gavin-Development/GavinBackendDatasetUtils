#pragma once
#include <vulkan.h>
#include <vector>
#include <iostream>

struct VulkanContext {
	VkInstance instance = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue queue = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	size_t requiredVram = 0;
};

bool setupVulkanEnvironment(VulkanContext* pVulkanStuff);