#pragma once

#include <vulkan/vulkan.h>

#include "types.hpp"

enum load_and_store_op {
    load_then_store          = 0 | (0 << 2),
    load_then_dont_care      = 0 | (1 << 2),

    clear_then_Store         = 1 | (0 << 2),
    clear_then_dont_care     = 1 | (1 << 2),

    dont_care_then_store     = 2 | (0 << 2),
    dont_care_then_dont_care = 2 | (1 << 2)
};

class render_pass_config {
public:
    render_pass_config(u32 attachment_count, u32 subpass_count);

    void add_attachment(load_and_store_op op, load_and_store_op depth_op);
};

class render_pass {

};
