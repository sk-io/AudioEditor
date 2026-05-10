#include "file_io.h"

#include "app.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}

#include <iostream>


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

	if (stream_index < 0) {
		show_error_box("could not find a valid audio stream");
		return false;
	}

	AVStream* stream = format_ctx->streams[stream_index];
	AVCodecParameters* params = stream->codecpar;

	AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
	ret = avcodec_open2(codec_ctx, codec, NULL);
	if (ret < 0) {
		show_error_box("dasjdnfnjdf");
		return false;
	}

	avcodec_parameters_to_context(codec_ctx, params);

	AVChannelLayout decode_layout;
	av_channel_layout_copy(&decode_layout, &codec_ctx->ch_layout);

	printf("Codec: %s\n", avcodec_get_name(params->codec_id));
	printf("Bitrate: %" PRId64 "\n", params->bit_rate);
	int sample_rate = params->sample_rate;
	printf("Sample rate: %d Hz\n", sample_rate);
	int num_channels = params->ch_layout.nb_channels;
	printf("Channels: %d\n", params->ch_layout.nb_channels);
	char buf[512];
	av_get_sample_fmt_string(buf, sizeof(buf), (AVSampleFormat) params->format);
	printf("Sample format: %d (%s)\n", params->format, buf);

	SwrContext* swr = nullptr;
	swr_alloc_set_opts2(
		&swr,
		&codec_ctx->ch_layout, AV_SAMPLE_FMT_FLT, codec_ctx->sample_rate,
		&codec_ctx->ch_layout, codec_ctx->sample_fmt, codec_ctx->sample_rate,
		0, NULL
	);
	swr_init(swr);

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
			int ret = avcodec_receive_frame(codec_ctx, frame);

			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;

			int num_samples = swr_get_out_samples(swr, frame->nb_samples);
			if (num_samples * 2 > sample_buf.size()) {
				sample_buf.resize(num_samples * 2);
			}

			uint8_t* arr[1] = {(uint8_t*) sample_buf.data()};

			int num_decoded = swr_convert(
				swr,
				arr,
				num_samples,
				(const uint8_t**) frame->extended_data,
				frame->nb_samples
			);

			if (num_decoded < 0) {
				show_error_box("error while decoding audio");
				return false;
			}

			//std::cout << "appending " << num_decoded << " samples\n";

			output_samples.insert(
				output_samples.end(),
				sample_buf.begin(),
				sample_buf.begin() + num_decoded * 2
			);
		}

		av_packet_unref(packet);
	}

	buffer.init(2, sample_rate, std::move(output_samples));

	av_frame_free(&frame);
	av_packet_free(&packet);
	swr_free(&swr);
	av_channel_layout_uninit(&decode_layout);
	avcodec_free_context(&codec_ctx);
	avformat_close_input(&format_ctx);

	return true;
}

bool FileIO::write(const AudioBuffer& buffer, const std::string& path, int format) {
	AVFormatContext* format_ctx = nullptr;

	avformat_alloc_output_context2(&format_ctx, NULL, NULL, path.c_str());
	if (!format_ctx) {
		show_error_box("failed to create ffmpeg output context");
		return false;
	}

	const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_MP3);
	if (!codec) {
		show_error_box("no MP3 encoder found");
		return false;
	}

	AVStream* stream = avformat_new_stream(format_ctx, NULL);
	AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);

	// TODO: avcodec_get_supported_config()

	codec_ctx->bit_rate = 192000;
	codec_ctx->sample_rate = 44100;
	codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLT;
	av_channel_layout_default(&codec_ctx->ch_layout, 2);
	codec_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

	if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
		show_error_box("failed to initialize codec");
		return false;
	}

	avcodec_parameters_from_context(stream->codecpar, codec_ctx);

	SwrContext* swr_ctx = nullptr;
	swr_alloc_set_opts2(
		&swr_ctx,
		&codec_ctx->ch_layout, codec_ctx->sample_fmt, codec_ctx->sample_rate,
		&codec_ctx->ch_layout, AV_SAMPLE_FMT_FLT, codec_ctx->sample_rate,
		0, NULL
	);
	swr_init(swr_ctx);

	if (~format_ctx->oformat->flags & AVFMT_NOFILE) {
		if (avio_open(&format_ctx->pb, path.c_str(), AVIO_FLAG_WRITE) < 0) {
			show_error_box("failed to open file");
			return false;
		}
	}

	if (avformat_write_header(format_ctx, NULL) < 0) {
		show_error_box("failed to write header");
		return false;
	}

	int frame_size = codec_ctx->frame_size;
	std::cout << "MP3 frame size = " << frame_size << std::endl;

	AVFrame* input_frame = av_frame_alloc();
	input_frame->nb_samples = frame_size;
	input_frame->format = AV_SAMPLE_FMT_FLT;
	av_channel_layout_copy(&input_frame->ch_layout, &codec_ctx->ch_layout);
	av_frame_get_buffer(input_frame, 0);

	AVFrame* encode_frame = av_frame_alloc();
	encode_frame->nb_samples = frame_size;
	encode_frame->format = AV_SAMPLE_FMT_FLT;
	av_channel_layout_copy(&encode_frame->ch_layout, &codec_ctx->ch_layout);
	av_frame_get_buffer(encode_frame, 0);

	AVPacket* packet = av_packet_alloc();

	const float* samples = buffer.get_samples().data();
	int64_t num_frames = buffer.get_num_frames();
	int64_t pts = 0;
	int64_t samples_written = 0;

	while (samples_written < num_frames) {
		av_frame_make_writable(input_frame);
		swr_convert_frame(swr_ctx, encode_frame, input_frame);

		encode_frame->pts = pts;
		pts += encode_frame->nb_samples;

		if (avcodec_send_frame(codec_ctx, input_frame) < 0) {
			show_error_box("error sending frame to encoder");
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
				show_error_box("error while saving mp3 file: yada yada");
				return false;
			}
		}

		samples_written += frame_size;
	}

	return true;
}
