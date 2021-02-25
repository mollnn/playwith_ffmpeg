#ifndef _COLOR_HPP_
#define _COLOR_HPP_

#include <bits/stdc++.h>
#include "vec3.hpp"

double fClamp(double x)
{
    x = std::min(x, 1.0);
    x = std::max(x, 0.0);
    return x;
}

vec3 colorClamp(vec3 c)
{
    c.x = fClamp(c.x);
    c.y = fClamp(c.y);
    c.z = fClamp(c.z);
    return c;
}

double fDeGamma(double x, double gamma = 2.2)
{
    return pow(x, 1.0 / gamma);
}

vec3 colorDeGamma(vec3 c, double gamma = 2.2)
{
    c.x = fDeGamma(c.x, gamma);
    c.y = fDeGamma(c.y, gamma);
    c.z = fDeGamma(c.z, gamma);
    return c;
}

double fEnGamma(double x, double gamma = 2.2)
{
    return pow(x, gamma);
}

vec3 colorEnGamma(vec3 c, double gamma = 2.2)
{
    c.x = fEnGamma(c.x, gamma);
    c.y = fEnGamma(c.y, gamma);
    c.z = fEnGamma(c.z, gamma);
    return c;
}

uint8_t fFloatToUint8(double f)
{
    return 255 * (f + 1e-6);
}

std::tuple<uint8_t, uint8_t, uint8_t> colorFloatToUint8(vec3 c)
{
    return make_tuple(fFloatToUint8(c.x), fFloatToUint8(c.y), fFloatToUint8(c.z));
}

double fUint8ToFloat(uint8_t f)
{
    return f * 1.0 / 255;
}

vec3 colorUint8ToFloat(uint8_t r, uint8_t g, uint8_t b)
{
    return {fUint8ToFloat(r), fUint8ToFloat(g), fUint8ToFloat(b)};
}

/**
 * @brief 将 x 限制在 [l,r] 内
 */
int bound(int l, int x, int r)
{
    return std::min(r, std::max(l, x));
}

/**
 * @brief yuv2rgb
 * @param y [0, 255]
 * @param u [0, 255]
 * @param v [0, 255]
 * @return #ABGR
 */
vec3 yuv2rgb(uint8_t y, uint8_t u, uint8_t v)
{
    int R = (y + 1.403 * (v - 128));
    int G = (y - 0.343 * (u - 128) - 0.714 * (v - 128));
    int B = (y + 1.770 * (u - 128));

    R = bound(0, R, 255);
    G = bound(0, G, 255);
    B = bound(0, B, 255);

    return {R, G, B};
}

#endif