#pragma once

#include <vector>
#include <assert.h>
#include "string.hpp"
#include "heap_array.hpp"
#include <vulkan/vulkan.h>

// ID of a resource
using graph_resource_ref = uint32_t;
// ID of a pass stage
using graph_stage_ref = uint32_t;

constexpr uint32_t invalid_graph_ref = 0xFFFFFFFF;
constexpr uint32_t graph_stage_ref_present = 0xBADC0FFE;

class render_graph;

// This whole struct is basically like a pointer
struct resource_usage_node {
    inline void invalidate() {
        stage = invalid_graph_ref;
        binding_idx = invalid_graph_ref;
    }

    inline bool is_invalid() {
        return stage == invalid_graph_ref;
    }

    graph_stage_ref stage;

    // May need more data (optional for a lot of cases) to describe this
    uint32_t binding_idx;
};

struct binding {
    enum type {
        sampled_image, storage_image, max_image, storage_buffer, uniform_buffer, max_buffer, none
    };

    // Index of this binding
    uint32_t idx;
    type utype;
    graph_resource_ref rref;

    // This is going to point to the next place that the given resource is used
    // Member is written to by the resource which is pointed to by the binding
    resource_usage_node next;

    VkDescriptorType get_descriptor_type() {
        switch (utype) {
        case sampled_image: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case storage_image: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case storage_buffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case uniform_buffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        default: return VK_DESCRIPTOR_TYPE_MAX_ENUM;
        }
    }

    VkImageLayout get_image_layout() {
        assert(utype < type::max_image);

        switch(utype) {
        case type::sampled_image:
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case type::storage_image:
            return VK_IMAGE_LAYOUT_GENERAL;
        default:
            assert(false);
            return VK_IMAGE_LAYOUT_MAX_ENUM;
        }
    }

    VkAccessFlags get_image_access() {
        assert(utype < type::max_image);

        switch(utype) {
        case type::sampled_image:
            return VK_ACCESS_SHADER_READ_BIT;
        case type::storage_image:
            return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        default:
            assert(false);
            return (VkAccessFlags)0;
        }
    }
};

struct image_info {
    // If format is NULL, use swapchain image
    VkFormat format;

    // If 0 in all dimentions, inherit extent from swapchain
    VkExtent3D extent = {};
};

/************************* Render Pass ****************************/
class render_pass {
public:
    render_pass() = default;
    render_pass(render_graph *, const uid_string &uid);

    ~render_pass() = default;

private:
    render_graph *builder_;

    friend class render_graph;
    friend class graph_pass;
};



/************************* Compute Pass ***************************/
class compute_pass {
public:
    compute_pass() = default;
    compute_pass(render_graph *, const uid_string &uid);

    ~compute_pass() = default;

    void set_source(const char *src_path);

    void send_data(const void *data, uint32_t size);

    // Push constant
    template <typename T>
    void send_data(const T &data) {
        send_data(&data, sizeof(T));
    }

    // Whenever we add a resource here, we need to update the linked list
    // of used nodes that start with the resource itself
    void add_sampled_image(const uid_string &, const image_info &i);
    void add_storage_image(const uid_string &, const image_info &i = {});
    void add_storage_buffer(const uid_string &);
    void add_uniform_buffer(const uid_string &);

    // Configures the dispatch with given dimensions
    void dispatch(uint32_t count_x, uint32_t count_y, uint32_t count_z);
    // Configures the dispatch with the size of each wave - specify which image to get resolution from
    void dispatch_waves(uint32_t wave_x, uint32_t wave_y, uint32_t wave_z, const uid_string &);
    // TODO: Support indirect

private:
    // Gets called everytime you call add_compute_pass from render_graph
    void reset_();

    // Actually creates the compute pass
    void create_();

    // Issue the commands to command buffer
    void issue_commands_(VkCommandBuffer cmdbuf);

    // void add_image(binding::type type);

    // Configuration data
private:
    // Push constant information
    void *push_constant_;
    uint32_t push_constant_size_;

    // Shader source path
    const char *src_path_;

    // Uniform bindings
    std::vector<binding> bindings_;

    // Dispatch parameters
    struct {
        uint32_t x, y, z;
        // Index of the binding for which we will bind our resolution with
        uint32_t binding_res;
        bool is_waves;
    } dispatch_params_;

    // Actual Vulkan objects
private:
    VkPipeline pipeline_;
    VkPipelineLayout layout_;

private:
    uid_string uid_;

    // If the compute pass is uninitialized, builder_ is nullptr
    render_graph *builder_;

    friend class render_graph;
    friend class graph_pass;
};

class graph_pass {
public:
    enum type {
        graph_compute_pass, graph_render_pass, none
    };

    graph_pass();
    graph_pass(const render_pass &);
    graph_pass(const compute_pass &);

    ~graph_pass() {
        switch (type_) {
            case graph_compute_pass: cp_.~compute_pass(); break;
            case graph_render_pass: rp_.~render_pass(); break;
            default: break;
        }
    };

    graph_pass &operator=(graph_pass &&other) {
        type_ = other.type_;
        switch (type_) {
            case graph_compute_pass: cp_ = std::move(other.cp_); break;
            case graph_render_pass: rp_ = std::move(other.rp_); break;
            default: break;
        }

        return *this;
    }

    graph_pass(graph_pass &&other) {
        type_ = other.type_;
        switch (type_) {
            case graph_compute_pass: cp_ = std::move(other.cp_); break;
            case graph_render_pass: rp_ = std::move(other.rp_); break;
            default: break;
        }
    }

    type get_type();
    render_pass &get_render_pass();
    compute_pass &get_compute_pass();

    binding &get_binding(uint32_t idx);

    bool is_initialized();

private:
    // For uninitialized graph passes, type_ is none
    type type_;

    union {
        compute_pass cp_;
        render_pass rp_;
    };
};



/************************* Resource *******************************/
class gpu_buffer { // TODO
public:
    gpu_buffer(render_graph *graph) 
    : builder_(graph) {

    }

    // TODO
    void update_action(const binding &b) {}
    void apply_action() {}

private:
    resource_usage_node head_node_;
    resource_usage_node tail_node_;

    // Keep track of last stage this was used in

    render_graph *builder_;
};

class gpu_image {
public:
    // TODO: Be smarter about how we can alias this resource to another
    // for now, we only support aliasing to the swapchain image
    enum action_flag { to_create, to_present, none };

    gpu_image();
    gpu_image(render_graph *);

    void add_usage_node(graph_stage_ref stg, uint32_t binding_idx);

    // Update action but also its usage flags
    void update_action(const binding &b);

    void apply_action();

    // Create descriptors given a usage flag (if the descriptors were already created,
    // don't do anything for that kind of descriptor).
    void create_descriptors(VkImageUsageFlags usage);

    VkDescriptorSet get_descriptor_set(binding::type t);

    // This is necessary in the case of indirection (there is always at most one level of indirection)
    gpu_image &get();

private:
    resource_usage_node head_node_;
    resource_usage_node tail_node_;

    render_graph *builder_;
    action_flag action_;

private:
    // Are we referencing another image?
    graph_resource_ref reference_;

    // Actual image data
    VkImage image_;
    VkImageView image_view_;
    VkDeviceMemory image_memory_;

    VkExtent3D extent_;

    VkImageAspectFlags aspect_;

    VkImageUsageFlags usage_;

    VkDescriptorSet descriptor_sets_[binding::type::none];

    // Keep track of current layout / usage stage
    VkImageLayout current_layout_;
    VkAccessFlags current_access_;
    VkPipelineStageFlags last_used_;

    friend class render_graph;
    friend class compute_pass;
};

class graph_resource {
public:
    enum type { graph_image, graph_buffer, none };

    graph_resource();
    graph_resource(const gpu_image &img);
    graph_resource(const gpu_buffer &buf);

    ~graph_resource() {
        switch (type_) {
            case graph_image: img_.~gpu_image(); break;
            case graph_buffer: buf_.~gpu_buffer(); break;
            default: break;
        }
    };

    graph_resource &operator=(graph_resource &&other) {
        type_ = other.type_;
        switch (type_) {
            case graph_image: img_ = std::move(other.img_); break;
            case graph_buffer: buf_ = std::move(other.buf_); break;
            default: break;
        }

        return *this;
    }

    graph_resource(graph_resource &&other) {
        type_ = other.type_;
        switch (type_) {
            case graph_image: img_ = std::move(other.img_); break;
            case graph_buffer: buf_ = std::move(other.buf_); break;
            default: break;
        }
    }

    // Given a binding, update the action that should be done on the resource
    inline void update_action(const binding &b) {
        switch (type_) {
        case graph_image: img_.update_action(b); break;
        case graph_buffer: buf_.update_action(b); break;
        default: break;
        }
    }

    inline type get_type() { return type_; }
    inline gpu_buffer &get_buffer() { return buf_; }
    inline gpu_image &get_image() { return img_; }

private:
    type type_;
    bool was_used_;

    union {
        gpu_buffer buf_;
        gpu_image img_;
    };

    friend class render_graph;
};



/************************* Graph Builder **************************/
struct graph_swapchain_info {
    uint32_t image_count;
    VkImage *images;
    VkImageView *image_views;
};

// This may be something that takes care of presenting to the screen
// Can create any custom generator you want if you want to custom set synch primitives
class cmdbuf_generator {
public:
    struct cmdbuf_info {
        uint32_t swapchain_idx;
        uint32_t frame_idx;
        VkCommandBuffer cmdbuf;
    };

    virtual cmdbuf_info get_command_buffer() = 0;
    virtual void submit_command_buffer(const cmdbuf_info &info, VkPipelineStageFlags stage) = 0;
};

class present_cmdbuf_generator : public cmdbuf_generator {
public:
    present_cmdbuf_generator(u32 frames_in_flight);

    // TODO: Add input and output synchornization semaphores and fences
    cmdbuf_info get_command_buffer() override;
    void submit_command_buffer(const cmdbuf_info &info, VkPipelineStageFlags stage) override;

private:
    heap_array<VkSemaphore> image_ready_semaphores_;
    heap_array<VkSemaphore> render_finished_semaphores_;
    heap_array<VkFence> fences_;
    heap_array<VkCommandBuffer> command_buffers_;

    u32 current_frame_;
    u32 max_frames_in_flight_;
};

class single_cmdbuf_generator : public cmdbuf_generator {
public:
    cmdbuf_info get_command_buffer() override;
    void submit_command_buffer(const cmdbuf_info &info, VkPipelineStageFlags stage) override;

private:

};

class render_graph {
public:
    render_graph();

    struct swapchain_info {
        u32 swapchain_image_count;
        VkImage *images;
        VkImageView *image_views;
        VkExtent2D extent;
    };

    void setup_swapchain(const swapchain_info &swapchain);

    // Register swapchain targets in the resources array
    void register_swapchain(const graph_swapchain_info &swp);
    
    // This will schedule the passes and potentially allocate and create them
    render_pass &add_render_pass(const uid_string &);
    compute_pass &add_compute_pass(const uid_string &);

    // Start recording a set of passes / commands
    void begin();

    // End the commands and submit - determines whether or not to create a command buffer on the fly
    // If there is a present command, will use the present_cmdbuf_generator
    // If not, will use single_cmdbuf_generator,
    // But can specify custom generator too
    void end(cmdbuf_generator *generator = nullptr);

    // Make sure that this image is what gets presented to the screen
    void present(const uid_string &);

private:
    inline compute_pass &get_compute_pass_(graph_stage_ref stg) { return passes_[stg].get_compute_pass(); }
    inline render_pass &get_render_pass_(graph_stage_ref stg) { return passes_[stg].get_render_pass(); }

    // For resources, we need to allocate them on the fly - not the case with passes
    inline graph_resource &get_resource_(graph_resource_ref id) {
        if (resources_.size() <= id)
            resources_.resize(id + 1);
        return resources_[id];
    }

    inline gpu_buffer &get_buffer_(graph_resource_ref id) { 
        auto &res = get_resource_(id);
        if (res.get_type() == graph_resource::type::none)
            new (&res) graph_resource(gpu_buffer(this));
        return res.get_buffer();
    }

    inline gpu_image &get_image_(graph_resource_ref id) {
        if (id == graph_stage_ref_present) {
            return get_image_(get_current_swapchain_ref_());
        }

        auto &res = get_resource_(id);
        if (res.get_type() == graph_resource::type::none)
            new (&res) graph_resource(gpu_image(this));
        return res.get_image();
    }

    inline binding &get_binding_(graph_stage_ref stg, uint32_t binding_idx) {
        if (stg == graph_stage_ref_present)
            return present_info_.b;
        else
            return passes_[stg].get_binding(binding_idx); 
    }

    graph_resource_ref get_current_swapchain_ref_() {
        return swapchain_uids[swapchain_img_idx_].id;
    }

public:
    static inline uint32_t rdg_name_id_counter = 0;
    static inline uint32_t stg_name_id_counter = 0;

private:
    static constexpr uint32_t swapchain_img_max_count = 5;
    static inline uid_string swapchain_uids[swapchain_img_max_count] = {};

    uint32_t swapchain_img_idx_;

    // All existing passes that could be used
    std::vector<graph_pass> passes_;
    // All existing resources that could be used
    std::vector<graph_resource> resources_;

    std::vector<graph_stage_ref> recorded_stages_;
    std::vector<graph_resource_ref> used_resources_;

    struct present_info {
        graph_resource_ref to_present;
        // Are we presenting with these commands?
        bool is_active;

        binding b;
    } present_info_;

    // Default cmdbuf generators
    present_cmdbuf_generator present_generator_;

    friend class compute_pass;
    friend class render_pass;
    friend class gpu_image;
    friend class gpu_buffer;
};

#define RES(x) (uid_string{     \
    x, sizeof(x),               \
    get_id<crc32<sizeof(x) - 2>(x) ^ 0xFFFFFFFF>(render_graph::rdg_name_id_counter)})

#define STG(x) (uid_string{     \
    x, sizeof(x),               \
    get_id<crc32<sizeof(x) - 2>(x) ^ 0xFFFFFFFF>(render_graph::stg_name_id_counter)})
