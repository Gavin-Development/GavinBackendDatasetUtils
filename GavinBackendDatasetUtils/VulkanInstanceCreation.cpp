#include "VulkanStuff.hpp"

bool setupVulkanEnvironment(VulkanContext* pVulkanStuff) {
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "GavinBackendDatasetUtils";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pEngineName = "None";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	if (vkCreateInstance(&createInfo, nullptr, &pVulkanStuff->instance) != VK_SUCCESS) {
		std::cout << "Unable to create vulkan instance. Features accelerated by vulkan may be unusable." << std::endl;
		return false;
	}

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	for (const auto& extension : extensions) {
		std::cout << extension.extensionName << std::endl;
	}

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(pVulkanStuff->instance, &deviceCount, nullptr);
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(pVulkanStuff->instance, &deviceCount, devices.data());

	std::cout << "There are " << deviceCount << " devices that support vulkan available." << std::endl;

	size_t DeviceVram = 0;
	for (auto device : devices) {
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		VkPhysicalDeviceMemoryProperties deviceMemoryProperties;

		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
		vkGetPhysicalDeviceMemoryProperties(device, &deviceMemoryProperties);

		// Ensure the GPU reaches some basic minimumn requirements to be useful for compute workloads.
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && 
			deviceFeatures.shaderUniformBufferArrayDynamicIndexing && 
			deviceFeatures.shaderStorageBufferArrayDynamicIndexing && 
			deviceFeatures.shaderInt64 && 
			deviceMemoryProperties.memoryHeaps[0].size >= pVulkanStuff->requiredVram &&
			deviceMemoryProperties.memoryHeaps[0].size > DeviceVram) {

			DeviceVram = deviceProperties.limits.maxComputeSharedMemorySize;
			pVulkanStuff->physicalDevice = device;

			std::cout << "Suitable device found:\n" << "\t" << deviceProperties.deviceName << "\n\t" << (float)deviceMemoryProperties.memoryHeaps[0].size / 1074000000 << " GB" << std::endl;

		}

	}

	if (pVulkanStuff->device == VK_NULL_HANDLE) {
		std::cout << "No devices were found that meet the requirements of this application. Some functionality may be unavailable or not accelerated." << std::endl;
		return false;
	}
	return true;

}