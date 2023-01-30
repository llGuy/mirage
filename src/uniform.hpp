#pragma once

#include "types.hpp"
#include <vulkan/vulkan.h>

struct uprototype {
    VkDescriptorType type;
    u32 count;
};

class uobject {
public:
    virtual VkDescriptorSet get_descriptor_set(VkDescriptorType type) = 0;
};
