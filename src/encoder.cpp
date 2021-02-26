#include <bits/stdc++.h>

#include "video.hpp"

int main(int argc, char *argv[])
{
    int width = 1920;
    int height = 804;
    VideoEncoder video_encoder("output.mp4", "libx264", width, height, 1000000, 23.976, 10);

    Image_RGB888 image(width, height);
    for (int i = 0; i < 100; i++)
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                uint8_t r = (y + i) % 200;
                uint8_t g = (x + i) % 200;
                uint8_t b = i * 2;
                image.Set(x, y, {r, g, b});
            }
        }
        video_encoder.NewFrame(image);
    }

    video_encoder.End();
}