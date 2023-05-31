// AUTOGENERATED COPYRIGHT HEADER START
// Copyright (C) 2023 Michael Fabian 'Xaymar' Dirks <info@xaymar.com>
// AUTOGENERATED COPYRIGHT HEADER END
// Copyright (C) 2022 Carsten Braun <info@braun-cloud.de>
// Copyright (C) 2022 lainon <GermanAizek@yandex.ru>
// Copyright (C) 2022-2023 Michael Fabian 'Xaymar' Dirks <info@xaymar.com>

#include "dnxhd.hpp"
#include "common.hpp"
#include "../codecs/dnxhr.hpp"
#include "ffmpeg/tools.hpp"
#include "plugin.hpp"

#include "warning-disable.hpp"
#include <array>
#include "warning-enable.hpp"

using namespace streamfx::encoder::ffmpeg;
using namespace streamfx::encoder::codec::dnxhr;

inline const char* dnx_profile_to_display_name(const char* profile)
{
	char buffer[1024];
	snprintf(buffer, sizeof(buffer), "%s.%s", S_CODEC_DNXHR_PROFILE, profile);
	return D_TRANSLATE(buffer);
}

dnxhd::dnxhd() : handler("dnxhd") {}

dnxhd::~dnxhd() {}

void dnxhd::adjust_info(ffmpeg_factory* factory, std::string& id, std::string& name, std::string& codec)
{
	//Most people don't know what VC3 is and only know it as DNx.
	//Change name to make it easier to find.
	name = "Avid DNxHR (via FFmpeg)";
}

bool dnxhd::has_keyframes(ffmpeg_factory* instance)
{
	return false;
}

void dnxhd::defaults(ffmpeg_factory* factory, obs_data_t* settings)
{
	obs_data_set_default_string(settings, S_CODEC_DNXHR_PROFILE, "dnxhr_sq");
}

void dnxhd::properties(ffmpeg_factory* factory, ffmpeg_instance* instance, obs_properties_t* props)
{
	// Try and acquire a valid context.
	std::shared_ptr<AVCodecContext> ctx;
	if (instance) {
		ctx = std::shared_ptr<AVCodecContext>(instance->get_avcodeccontext(), [](AVCodecContext*) {});
	} else { // If we don't have a context, create a temporary one that is automatically freed.
		ctx = std::shared_ptr<AVCodecContext>(avcodec_alloc_context3(factory->get_avcodec()), [](AVCodecContext* v) { avcodec_free_context(&v); });
		if (!ctx->priv_data) {
			return;
		}
	}

	auto p = obs_properties_add_list(props, S_CODEC_DNXHR_PROFILE, D_TRANSLATE(S_CODEC_DNXHR_PROFILE), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	streamfx::ffmpeg::tools::avoption_list_add_entries(ctx->priv_data, "profile", [&p](const AVOption* opt) {
		if (strcmp(opt->name, "dnxhd") == 0) {
			//Do not show DNxHD profile as it is outdated and should not be used.
			//It's also very picky about framerate and framesize combos, which makes it even less useful
			return;
		}

		//ffmpeg returns the profiles for DNxHR from highest to lowest.
		//Lowest to highest is what people usually expect.
		//Therefore, new entries will always be inserted at the top, effectively reversing the list
		obs_property_list_insert_string(p, 0, dnx_profile_to_display_name(opt->name), opt->name);
	});
}

void dnxhd::update(ffmpeg_factory* factory, ffmpeg_instance* instance, obs_data_t* settings)
{
	const char* profile = obs_data_get_string(settings, S_CODEC_DNXHR_PROFILE);
	av_opt_set(instance->get_avcodeccontext(), "profile", profile, AV_OPT_SEARCH_CHILDREN);
}

void dnxhd::log(ffmpeg_factory* factory, ffmpeg_instance* instance, obs_data_t* settings)
{
	DLOG_INFO("[%s]   Avid DNxHR:", factory->get_avcodec()->name);
	streamfx::ffmpeg::tools::print_av_option_string2(instance->get_avcodeccontext(), "profile", "    Profile", [](int64_t v, std::string_view o) { return std::string(o); });
}

void dnxhd::override_colorformat(ffmpeg_factory* factory, ffmpeg_instance* instance, obs_data_t* settings, AVPixelFormat& target_format)
{
	static const std::array<std::pair<const char*, AVPixelFormat>, static_cast<size_t>(5)> profile_to_format_map{std::pair{"dnxhr_lb", AV_PIX_FMT_YUV422P}, std::pair{"dnxhr_sq", AV_PIX_FMT_YUV422P}, std::pair{"dnxhr_hq", AV_PIX_FMT_YUV422P}, std::pair{"dnxhr_hqx", AV_PIX_FMT_YUV422P10}, std::pair{"dnxhr_444", AV_PIX_FMT_YUV444P10}};

	const char* selected_profile = obs_data_get_string(settings, S_CODEC_DNXHR_PROFILE);
	for (const auto& kv : profile_to_format_map) {
		if (strcmp(kv.first, selected_profile) == 0) {
			target_format = kv.second;
			return;
		}
	}

	// Fallback for (yet) unknown formats
	target_format = AV_PIX_FMT_YUV422P;
}

static auto inst = dnxhd();
