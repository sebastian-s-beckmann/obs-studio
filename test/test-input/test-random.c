#include <stdlib.h>
#include <util/threading.h>
#include <util/platform.h>
#include <obs.h>

struct random_tex {
	obs_source_t *source;
	os_event_t *stop_signal;
	pthread_t thread;
	bool initialized;
};

static const char *random_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "20x20 Random Pixel Texture Source (Test)";
}

static void random_destroy(void *data)
{
	struct random_tex *rt = data;

	if (rt) {
		if (rt->initialized) {
			os_event_signal(rt->stop_signal);
			pthread_join(rt->thread, NULL);
		}

		os_event_destroy(rt->stop_signal);
		bfree(rt);
	}
}

static inline void fill_texture(uint32_t *pixels)
{
	size_t x, y;

	for (y = 0; y < 20; y++) {
		for (x = 0; x < 20; x++) {
			uint32_t pixel = 0;
			pixel |= (rand() % 256);
			pixel |= (rand() % 256) << 8;
			pixel |= (rand() % 256) << 16;
			//pixel |= (rand()%256) << 24;
			//pixel |= 0xFFFFFFFF;
			pixels[y * 20 + x] = pixel;
		}
	}
}

static void *video_thread(void *data)
{
	struct random_tex *rt = data;
	uint32_t pixels[20 * 20];
	uint64_t cur_time = os_gettime_ns();

	struct obs_source_frame frame = {
		.data = {[0] = (uint8_t *)pixels},
		.linesize = {[0] = 20 * 4},
		.width = 20,
		.height = 20,
		.format = VIDEO_FORMAT_BGRX,
	};

	while (os_event_try(rt->stop_signal) == EAGAIN) {
		fill_texture(pixels);

		frame.timestamp = cur_time;

		obs_source_output_video(rt->source, &frame);

		os_sleepto_ns(cur_time += 250000000);
	}

	return NULL;
}

static void *random_create(obs_data_t *settings, obs_source_t *source)
{
	struct random_tex *rt = bzalloc(sizeof(struct random_tex));
	rt->source = source;

	if (os_event_init(&rt->stop_signal, OS_EVENT_TYPE_MANUAL) != 0) {
		random_destroy(rt);
		return NULL;
	}

	if (pthread_create(&rt->thread, NULL, video_thread, rt) != 0) {
		random_destroy(rt);
		return NULL;
	}

	rt->initialized = true;

	UNUSED_PARAMETER(settings);
	UNUSED_PARAMETER(source);
	return rt;
}

static bool button_clicked_callback(__unused obs_properties_t *props, __unused obs_property_t *property, void *data)
{
	blog(LOG_INFO, "Clicked!");
	return (bool)data;
}

static obs_properties_t *random_properties(__unused void *data)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;

	// Bool, Int, Float
	p = obs_properties_add_bool(props, "bool", "Bool");
	obs_property_set_long_description(p, "On or off");
	p = obs_properties_add_int(props, "int", "Integer", 1, 96, 2);
	obs_property_set_long_description(p, "Integer from 1 to 100 in steps of 5");
	p = obs_properties_add_float(props, "float", "Float", 0, 2, 0.1);
	obs_property_set_long_description(p, "Float from 0 to 2 in steps of 0.1");
	p = obs_properties_add_int_slider(props, "int_slider", "Int slider", 1, 20, 1);
	obs_property_set_long_description(p, "Integer slider from 1 to 20 in steps of 1");
	p = obs_properties_add_float_slider(props, "float_slider", "Float slider", 5, 15, 0.01);
	obs_property_set_long_description(p, "Float slider from 5 to 15 in steps of 0.01");

	// Input Text
	p = obs_properties_add_text(props, "text_default", "Text (Default)", OBS_TEXT_DEFAULT);
	obs_property_set_long_description(p, "Normal single line text input");
	p = obs_properties_add_text(props, "text_default_mono", "Text (Default, Monospace)", OBS_TEXT_DEFAULT);
	obs_property_text_set_monospace(p, true);
	obs_property_set_long_description(p, "Normal single line text input (monospace)");
	p = obs_properties_add_text(props, "text_password", "Text (Password)", OBS_TEXT_PASSWORD);
	obs_property_set_long_description(p, "Password text input");
	p = obs_properties_add_text(props, "text_password_mono", "Text (Password, Monospace)", OBS_TEXT_PASSWORD);
	obs_property_set_long_description(p, "Password text input (monospace)");
	obs_property_text_set_monospace(p, true);
	p = obs_properties_add_text(props, "text_multiline", "Text (Multiline)", OBS_TEXT_MULTILINE);
	obs_property_set_long_description(p, "Multiline text input");
	p = obs_properties_add_text(props, "text_multiline_mono", "Text (Multiline, Monospace)", OBS_TEXT_MULTILINE);
	obs_property_set_long_description(p, "Multiline text input (monospace)");
	obs_property_text_set_monospace(p, true);

	// Info Text
	p = obs_properties_add_text(props, "text_info_default", "Info Text (Default)", OBS_TEXT_INFO);
	obs_property_set_long_description(p, "This is an informational text.");
	p = obs_properties_add_text(props, "text_info_warning", "Info Text (Warning)", OBS_TEXT_INFO);
	obs_property_text_set_info_type(p, OBS_TEXT_INFO_WARNING);
	obs_property_set_long_description(p, "This is warning.");
	p = obs_properties_add_text(props, "text_info_error", "Info Text (Error)", OBS_TEXT_INFO);
	obs_property_text_set_info_type(p, OBS_TEXT_INFO_ERROR);
	obs_property_set_long_description(p, "This is an error.");
	// word_warp and the settings logic are intentionally not included in this test.

	// Paths
	p = obs_properties_add_path(props, "path_file", "File", OBS_PATH_FILE, "Text files (*.txt);;", NULL);
	obs_property_set_long_description(p, "Path to an existing text file");
	p = obs_properties_add_path(props, "path_directory", "Directory", OBS_PATH_DIRECTORY, "", NULL);
	obs_property_set_long_description(p, "Path to a directory");
	p = obs_properties_add_path(props, "path_savefile", "Save file", OBS_PATH_FILE_SAVE, "Text file (*.txt);;",
				    NULL);
	obs_property_set_long_description(p, "Path to save a file to (the file doesn't need to exists)");

	// Lists
	p = obs_properties_add_list(props, "list_combo_int", "List (Combo, Int)", OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_INT);
	obs_property_set_long_description(p, "List of integers as combobox");
	obs_property_list_add_int(p, "One (1)", 1);
	obs_property_list_add_int(p, "Two (2)", 2);
	obs_property_list_add_int(p, "Three (3) (disabled)", 3);
	obs_property_list_item_disable(p, 2, true); // TODO: Rename to ..._item_set_disabled(false)
	p = obs_properties_add_list(props, "list_combo_float", "List (Combo, Float)", OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_FLOAT);
	obs_property_set_long_description(p, "List of floats as combobox");
	obs_property_list_add_float(p, "One (1)", 1);
	obs_property_list_add_float(p, "One and a half (1.5) (disabled)", 1.5);
	obs_property_list_item_disable(p, 1, true);
	obs_property_list_add_float(p, "One point seven (1.7)", 1.7);
	p = obs_properties_add_list(props, "list_combo_string", "List (Combo, String)", OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);
	obs_property_set_long_description(p, "List of strings as combobox");
	obs_property_list_add_string(p, "First string (disabled)", "value of first");
	obs_property_list_item_disable(p, 0, true);
	obs_property_list_add_string(p, "Something", "nothing");
	obs_property_list_add_string(p, "Even more", "still nothing");
	p = obs_properties_add_list(props, "list_combo_bool", "List (Combo, Bool)", OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_BOOL);
	obs_property_set_long_description(p, "List of integers as combobox");
	obs_property_list_add_bool(p, "Yes", true);
	obs_property_list_add_bool(p, "No (but disabled)", false);
	obs_property_list_item_disable(p, 1, true);
	p = obs_properties_add_list(props, "list_radio_int", "List (Radio, Int)", OBS_COMBO_TYPE_RADIO,
				    OBS_COMBO_FORMAT_INT);
	obs_property_set_long_description(p, "List of integers as radio buttons");
	obs_property_list_add_int(p, "One (1)", 1);
	obs_property_list_add_int(p, "Two (2)", 2);
	obs_property_list_add_int(p, "Three (3) (disabled)", 3);
	obs_property_list_item_disable(p, 2, true); // TODO: Rename to ..._item_set_disabled(false)
	p = obs_properties_add_list(props, "list_radio_float", "List (Radio, Float)", OBS_COMBO_TYPE_RADIO,
				    OBS_COMBO_FORMAT_FLOAT);
	obs_property_set_long_description(p, "List of floats as radio buttons");
	obs_property_list_add_float(p, "One (1)", 1);
	obs_property_list_add_float(p, "One and a half (1.5) (disabled)", 1.5);
	obs_property_list_item_disable(p, 1, true);
	obs_property_list_add_float(p, "One point seven (1.7)", 1.7);
	p = obs_properties_add_list(props, "list_radio_string", "List (Radio, String)", OBS_COMBO_TYPE_RADIO,
				    OBS_COMBO_FORMAT_STRING);
	obs_property_set_long_description(p, "List of strings as radio buttons");
	obs_property_list_add_string(p, "First string (disabled)", "value of first");
	obs_property_list_item_disable(p, 0, true);
	obs_property_list_add_string(p, "Something", "nothing");
	obs_property_list_add_string(p, "Even more", "still nothing");
	p = obs_properties_add_list(props, "list_radio_bool", "List (Radio, Bool)", OBS_COMBO_TYPE_RADIO,
				    OBS_COMBO_FORMAT_BOOL);
	obs_property_set_long_description(p, "List of integers as radio buttons");
	obs_property_list_add_bool(p, "Yes", true);
	obs_property_list_add_bool(p, "No (but disabled)", false);
	obs_property_list_item_disable(p, 1, true);
	p = obs_properties_add_list(props, "list_editable", "List (Editable, String)", OBS_COMBO_TYPE_EDITABLE,
				    OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, "Something", "nothing");
	obs_property_list_add_string(p, "Even more", "still nothing");
	obs_property_set_long_description(p, "Combo box of strings that is editable "
					     "(Not to be confused with 'Editable List'). "
					     "This is only allowed for strings (per the docs).");

	// Color
	p = obs_properties_add_color(props, "color_normal", "Color (Normal)");
	obs_property_set_long_description(p, "Normal color.");
	p = obs_properties_add_color_alpha(props, "color_alpha", "Color (Alpha)");
	obs_property_set_long_description(p, "Color that allows transparency.");

	// Buttons
	p = obs_properties_add_button(props, "button_false", "Button (useless)", button_clicked_callback);
	obs_property_set_long_description(p, "Button that does nothing (except print a log message)");
	p = obs_properties_add_button2(props, "button_true", "Button (refresh)", button_clicked_callback, (void *)true);
	obs_property_set_long_description(p, "Button that refreshes the properties when clicked.");
	p = obs_properties_add_button(props, "button_link", "Button (link)", NULL);
	obs_property_button_set_type(p, OBS_BUTTON_URL);
	obs_property_button_set_url(p, "https://obsproject.com");

	// Font
	p = obs_properties_add_font(props, "font", "Font");
	obs_property_set_long_description(p, "A font of your choice");

	// Editable list
	p = obs_properties_add_editable_list(props, "editablelist_files", "Editable list", OBS_EDITABLE_LIST_TYPE_FILES,
					     "All Files (*.*)", "");
	obs_property_set_long_description(p, "A list of files");
	p = obs_properties_add_editable_list(props, "editablelist_strings", "Editable list",
					     OBS_EDITABLE_LIST_TYPE_STRINGS, "All Files (*.*)", "");
	obs_property_set_long_description(p, "A list of strings");
	p = obs_properties_add_editable_list(props, "editablelist_files_and_urls", "Editable list",
					     OBS_EDITABLE_LIST_TYPE_FILES_AND_URLS, "All Files (*.*)", "");
	obs_property_set_long_description(p, "A list of files and URLs");

	// Frame rate
	p = obs_properties_add_frame_rate(props, "framerate", "Frame rate");
	obs_property_frame_rate_option_add(p, "custom1", "Custom selection");
	obs_property_frame_rate_option_add(p, "custom2", "Other custom selection");
	const struct media_frames_per_second min1 = {10, 1};
	const struct media_frames_per_second max1 = {1000, 3};
	const struct media_frames_per_second min2 = {800, 1};
	const struct media_frames_per_second max2 = {1000, 1};
	obs_property_frame_rate_fps_range_add(p, min1, max1);
	obs_property_frame_rate_fps_range_add(p, min2, max2);
	obs_property_set_long_description(p, "How many frames do you want? (Allowed: 10-333.3 and 800-1000)");

	// Groups
	obs_properties_t *group;

	group = obs_properties_create();
	p = obs_properties_add_int(group, "group1_int", "Int", 1, 5, 1);
	obs_property_set_long_description(p, "Integer within the group");
	p = obs_properties_add_text(group, "group1_text", "Text (Default)", OBS_TEXT_DEFAULT);
	obs_property_set_long_description(p, "Single line text in the group");
	p = obs_properties_add_group(props, "group1", "Group", OBS_GROUP_NORMAL, group);
	obs_property_set_long_description(p, "A normal group");

	group = obs_properties_create();
	p = obs_properties_add_int(group, "group2_int", "Int", 1, 42, 1);
	obs_property_set_long_description(p, "Integer within the checkable group");
	p = obs_properties_add_text(group, "group2_text", "Text (Multiline)", OBS_TEXT_MULTILINE);
	obs_property_set_long_description(p, "Multiline text in the group");
	p = obs_properties_add_group(props, "group2", "Checkable Group", OBS_GROUP_CHECKABLE, group);
	obs_property_set_long_description(p, "A checkable group");

	return props;
}

struct obs_source_info test_random = {
	.id = "random",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_ASYNC_VIDEO,
	.get_name = random_getname,
	.create = random_create,
	.destroy = random_destroy,
	.get_properties = random_properties,
};
