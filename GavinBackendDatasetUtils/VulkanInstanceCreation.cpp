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
		std::vector<VkQueueFamilyProperties> deviceQueueFamilies;
		uint32_t queueFamilyCount = 0;
		uint32_t queueIndex = 0;
		bool requiredDeviceQueueFamiliesPresent = false;



		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
		vkGetPhysicalDeviceMemoryProperties(device, &deviceMemoryProperties);
		

		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		deviceQueueFamilies.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, deviceQueueFamilies.data());



		for (auto queueFamily : deviceQueueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT & VK_QUEUE_TRANSFER_BIT) {
				requiredDeviceQueueFamiliesPresent = true;
				break;
			}
			queueIndex++;
		}



		// Ensure the GPU reaches some basic minimumn requirements to be useful for compute workloads.
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && 
			deviceFeatures.shaderUniformBufferArrayDynamicIndexing && 
			deviceFeatures.shaderStorageBufferArrayDynamicIndexing && 
			deviceFeatures.shaderInt64 && 
			deviceMemoryProperties.memoryHeaps[0].size >= pVulkanStuff->requiredVram &&
			deviceMemoryProperties.memoryHeaps[0].size > DeviceVram) {

			// create the queue.
			float queuePriority = 1.0f;
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueIndex;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			VkPhysicalDeviceFeatures deviceFeatures{};

			VkDeviceCreateInfo deviceCreateInfo{};
			deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
			deviceCreateInfo.queueCreateInfoCount = 1;
			deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
			deviceCreateInfo.enabledExtensionCount = 0;
			deviceCreateInfo.enabledLayerCount = 0;
			
			// set the values in the struct so that the programmer can use the supplied vulkan objects to easilly impliment stuff.
			DeviceVram = deviceProperties.limits.maxComputeSharedMemorySize;
			pVulkanStuff->physicalDevice = device;
			if (vkCreateDevice(pVulkanStuff->physicalDevice, &deviceCreateInfo, nullptr, &pVulkanStuff->device) != VK_SUCCESS) {
				std::cout << "Unable to setup vulkan context. Some features may be unavailable or not accelerated." << std::endl;
				return false;
			}
			vkGetDeviceQueue(pVulkanStuff->device, queueIndex, 0, &pVulkanStuff->queue);

			std::cout << "Suitable device found:\n" << "\t" << deviceProperties.deviceName << "\n\t" << (float)deviceMemoryProperties.memoryHeaps[0].size / 1074000000 << " GB" << "\n\tVK_QUEUE_COMPUTE & VK_QUEUE_TRANSFER available at queue index " << queueIndex << std::endl;

		}

	}

	if (pVulkanStuff->device == VK_NULL_HANDLE) {
		std::cout << "No devices were found that meet the requirements of this application. Some functionality may be unavailable or not accelerated." << std::endl;
		return false;
	}

	std::cout << "Device picked." << std::endl;
	return true;
}