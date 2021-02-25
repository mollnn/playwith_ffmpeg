#include <bits/stdc++.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <SDL2/SDL.h>
#include <libavutil/imgutils.h>
};

#include "image.hpp"

int main(int argc, char *argv[])
{
	AVFormatContext *p_format_context;
	int i, videoindex;
	AVCodecContext *p_codec_context;
	AVCodec *p_codec;
	AVFrame *p_frame, *p_frame_yuv;
	unsigned char *out_buffer;
	AVPacket *packet;
	int y_size;
	int flag_ret, flag_got_picture;
	struct SwsContext *img_convert_context;

	auto filepath = argv[1];

	int screen_w = 0, screen_h = 0;
	SDL_Window *screen;
	SDL_Renderer *sdl_renderer;
	SDL_Texture *sdl_texture;
	SDL_Rect sdl_rect;

	FILE *fp_yuv;

	av_register_all();
	avformat_network_init();
	p_format_context = avformat_alloc_context();

	if (avformat_open_input(&p_format_context, filepath, NULL, NULL) != 0)
	{
		printf("Couldn't open input stream.\n");
		return -1;
	}
	if (avformat_find_stream_info(p_format_context, NULL) < 0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex = -1;
	for (i = 0; i < p_format_context->nb_streams; i++)
		if (p_format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
	if (videoindex == -1)
	{
		printf("Didn't find a video stream.\n");
		return -1;
	}

	p_codec_context = p_format_context->streams[videoindex]->codec;
	p_codec = avcodec_find_decoder(p_codec_context->codec_id);
	if (p_codec == NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	if (avcodec_open2(p_codec_context, p_codec, NULL) < 0)
	{
		printf("Could not open codec.\n");
		return -1;
	}

	p_frame = av_frame_alloc();
	p_frame_yuv = av_frame_alloc();
	out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, p_codec_context->width, p_codec_context->height, 1));
	av_image_fill_arrays(p_frame_yuv->data, p_frame_yuv->linesize, out_buffer,
						 AV_PIX_FMT_YUV420P, p_codec_context->width, p_codec_context->height, 1);

	packet = (AVPacket *)av_malloc(sizeof(AVPacket));

	av_dump_format(p_format_context, 0, filepath, 0);
	img_convert_context = sws_getContext(p_codec_context->width, p_codec_context->height, p_codec_context->pix_fmt,
										 p_codec_context->width, p_codec_context->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}

	screen_w = p_codec_context->width;
	screen_h = p_codec_context->height;

	screen = SDL_CreateWindow("Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
							  screen_w, screen_h,
							  SDL_WINDOW_OPENGL);

	if (!screen)
	{
		printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
		return -1;
	}

	sdl_renderer = SDL_CreateRenderer(screen, -1, 0);
	sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, p_codec_context->width, p_codec_context->height);
	sdl_rect.x = 0;
	sdl_rect.y = 0;
	sdl_rect.w = screen_w;
	sdl_rect.h = screen_h;

	bool flag_running = true;

	while (av_read_frame(p_format_context, packet) >= 0 && flag_running)
	{
		if (packet->stream_index == videoindex)
		{
			flag_ret = avcodec_decode_video2(p_codec_context, p_frame, &flag_got_picture, packet);
			if (flag_ret < 0)
			{
				printf("Decode Error.\n");
				return -1;
			}
			if (flag_got_picture)
			{
				sws_scale(img_convert_context, (const unsigned char *const *)p_frame->data, p_frame->linesize, 0, p_codec_context->height,
						  p_frame_yuv->data, p_frame_yuv->linesize);

				// SDL_UpdateYUVTexture(sdl_texture, &sdl_rect,
				// 					 p_frame_yuv->data[0], p_frame_yuv->linesize[0],
				// 					 p_frame_yuv->data[1], p_frame_yuv->linesize[1],
				// 					 p_frame_yuv->data[2], p_frame_yuv->linesize[2]);

				size_t image_h = p_codec_context->height;
				size_t image_w = p_codec_context->width;

				Image image(image_w, image_h);

				for (int y = 0; y < image_h; y++)
				{
					for (int x = 0; x < image_w; x++)
					{
						int y2 = y / 2;
						int x2 = x / 2;

						uint8_t Y = p_frame_yuv->data[0][y * p_frame_yuv->linesize[0] + x];
						uint8_t Cb = p_frame_yuv->data[1][y2 * p_frame_yuv->linesize[1] + x2];
						uint8_t Cr = p_frame_yuv->data[2][y2 * p_frame_yuv->linesize[2] + x2];

						image.Set(x, y, yuv2rgb(Y, Cb, Cr));
					}
				}

				uint8_t *pixels = new uint8_t[image_h * image_w * 4];
				for (int y = 0; y < image_h; y++)
				{
					for (int x = 0; x < image_w; x++)
					{
						vec3 vec = image.Get(x, y);

						uint8_t r = vec.x;
						uint8_t g = vec.y;
						uint8_t b = vec.z;

						pixels[y * image_w * 4 + x * 4 + 0] = 0;
						pixels[y * image_w * 4 + x * 4 + 1] = b;
						pixels[y * image_w * 4 + x * 4 + 2] = g;
						pixels[y * image_w * 4 + x * 4 + 3] = r;
					}
				}

				SDL_UpdateTexture(sdl_texture, &sdl_rect, pixels, image_w * 4);

				delete[] pixels;

				SDL_RenderClear(sdl_renderer);
				SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, &sdl_rect);
				SDL_RenderPresent(sdl_renderer);
				// SDL_Delay(10);
			}
		}
		av_free_packet(packet);

		SDL_Event sdl_event;
		while (SDL_PollEvent(&sdl_event))
		{
			if (sdl_event.type == SDL_QUIT)
			{
				flag_running = false;
			}
			if (sdl_event.type == SDL_KEYUP)
			{
				if (sdl_event.key.keysym.sym == SDLK_ESCAPE)
				{
					flag_running = false;
				}
			}
		}
	}

	if (flag_running)
	{
		while (1)
		{
			flag_ret = avcodec_decode_video2(p_codec_context, p_frame, &flag_got_picture, packet);
			if (flag_ret < 0)
				break;
			if (!flag_got_picture)
				break;
			sws_scale(img_convert_context, (const unsigned char *const *)p_frame->data, p_frame->linesize, 0, p_codec_context->height,
					  p_frame_yuv->data, p_frame_yuv->linesize);
			//SDL---------------------------
			SDL_UpdateTexture(sdl_texture, &sdl_rect, p_frame_yuv->data[0], p_frame_yuv->linesize[0]);
			SDL_RenderClear(sdl_renderer);
			SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, &sdl_rect);
			SDL_RenderPresent(sdl_renderer);
			//SDL End-----------------------
		}
	}

	sws_freeContext(img_convert_context);

	SDL_Quit();

	av_frame_free(&p_frame_yuv);
	av_frame_free(&p_frame);
	avcodec_close(p_codec_context);
	avformat_close_input(&p_format_context);

	std::cout << "exited." << std::endl;

	return 0;
}