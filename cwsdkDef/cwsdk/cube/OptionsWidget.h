#pragma once

#include "../non_cube.h"
#include "../plasma.h"
#include "../msvc_bincompat.h"

namespace cube {
	struct OptionsWidget : plasma::Widget
	{
		//uint32_t field_0x15c;
		MSVCBinCompat::vector<uint32_t> screen_coordinates;
		GameController *gc;
		plasma::Node* mode_left_button;
		plasma::Node* mode_right_button;
		plasma::Node* resolution_left_button;
		plasma::Node* resolution_right_button;
		plasma::Node* antialiasing_left_button;
		plasma::Node* antialiasing_right_button;
		plasma::Node* render_distance_left_button;
		plasma::Node* render_distance_right_button;
		plasma::Node* fx_volume_left_button;
		plasma::Node* fx_volume_right_button;
		plasma::Node* music_volume_left_button;
		plasma::Node* music_volume_right_button;
		plasma::Node* camera_speed_left_button;
		plasma::Node* camera_speed_right_button;
		plasma::Node* camera_smoothness_left_button;
		plasma::Node* camera_smoothness_right_button;
		plasma::Node* invert_y_axis_left_button;
		plasma::Node* invert_y_axis_right_button;
		plasma::Node* fps_limit_left_button;
		plasma::Node* fps_limit_right_button;
		plasma::Node* language_left_button;
		plasma::Node* language_right_button;
		plasma::Node* apply_button;
		plasma::Node* ok_button;
		plasma::Node* cancel_button;
		uint32_t is_fullscreen;
		uint32_t resolution_x;
		uint32_t resolution_y;
		uint32_t antialiasing;
		uint32_t render_distance;
		uint32_t sound_fx_volume;
		uint32_t music_volume;
		uint32_t camera_speed;
		uint32_t camera_smoothness;
		uint32_t is_y_axis_inverted;
		int32_t language_id;
		uint32_t milliseconds_per_frame;
	};
};