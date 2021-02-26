#include <bits/stdc++.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
};

// 对每一帧进行编码
static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,
                   FILE *outfile)
{
    int ret;
    /* send the frame to the encoder */
    if (frame)
        printf("Send frame %3" PRId64 "\n", frame->pts);
    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0)
    {
        fprintf(stderr, "Error sending a frame for encoding\n");
        exit(1);
    }
    while (ret >= 0)
    {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0)
        {
            fprintf(stderr, "Error during encoding\n");
            exit(1);
        }
        printf("Write packet %3" PRId64 " (size=%5d)\n", pkt->pts, pkt->size);
        fwrite(pkt->data, 1, pkt->size, outfile);
        av_packet_unref(pkt);
    }
}

int encode_video(const char *filename, const char *codec_name)
{

    //编码器
    const AVCodec *codec;
    //编码器上下文
    AVCodecContext *c = NULL;
    //got_output 用于标记一帧是否压缩成功
    int i, ret, x, y, got_output;
    FILE *f;
    //存放解码后的原始帧（未压缩的数据）
    AVFrame *frame;
    AVPacket pkt;
    uint8_t endcode[] = {0, 0, 1, 0xb7};

    //将所有需要的编解码，多媒体格式，以及网络，都注册要程序里
    avcodec_register_all();

    /* find the mpeg1video encoder */
    //通过编解码名找到对应的编解码器
    codec = avcodec_find_encoder_by_name(codec_name);
    if (!codec)
    {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }
    // 根据编码器，创建相对应的编码器上下文
    c = avcodec_alloc_context3(codec);
    if (!c)
    {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }
    //设置相关参数

    /* put sample parameters */
    //码率，400kb
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = 352;
    c->height = 288;
    /* frames per second */
    //时间基，每一秒24帧，每一刻度24分之1(时间基根据帧率而变化)
    c->time_base = (AVRational){1, 24};
    //帧率
    c->framerate = (AVRational){24, 1};

    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    //多少帧产生一组关键帧
    c->gop_size = 10;
    //b帧，参考帧
    c->max_b_frames = 1;
    //编码的原始数据的YUV格式
    c->pix_fmt = AV_PIX_FMT_YUV420P;

    //如果编码器id 是 h264
    if (codec->id == AV_CODEC_ID_H264)
        // preset表示采用一个预先设定好的h264参数集，级别是slow，slow表示压缩速度是慢的，慢的可以保证视频质量，用快的会降低视频质量
        av_opt_set(c->priv_data, "preset", "slow", 0);

    /* open it */
    //打开编码器
    if (avcodec_open2(c, codec, NULL) < 0)
    {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    //打开输入文件
    f = fopen(filename, "wb");
    if (!f)
    {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    //初始化帧并设置帧的YUV格式和分辨率
    frame = av_frame_alloc();
    if (!frame)
    {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;

    //为音频或视频数据分配新的缓冲区
    ret = av_frame_get_buffer(frame, 32);
    if (ret < 0)
    {
        fprintf(stderr, "Could not allocate the video frame data\n");
        exit(1);
    }

    /* encode 10 second of video */
    for (i = 0; i < 240; i++)
    {
        //初始化packet
        av_init_packet(&pkt);
        pkt.data = NULL; // packet data will be allocated by the encoder
        pkt.size = 0;
        // 强制输出写入文件
        fflush(stdout);

        /* make sure the frame data is writable */
        //确保帧被写入
        ret = av_frame_make_writable(frame);
        if (ret < 0)
            exit(1);

        // 下面2个循环是人工往frame里面添的数据
        /* prepare a dummy image */
        /* Y */
        for (y = 0; y < c->height; y++)
        {
            for (x = 0; x < c->width; x++)
            {
                frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
            }
        }

        /* Cb and Cr */
        for (y = 0; y < c->height / 2; y++)
        {
            for (x = 0; x < c->width / 2; x++)
            {
                frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
                frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
            }
        }

        frame->pts = i;
        encode(c, frame, &pkt, f);
    }
    // 进行编码压缩
    encode(c, frame, &pkt, f);
    /* add sequence end code to have a real MPEG file */
    fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);

    avcodec_free_context(&c);
    av_frame_free(&frame);

    return 0;
}

int main(int argc, char *argv[])
{
    encode_video("output.mp4", "libx264");
}