#include <stdio.h>
#include <stdlib.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>

static const char* format = "/home/the108/video/output%d";

void write_file(int number, uint8_t* data, int size)
{
    FILE* f;
    char filename[256];
    sprintf(filename, format, number);
    f = fopen(filename, "wb+");
    fwrite(data, size, 1, f);
    fclose(f);
}

int main(int argc, char** argv) {

    AVFormatContext* context = avformat_alloc_context();
    int video_stream_index;

    av_log_set_level(AV_LOG_DEBUG);
    av_register_all();
    avcodec_register_all();
    //avformat_network_init();

    //open rtsp
    if (avformat_open_input(&context, "/home/the108/video/test.ts", NULL, NULL) != 0)
        return EXIT_FAILURE;

    if (avformat_find_stream_info(context, NULL) < 0)
        return EXIT_FAILURE;

    //search video stream
    for (int i = 0; i < context->nb_streams; i++)
    {
        // printf("%d:  codec_type = %d\n", i, context->streams[i]->codec->codec_type);
        if(context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            video_stream_index = i;
    }

    context->streams[video_stream_index]->codec->codec_tag = 0;

    // printf("CodecId: %d codec_tag: %d\n", context->streams[video_stream_index]->codec->codec_id,
        // context->streams[video_stream_index]->codec->codec_tag);

    AVPacket packet;
    av_init_packet(&packet);

    //open output file
    AVOutputFormat* fmt = av_guess_format(NULL, "test.flv", NULL);

    // printf("Long name: %s video codec: %d == %d\n", fmt->long_name, fmt->video_codec,
        // AV_CODEC_ID_H264);

    AVFormatContext* oc = avformat_alloc_context();
    oc->oformat = fmt;
    //avio_open2(&oc->pb, "test.flv", AVIO_FLAG_WRITE, NULL, NULL);

    int buffer_size = 4 * 1024 * 1024;
    unsigned char* buffer = av_malloc(buffer_size);
    AVIOContext* buffer_context = avio_alloc_context(buffer, buffer_size, 1, NULL, NULL, NULL, NULL);

    oc->pb = buffer_context;
    oc->flags = AVFMT_FLAG_CUSTOM_IO;

    AVStream* stream = NULL;
    //start reading packets from stream and write them to file

    int cnt = 0;

    while (av_read_frame(context, &packet) >= 0) 
    {
        if (packet.stream_index == video_stream_index)
        {
            printf("pts %ld duration %d size %d\n", packet.pts, packet.duration, packet.size);
            /*if (packet.size > 0)
            {
                write_file(cnt++, packet.data, packet.size);
            }*/

            if (stream == NULL)
            {//create stream in file
                stream = avformat_new_stream(oc, context->streams[video_stream_index]->codec->codec);
                // printf("Copy context\n");
                avcodec_copy_context(stream->codec, context->streams[video_stream_index]->codec);
                stream->sample_aspect_ratio = context->streams[video_stream_index]->codec->sample_aspect_ratio;
                // printf("Write header\n");

                avformat_write_header(oc, NULL);
            }
            packet.stream_index = stream->id;

            // printf("Write frame %d\n", cnt++);

            packet.pts = av_rescale_q(packet.pts, context->streams[video_stream_index]->codec->time_base, stream->time_base);
            packet.dts = av_rescale_q(packet.dts, context->streams[video_stream_index]->codec->time_base, stream->time_base);

            packet.pts *= context->streams[video_stream_index]->codec->ticks_per_frame;
            packet.dts *= context->streams[video_stream_index]->codec->ticks_per_frame;

            //av_write_frame(oc, &packet);
            av_interleaved_write_frame(oc, &packet);
            // printf("Write frame end\n");
        }
        av_free_packet(&packet);
        av_init_packet(&packet);
    }

    av_free_packet(&packet);
    av_read_pause(context);
    av_write_trailer(oc);

    write_file(0, buffer, oc->pb->pos);

    //avio_close();
    av_free(buffer);
    av_free(oc->pb);
    avformat_free_context(oc);

    return (EXIT_SUCCESS);
}