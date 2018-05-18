/*
 * Copyright (C) 2018 Harry Wagstaff
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace libgvnc {
    struct FB_PixelFormat {

        FB_PixelFormat() {
        }

        FB_PixelFormat(uint8_t bpp, uint8_t depth, uint8_t be, uint8_t truecolor, uint8_t red_max, uint8_t green_max, uint8_t blue_max, uint8_t red_shift, uint8_t green_shift, uint8_t blue_shift)
        : bits_per_pixel(bpp), depth(depth), big_endian(be), true_color(truecolor), red_max(red_max), green_max(green_max), blue_max(blue_max), red_shift(red_shift), green_shift(green_shift), blue_shift(blue_shift) {
        }

        uint8_t bits_per_pixel;
        uint8_t depth;
        uint8_t big_endian;
        uint8_t true_color;
        uint16_t red_max;
        uint16_t green_max;
        uint16_t blue_max;
        uint8_t red_shift;
        uint8_t green_shift;
        uint8_t blue_shift;

        uint8_t padding[3];
    };

    enum class EncodingType : int32_t {
        Raw = 0
    };

    class RectangleShape {
    public:
        uint16_t X, Y, Width, Height;
    };

    class Rectangle {
    public:
        RectangleShape Shape;
        EncodingType Encoding;

        std::vector<char> Data;
    };

    class Framebuffer {
    public:
        Framebuffer(uint16_t width, uint16_t height, FB_PixelFormat format);

        uint16_t GetHeight() const {
            return height_;
        }

        uint16_t GetWidth() const {
            return width_;
        }

        const struct FB_PixelFormat &GetPixelFormat() const {
            return pixel_format_;
        }

        const std::string &GetTitle() const {
            return title_;
        }

        void SetData(void *data) {
            data_ = data;
        }

        const void *GetData() const {
            return data_;
        }

        std::vector<Rectangle> ServeRequest(const struct fb_update_request &request, const struct FB_PixelFormat &target_format);
        void FillRectangle(Rectangle &rect, const struct FB_PixelFormat &target_format, EncodingType encoding);

        uint32_t GetPixel(uint32_t x, uint32_t y) const;

    private:
        uint16_t height_, width_;
        std::string title_;
        struct FB_PixelFormat pixel_format_;

        void *data_;
    };
}
