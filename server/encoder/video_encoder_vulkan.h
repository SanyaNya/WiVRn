/*
 * WiVRn VR streaming
 * Copyright (C) 2024  Patrick Nicolas <patricknicolas@laposte.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "video_encoder.h"
#include "vk/allocation.h"

namespace wivrn
{
class video_encoder_vulkan : public VideoEncoder
{
	wivrn_vk_bundle & vk;
	const vk::VideoEncodeCapabilitiesKHR encode_caps;

	vk::raii::VideoSessionKHR video_session = nullptr;
	vk::raii::VideoSessionParametersKHR video_session_parameters = nullptr;

	vk::raii::QueryPool query_pool = nullptr;

	buffer_allocation output_buffer;
	size_t output_buffer_size;

	vk::ImageViewUsageCreateInfo image_view_template_next;
	vk::ImageViewCreateInfo image_view_template;
	std::unordered_map<VkImage, vk::raii::ImageView> image_views; // for input images
	std::array<vk::Fence, num_slots> fences;

	image_allocation dpb_image;

	struct dpb_item
	{
		vk::raii::ImageView image_view;
		vk::VideoPictureResourceInfoKHR resource;
		vk::VideoReferenceSlotInfoKHR & info;
		uint64_t frame_index = -1;
	};

	std::vector<dpb_item> dpb;
	std::vector<vk::VideoReferenceSlotInfoKHR> dpb_info;

	std::vector<vk::raii::DeviceMemory> mem;

	vk::VideoFormatPropertiesKHR select_video_format(
	        vk::raii::PhysicalDevice & physical_device,
	        const vk::PhysicalDeviceVideoFormatInfoKHR &);

	uint32_t frame_num = 0;
	std::atomic<uint64_t> last_ack = 0;
	bool session_initialized = false;
	const vk::Rect2D rect;
	const float fps;

	vk::VideoEncodeRateControlLayerInfoKHR rate_control_layer;

protected:
	const uint8_t num_dpb_slots = 5;
	std::optional<vk::VideoEncodeRateControlInfoKHR> rate_control;

	video_encoder_vulkan(wivrn_vk_bundle & vk, vk::Rect2D rect, vk::VideoEncodeCapabilitiesKHR encode_caps, float fps, uint64_t bitrate);

	void init(const vk::VideoCapabilitiesKHR & video_caps,
	          const vk::VideoProfileInfoKHR & video_profile,
	          void * video_session_create_next,
	          void * session_params_next);

	virtual ~video_encoder_vulkan();

	std::vector<uint8_t> get_encoded_parameters(void * next);

	virtual void send_idr_data() = 0;

	virtual std::vector<void *> setup_slot_info(size_t dpb_size) = 0;
	virtual void * encode_info_next(uint32_t frame_num, size_t slot, std::optional<int32_t> reference_slot) = 0;
	virtual vk::ExtensionProperties std_header_version() = 0;

public:
	std::optional<data> encode(bool idr, std::chrono::steady_clock::time_point target_timestamp, uint8_t slot) override;
	void present_image(vk::Image y_cbcr, vk::raii::CommandBuffer & cmd_buf, vk::Fence, uint8_t slot, uint64_t frame_index) override;
	void on_feedback(const from_headset::feedback &) override;
};
} // namespace wivrn
