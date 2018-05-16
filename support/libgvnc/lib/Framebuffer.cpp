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
#include "libgvnc/Framebuffer.h"
#include "libgvnc/Encoder.h"
#include "libgvnc/ClientConnection.h"

#include <functional>
#include <map>
#include <cmath>

using namespace libgvnc;

Framebuffer::Framebuffer(uint16_t width, uint16_t height, FB_PixelFormat format) : width_(width), height_(height), pixel_format_(format)
{

}

std::vector<Rectangle> Framebuffer::ServeRequest(const struct fb_update_request& request, const struct FB_PixelFormat& target_format)
{
	std::vector<Rectangle> results;

	// For now, just create one big rectangle to return
	Rectangle output;
	output.Shape.X = request.x_pos;
	output.Shape.Y = request.y_pos;
	output.Shape.Width = request.width;
	output.Shape.Height = request.height;
	output.Encoding = EncodingType::Raw;

	FillRectangle(output, target_format, EncodingType::Raw);

	results.push_back(output);

	return results;
}

using encoder_factory_t = Encoder*(const FB_PixelFormat& format, const RectangleShape& shape);

static std::map<EncodingType, std::function<encoder_factory_t>> encoders
{
	std::make_pair(EncodingType::Raw, [](const FB_PixelFormat& format, const RectangleShape & shape)
	{
		return(Encoder*)new RawEncoder(format, shape);
	})
};

static int ilog2(int index)
{
	int targetlevel = 0;
	while (index >>= 1) ++targetlevel;
	return targetlevel;
}

class PixelConverter {
public:

	PixelConverter(const FB_PixelFormat &start, const FB_PixelFormat &end) : start_(start), end_(end)
	{
		uint32_t red_bits_start = ilog2(start_.red_max + 1);
		uint32_t green_bits_start = ilog2(start_.green_max + 1);
		uint32_t blue_bits_start = ilog2(start_.blue_max + 1);

		uint32_t red_bits_end = ilog2(end_.red_max + 1);
		uint32_t green_bits_end = ilog2(end_.green_max + 1);
		uint32_t blue_bits_end = ilog2(end_.blue_max + 1);

		red_shift_ = red_bits_end - red_bits_start + end_.red_shift;
		green_shift_ = green_bits_end - green_bits_start + end_.green_shift;
		blue_shift_ = blue_bits_end - blue_bits_start + end_.blue_shift;
	}

	uint32_t Convert(uint32_t data)
	{
		uint64_t true_red = ((data >> start_.red_shift) & start_.red_max) << red_shift_;
		uint64_t true_green = ((data >> start_.green_shift) & start_.green_max) << green_shift_;
		uint64_t true_blue = ((data >> start_.blue_shift) & start_.blue_max) << blue_shift_;

		return(true_red) | (true_green) | (true_blue);
	}

private:
	FB_PixelFormat start_, end_;
	int red_shift_, green_shift_, blue_shift_;
};

uint32_t Framebuffer::GetPixel(uint32_t x, uint32_t y) const
{
	uint32_t data = *(uint32_t*) ((char*) data_ + (y * GetWidth() * GetPixelFormat().bits_per_pixel / 8) + (x * GetPixelFormat().bits_per_pixel / 8));
	return data;
}

void Framebuffer::FillRectangle(Rectangle& rect, const FB_PixelFormat& target_format, EncodingType encoding)
{
	Encoder *encoder = encoders[encoding](target_format, rect.Shape);
	PixelConverter converter(GetPixelFormat(), target_format);

	std::vector<uint32_t> pixels;
	pixels.reserve(rect.Shape.Width * rect.Shape.Height);

	for (int y = 0; y < rect.Shape.Height; ++y) {
		for (int x = 0; x < rect.Shape.Width; ++x) {
			uint32_t pixeldata = GetPixel(x, y);
			pixels.push_back(pixeldata);
		}
	}

	for (auto &pxl : pixels) {
		pxl = converter.Convert(pxl);
	}

	rect.Data = encoder->Encode(pixels);

	delete encoder;
}
