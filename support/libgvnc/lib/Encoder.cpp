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
#include "libgvnc/Encoder.h"

#include <stdexcept>

using namespace libgvnc;

Encoder::Encoder(const FB_PixelFormat& format, const RectangleShape& shape) : encoded_format_(format), shape_(shape)
{

}

RawEncoder::RawEncoder(const FB_PixelFormat& format, const RectangleShape& shape) : Encoder(format, shape)
{

}

std::vector<char> RawEncoder::Encode(const std::vector<uint32_t> &pixels)
{
	std::vector<char> data(GetShape().Width * GetShape().Height * GetFormat().bits_per_pixel / 8);
	uint32_t data_ptr = 0;
	for (auto pixel_data : pixels) {
		switch (GetFormat().bits_per_pixel) {
		case 8:
			data[data_ptr++] = pixel_data;
			break;
		case 16:
			data[data_ptr++] = pixel_data >> 8;
			data[data_ptr++] = pixel_data;
			break;
		case 24:
			data[data_ptr++] = pixel_data >> 16;
			data[data_ptr++] = pixel_data >> 8;
			data[data_ptr++] = pixel_data;
			break;
		case 32:
			*(uint32_t*) (data.data() + data_ptr) = pixel_data;
			data_ptr += 4;
			break;

		default:
			throw std::logic_error("Unknown pixel depth");
		}
	}

	return data;
}
