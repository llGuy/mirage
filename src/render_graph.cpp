#include "render_graph.hpp"

/************************* Render Pass ****************************/
// TODO:
render_pass::render_pass(render_graph *, const uid_string &) {

}



/************************* Compute Pass ***************************/
compute_pass::compute_pass(render_graph *builder, const uid_string &uid) 
: builder_(builder), uid_(uid) {

}

void compute_pass::set_source(const char *src_path) {
    src_path_ = src_path;
}

void compute_pass::add_sampled_image(const uid_string &uid, const image_info &i) {
    uint32_t binding_id = bindings_.size();

    binding b = { binding::type::sampled_image, uid.id };
    bindings_.push_back(b);

    // Get image will allocate space for the image struct if it hasn't been created yet
    gpu_image &img = builder_->get_image_(uid.id);
    img.add_usage_node(uid.id, binding_id);
}

void compute_pass::add_storage_image(const uid_string &uid, const image_info &i) {
    uint32_t binding_id = bindings_.size();

    binding b = { binding::type::storage_image, uid.id };
    bindings_.push_back(b);

    gpu_image &img = builder_->get_image_(uid.id);
    img.add_usage_node(uid.id, binding_id);
}

void compute_pass::add_storage_buffer(const uid_string &uid) {
    binding b = { binding::type::storage_buffer, uid.id };
    bindings_.push_back(b);

    // Do the same for buffers (as we do for images)
}

void compute_pass::add_uniform_buffer(const uid_string &uid) {
    binding b = { binding::type::uniform_buffer, uid.id };
    bindings_.push_back(b);
}

void compute_pass::send_data(const void *data, uint32_t size) {
    if (!push_constant_) {
        push_constant_ = malloc(size);
    }

    memcpy(push_constant_, data, size);
    push_constant_size_ = size;
}

void compute_pass::dispatch(uint32_t count_x, uint32_t count_y, uint32_t count_z) {
    dispatch_params_.x = count_x;
    dispatch_params_.y = count_y;
    dispatch_params_.z = count_z;
    dispatch_params_.is_waves = false;
}

void compute_pass::dispatch_waves(uint32_t wave_x, uint32_t wave_y, uint32_t wave_z) {
    dispatch_params_.x = wave_x;
    dispatch_params_.y = wave_y;
    dispatch_params_.z = wave_z;
    dispatch_params_.is_waves = true;
}

void compute_pass::reset_() {
    // Should keep capacity the same to no reallocs after the first time adding the compute pass
    bindings_.clear();
}



/************************* Graph Pass *****************************/
graph_pass::graph_pass()
: type_(graph_pass::type::none) {

}

graph_pass::graph_pass(const render_pass &pass) 
: rp_(pass), type_(graph_render_pass) {

}

graph_pass::graph_pass(const compute_pass &pass)
: cp_(pass), type_(graph_compute_pass) {

}

graph_pass::type graph_pass::get_type() {
    return type_;
}

render_pass &graph_pass::get_render_pass() {
    return rp_;
}

compute_pass &graph_pass::get_compute_pass() {
    return cp_;
}

bool graph_pass::is_initialized() {
    return (type_ != graph_pass::type::none);
}

binding &graph_pass::get_binding(uint32_t idx) {
    switch (type_) {
    case type::graph_compute_pass: {
        return cp_.bindings_[idx];
    } break;

    case type::graph_render_pass: {
        // return rp.bindings[idx];
        assert(false);
        return cp_.bindings_[idx];
    } break;

    default: 
        assert(false);
        return cp_.bindings_[idx];
    }
}



/************************* Resource *******************************/
gpu_image::gpu_image() 
: head_node_{ invalid_graph_ref, invalid_graph_ref },
  tail_node_{ invalid_graph_ref, invalid_graph_ref },
  usage_(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {

}

gpu_image::gpu_image(render_graph *builder)
: head_node_{ invalid_graph_ref, invalid_graph_ref },
  tail_node_{ invalid_graph_ref, invalid_graph_ref },
  builder_(builder),
  usage_(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {

}

void gpu_image::add_usage_node(graph_stage_ref stg, uint32_t binding_idx) {
    if (!tail_node_.is_invalid()) {
        binding &tail = builder_->get_binding_(tail_node_.stage, tail_node_.binding_idx);

        tail.next.stage = stg;
        tail.next.binding_idx = binding_idx;
    }

    tail_node_.stage = stg;
    tail_node_.binding_idx = binding_idx;

    binding &tail = builder_->get_binding_(tail_node_.stage, tail_node_.binding_idx);
    // Just make sure that if we traverse through this node, we don't try to keep going
    tail.next.invalidate();

    if (head_node_.stage == invalid_graph_ref) {
        head_node_.stage = stg;
        head_node_.binding_idx = binding_idx;
    }
}
 
void gpu_image::update_action(const binding &b) {
    if (image_ == VK_NULL_HANDLE) {
        // Lazily create the images
        action_ = action_flag::to_create;
    }
    else {
        action_ = action_flag::none;
    }

    // This is going to determine which descriptor sets to create
    switch (b.utype) {
    case binding::type::sampled_image: usage_ |= VK_IMAGE_USAGE_SAMPLED_BIT; break;
    case binding::type::storage_image: usage_ |= VK_IMAGE_USAGE_STORAGE_BIT; break;
    default: break;
    }
}

void gpu_image::apply_action() {
    // We need to create this image
    if (action_ == action_flag::to_create) {
        // TODO: actual image creation

        create_descriptors(usage_);
    }
    else if (action_ == action_flag::to_present) {
        reference_ = builder_->get_current_swapchain_ref_();

        // The image to present needs to also have descriptors created for it
        builder_->get_image_(reference_).create_descriptors(usage_);
    }
    else {
        // If the descriptors were already created, no need to do anything for them
        create_descriptors(usage_);
    }
}

// These may or may not match with the usage_ member - could be invoked
// by another image (resource aliasing stuff)
void gpu_image::create_descriptors(VkImageUsageFlags usage) {

}

graph_resource::graph_resource()
: type_(graph_resource::type::none) {

}

graph_resource::graph_resource(const gpu_image &img) 
: type_(type::graph_image), img_(img) {

}

graph_resource::graph_resource(const gpu_buffer &buf) 
: type_(type::graph_buffer), buf_(buf) {

}



/************************* Graph Builder **************************/
render_graph::render_graph() {
    // Setup swapchain targets (we assume that there won't be more than 5 swapchain targets)
    swapchain_uids[0] = RES("swapchain-target-0");
    swapchain_uids[1] = RES("swapchain-target-1");
    swapchain_uids[2] = RES("swapchain-target-2");
    swapchain_uids[3] = RES("swapchain-target-3");
    swapchain_uids[4] = RES("swapchain-target-4");
}

void render_graph::register_swapchain(const graph_swapchain_info &swp) {
    // resources_.push_back();
}

render_pass &render_graph::add_render_pass(const uid_string &uid) {
    if (passes_.size() <= uid.id) {
        // Reallocate the vector such that the new pass will fit
        passes_.resize(uid.id + 1);
    }

    if (!passes_[uid.id].is_initialized())
        new (&passes_[uid.id]) graph_pass(render_pass(this, uid));

    recorded_stages_.push_back(uid.id);

    return get_render_pass_(uid.id);
}

compute_pass &render_graph::add_compute_pass(const uid_string &uid) {
    if (passes_.size() <= uid.id) {
        // Reallocate the vector such that the new pass will fit
        passes_.resize(uid.id + 1);
    }

    if (!passes_[uid.id].is_initialized())
        new (&passes_[uid.id]) graph_pass(compute_pass(this, uid));

    recorded_stages_.push_back(uid.id);

    return get_compute_pass_(uid.id);
}

void render_graph::begin() {
    recorded_stages_.clear();
    present_info_.is_active = false;
}

void render_graph::end() {
    // First traverse through all stages in order to figure out which resources to use
    for (auto stg : recorded_stages_) {
        if (stg == graph_stage_ref_present) {
            // Handle present case
            gpu_image &swapchain_target = get_image_(present_info_.to_present);
            swapchain_target.action_ = gpu_image::action_flag::to_present;
        }
        else {
            // Handle render pass / compute pass case
            switch (passes_[stg].get_type()) {
            case graph_pass::graph_compute_pass: {
                compute_pass &cp = passes_[stg].get_compute_pass();

                // Loop through each binding
                for (auto &bind : cp.bindings_) {

                }
            } break;

            case graph_pass::graph_render_pass: {
                render_pass &rp = passes_[stg].get_render_pass();
            } break;

            default: break;
            }
        }
    }
}

void render_graph::present(const uid_string &res_uid) {
    present_info_.to_present = res_uid.id;
    present_info_.is_active = true;

    gpu_image &img = get_image_(res_uid.id);
    img.add_usage_node(graph_stage_ref_present, 0);

    recorded_stages_.push_back(graph_stage_ref_present);
}
