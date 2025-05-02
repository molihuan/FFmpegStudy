#include "MergeAudioVideo.h"

using namespace std;

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

// 查找指定类型的流
static int find_stream_index(AVFormatContext* format_ctx, AVMediaType type) {
    for (int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == type) {
            return i;
        }
    }
    return -1;
}

bool open_input_file(AVFormatContext** ctx, const char* file) {
    if (avformat_open_input(ctx, file, nullptr, nullptr) < 0) {
        cerr << "Error opening input file: " << file << endl;
        return false;
    }

    if (avformat_find_stream_info(*ctx, nullptr) < 0) {
        cerr << "Error finding stream info for file: " << file << endl;
        avformat_close_input(ctx);
        return false;
    }

    return true;
}

bool add_stream_to_output(AVFormatContext* output_ctx, AVFormatContext* input_ctx,
                          AVMediaType type, AVStream** out_stream) {
    int stream_index = find_stream_index(input_ctx, type);
    if (stream_index < 0) {
        cerr << "Error finding stream of type " << type << endl;
        return false;
    }

    AVStream* in_stream = input_ctx->streams[stream_index];
    AVCodec* codec = avcodec_find_decoder(in_stream->codecpar->codec_id);
    if (!codec) {
        cerr << "Error finding decoder for stream type " << type << endl;
        return false;
    }

    *out_stream = avformat_new_stream(output_ctx, codec);
    if (!*out_stream) {
        cerr << "Error creating output stream for type " << type << endl;
        return false;
    }

    if (avcodec_parameters_copy((*out_stream)->codecpar, in_stream->codecpar) < 0) {
        cerr << "Error copying codec parameters for stream type " << type << endl;
        return false;
    }

    (*out_stream)->time_base = in_stream->time_base;
    if (type == AVMEDIA_TYPE_VIDEO) {
        ((AVStream*)*out_stream)->r_frame_rate = ((AVStream*)in_stream)->r_frame_rate;
    }

    return true;
}

bool write_output_file(AVFormatContext* output_ctx, const char* output_file,
                       AVFormatContext* input_video_ctx, AVFormatContext* input_audio_ctx) {
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        cerr << "Failed to allocate packet" << endl;
        return false;
    }

    // 打开输出文件
    if (!(output_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_ctx->pb, output_file, AVIO_FLAG_WRITE) < 0) {
            cerr << "Error opening output file: " << output_file << endl;
            av_packet_free(&packet);
            return false;
        }
    }

    if (avformat_write_header(output_ctx, nullptr) < 0) {
        cerr << "Error writing header to output file" << endl;
        av_packet_free(&packet);
        return false;
    }

    // 复制视频数据包
    AVStream* video_out_stream = output_ctx->streams[0];
    AVStream* video_in_stream = input_video_ctx->streams[find_stream_index(input_video_ctx, AVMEDIA_TYPE_VIDEO)];
    while (av_read_frame(input_video_ctx, packet) >= 0) {
        if (packet->stream_index == find_stream_index(input_video_ctx, AVMEDIA_TYPE_VIDEO)) {
            packet->pts = av_rescale_q(packet->pts, video_in_stream->time_base, video_out_stream->time_base);
            packet->dts = av_rescale_q(packet->dts, video_in_stream->time_base, video_out_stream->time_base);
            packet->duration = av_rescale_q(packet->duration, video_in_stream->time_base, video_out_stream->time_base);
            packet->stream_index = video_out_stream->index;

            if (av_interleaved_write_frame(output_ctx, packet) < 0) {
                cerr << "Error writing video frame" << endl;
                av_packet_unref(packet);
                av_packet_free(&packet);
                return false;
            }
        }
        av_packet_unref(packet);
    }

    // 复制音频数据包
    AVStream* audio_out_stream = output_ctx->streams[1];
    AVStream* audio_in_stream = input_audio_ctx->streams[find_stream_index(input_audio_ctx, AVMEDIA_TYPE_AUDIO)];
    while (av_read_frame(input_audio_ctx, packet) >= 0) {
        if (packet->stream_index == find_stream_index(input_audio_ctx, AVMEDIA_TYPE_AUDIO)) {
            packet->pts = av_rescale_q(packet->pts, audio_in_stream->time_base, audio_out_stream->time_base);
            packet->dts = av_rescale_q(packet->dts, audio_in_stream->time_base, audio_out_stream->time_base);
            packet->duration = av_rescale_q(packet->duration, audio_in_stream->time_base, audio_out_stream->time_base);
            packet->stream_index = audio_out_stream->index;

            if (av_interleaved_write_frame(output_ctx, packet) < 0) {
                cerr << "Error writing audio frame" << endl;
                av_packet_unref(packet);
                av_packet_free(&packet);
                return false;
            }
        }
        av_packet_unref(packet);
    }

    av_packet_free(&packet);

    // 写入尾部信息
    if (av_write_trailer(output_ctx) < 0) {
        cerr << "Error writing trailer to output file" << endl;
        return false;
    }

    return true;
}

void cleanup_resources(AVFormatContext* input_video_ctx, AVFormatContext* input_audio_ctx,
                       AVFormatContext* output_ctx) {
    if (input_video_ctx) {
        avformat_close_input(&input_video_ctx);
    }
    if (input_audio_ctx) {
        avformat_close_input(&input_audio_ctx);
    }
    if (output_ctx) {
        if (output_ctx->pb) {
            avio_closep(&output_ctx->pb);
        }
        avformat_free_context(output_ctx);
    }
}

int main() {
    const char* video_file = "D:/DesktopSpace/Development/Cplusplus/Project/FFmpegStudy/MergeAudioVideo/TestRes/download/666666/1/80/video.m4s";
    const char* audio_file = "D:/DesktopSpace/Development/Cplusplus/Project/FFmpegStudy/MergeAudioVideo/TestRes/download/666666/1/80/audio.m4s";
    const char* output_file = "D:/DesktopSpace/Development/Cplusplus/Project/FFmpegStudy/MergeAudioVideo/TestRes/download/666666/1/80/output.mp4";

    // 初始化FFmpeg库
    av_register_all();
    //avformat_network_init();

    AVFormatContext* input_video_ctx = nullptr;
    AVFormatContext* input_audio_ctx = nullptr;
    AVFormatContext* output_ctx = nullptr;

    // 创建输出上下文
    if (avformat_alloc_output_context2(&output_ctx, nullptr, nullptr, output_file) < 0) {
        cerr << "Error creating output context" << endl;
        return -1;
    }

    // 打开输入文件
    if (!open_input_file(&input_video_ctx, video_file) ||
        !open_input_file(&input_audio_ctx, audio_file)) {
        cleanup_resources(input_video_ctx, input_audio_ctx, output_ctx);
        return -1;
    }

    // 添加音视频流到输出
    AVStream* out_video_stream = nullptr;
    AVStream* out_audio_stream = nullptr;
    if (!add_stream_to_output(output_ctx, input_video_ctx, AVMEDIA_TYPE_VIDEO, &out_video_stream) ||
        !add_stream_to_output(output_ctx, input_audio_ctx, AVMEDIA_TYPE_AUDIO, &out_audio_stream)) {
        cleanup_resources(input_video_ctx, input_audio_ctx, output_ctx);
        return -1;
    }

    // 写入输出文件
    if (!write_output_file(output_ctx, output_file, input_video_ctx, input_audio_ctx)) {
        cleanup_resources(input_video_ctx, input_audio_ctx, output_ctx);
        return -1;
    }

    cleanup_resources(input_video_ctx, input_audio_ctx, output_ctx);

    cout << "Video and audio streams merged successfully!" << endl;
    return 0;
}