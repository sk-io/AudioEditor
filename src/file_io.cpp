#include "file_io.h"

#include "app.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <algorithm>

// TODO: refactor error checking and reporting
static void print_error_msg(int err) {
	char buf[AV_ERROR_MAX_STRING_SIZE];
	av_strerror(err, buf, sizeof(buf));
	std::cout << "ffmpeg error msg: " << buf << std::endl;
}

// TODO: refactor error checking and reporting
bool FileIO::read(AudioBuffer& buffer, const std::string& path) {
	int ret;

	AVFormatContext* format_ctx = NULL;
	ret = avformat_open_input(&format_ctx, path.c_str(), NULL, NULL);
	if (ret < 0) {
		show_error_box(QString("failed to open file %0").arg(path));
		return false;
	}

	avformat_find_stream_info(format_ctx, NULL);

	std::cout << "num audio streams: " << format_ctx->nb_streams << std::endl;

	const AVCodec* codec = NULL;
	int stream_index = av_find_best_stream(
		format_ctx,
		AVMEDIA_TYPE_AUDIO,
		-1,
		-1,
		&codec,
		0
	);

	std::cout << "chose stream " << stream_index << "\n";

	if (stream_index < 0) {
		show_error_box("could not find a valid audio stream");
		return false;
	}

	AVStream* stream = format_ctx->streams[stream_index];
	AVCodecParameters* params = stream->codecpar;

	int sample_rate = params->sample_rate;
	int channels = params->ch_layout.nb_channels;

	if (channels < 0 || channels > 2) {
		show_error_box("invalid number of channels");
		return false;
	}

	printf("Codec: %s\n", avcodec_get_name(params->codec_id));
	printf("Bitrate: %" PRId64 "\n", params->bit_rate);
	printf("Sample rate: %d Hz\n", params->sample_rate);
	printf("Channels: %d\n", params->ch_layout.nb_channels);
	char buf[512];
	av_get_sample_fmt_string(buf, sizeof(buf), (AVSampleFormat) params->format);
	printf("Sample format: %d (%s)\n", params->format, buf);

	AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(codec_ctx, params);
	ret = avcodec_open2(codec_ctx, codec, NULL);
	if (ret < 0) {
		print_error_msg(ret);
		show_error_box("failed to open codec");
		return false;
	}

	AVChannelLayout decode_layout;
	av_channel_layout_copy(&decode_layout, &codec_ctx->ch_layout);

	SwrContext* swr = NULL;

	ret = swr_alloc_set_opts2(
		&swr,
		&codec_ctx->ch_layout, AV_SAMPLE_FMT_FLT, codec_ctx->sample_rate,
		&codec_ctx->ch_layout, codec_ctx->sample_fmt, codec_ctx->sample_rate,
		0, NULL
	);
	if (!swr) {
		show_error_box("swr_alloc_set_opts2");
		return false;
	}

	ret = swr_init(swr);
	if (ret < 0) {
		show_error_box("swr_init");
		return false;
	}

	std::vector<float> output_samples, sample_buf;

	AVFrame* frame = av_frame_alloc();

	AVPacket* packet = av_packet_alloc();
	while (av_read_frame(format_ctx, packet) >= 0) {
		if (packet->stream_index != stream_index) {
			av_packet_unref(packet);
			continue;
		}

		avcodec_send_packet(codec_ctx, packet);

		for (;;) {
			ret = avcodec_receive_frame(codec_ctx, frame);

			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;

			int num_samples = swr_get_out_samples(swr, frame->nb_samples);
			if (num_samples * channels > sample_buf.size())
				sample_buf.resize(num_samples * channels);

			uint8_t* arr[1] = {(uint8_t*) sample_buf.data()};

			int num_converted = swr_convert(
				swr,
				arr,
				num_samples,
				(const uint8_t**) frame->extended_data,
				frame->nb_samples
			);

			if (num_converted < 0) {
				std::cout << ret << std::endl;
				char buf[AV_ERROR_MAX_STRING_SIZE];
				av_strerror(ret, buf, sizeof(buf));
				std::cout << buf << std::endl;
				show_error_box("failed to convert");
			}

			output_samples.insert(
				output_samples.end(),
				sample_buf.begin(),
				sample_buf.begin() + num_converted * channels
			);
		}

		av_packet_unref(packet);
	}

	buffer.init(channels, codec_ctx->sample_rate, std::move(output_samples));

	av_frame_free(&frame);
	av_packet_free(&packet);
	swr_free(&swr);
	av_channel_layout_uninit(&decode_layout);
	avcodec_free_context(&codec_ctx);
	avformat_close_input(&format_ctx);

	return true;
}

static void print_swr_current_values(SwrContext *swr) {
    const AVOption *opt = NULL;
    
    printf("%-20s | %-20s\n", "Option Name", "Current Value");
    printf("----------------------------------------------------\n");

    while ((opt = av_opt_next(swr, opt))) {
        if (opt->type == AV_OPT_TYPE_CONST) continue;

        uint8_t *val = NULL;
        if (av_opt_get(swr, opt->name, 0, &val) >= 0) {
            printf("%-20s | %-20s\n", opt->name, val);
            
            av_free(val);
        }
    }
}

// TODO: refactor error checking and reporting
bool FileIO::write(const AudioBuffer& buffer, const std::string& path, int format) {
	int ret;

	AVFormatContext* format_ctx = nullptr;

	ret = avformat_alloc_output_context2(&format_ctx, NULL, NULL, path.c_str());
	if (!format_ctx) {
		print_error_msg(ret);
		return false;
	}

	const AVCodec* codec = avcodec_find_encoder((AVCodecID) format);
	if (!codec) {
		return false;
	}

	AVStream* stream = avformat_new_stream(format_ctx, NULL);
	AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);

	// TODO: avcodec_get_supported_config()

	codec_ctx->bit_rate = 192000;
	codec_ctx->sample_rate = buffer.get_sample_rate();
	codec_ctx->sample_fmt = codec->sample_fmts[0];
	av_channel_layout_default(&codec_ctx->ch_layout, 2);
	codec_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

	ret = avcodec_open2(codec_ctx, codec, NULL);
	if (ret < 0) {
		print_error_msg(ret);
		return false;
	}

	avcodec_parameters_from_context(stream->codecpar, codec_ctx);

	SwrContext* swr = nullptr;
	swr_alloc_set_opts2(
		&swr,

		&codec_ctx->ch_layout,
		codec_ctx->sample_fmt,
		codec_ctx->sample_rate,

		&codec_ctx->ch_layout,
		AV_SAMPLE_FMT_FLT,
		codec_ctx->sample_rate,

		0,
		NULL
	);
	swr_init(swr);

	//print_swr_current_values(swr);

	if (~format_ctx->oformat->flags & AVFMT_NOFILE) {
		if (avio_open(&format_ctx->pb, path.c_str(), AVIO_FLAG_WRITE) < 0) {
			return false;
		}
	}

	ret = avformat_write_header(format_ctx, NULL);
	if (ret < 0) {
		print_error_msg(ret);
		return false;
	}

	AVFrame* frame = av_frame_alloc();
	frame->format = codec_ctx->sample_fmt;
	frame->nb_samples = codec_ctx->frame_size;
	frame->sample_rate = buffer.get_sample_rate();
	frame->ch_layout = codec_ctx->ch_layout;

	ret = av_frame_get_buffer(frame, 0);
	if (ret < 0) {
		print_error_msg(ret);
		return false;
	}

	assert(ret >= 0);

	AVPacket* packet = av_packet_alloc();

	const float* samples = buffer.get_samples().data();
	int num_frames = buffer.get_num_frames();
	int pts = 0;

	while (pts < num_frames) {
		av_frame_make_writable(frame);

		int remaining = num_frames - pts;
		int write_count = std::min(remaining, frame->nb_samples);

		const uint8_t* in_arr[1] = {
			(const uint8_t*) (samples + pts * 2)
		};

		ret = swr_convert(
			swr,
			frame->data,
			frame->nb_samples,
			in_arr,
			write_count
		);

		if (ret < 0) {
			print_error_msg(ret);
			return false;
		}

		frame->pts = pts;
		pts += write_count;

		ret = avcodec_send_frame(codec_ctx, frame);
		if (ret < 0) {
			print_error_msg(ret);
			return false;
		}

		for (;;) {
			int ret = avcodec_receive_packet(codec_ctx, packet);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;

			av_packet_rescale_ts(packet, codec_ctx->time_base, stream->time_base);
			packet->stream_index = stream->index;

			ret = av_interleaved_write_frame(format_ctx, packet);
			av_packet_unref(packet);

			if (ret < 0) {
				print_error_msg(ret);
				return false;
			}
		}
	}

	av_frame_free(&frame);
	av_packet_free(&packet);
	swr_free(&swr);
	avcodec_free_context(&codec_ctx);
	avformat_close_input(&format_ctx);

	return true;
}
