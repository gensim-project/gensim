/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
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
	std::vector<char> data (GetShape().Width * GetShape().Height * GetFormat().bits_per_pixel / 8);
	uint32_t data_ptr = 0;
	for(auto pixel_data : pixels) {
		switch(GetFormat().bits_per_pixel) {
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
				*(uint32_t*)(data.data() + data_ptr) = pixel_data;
				data_ptr += 4;
				break;

			default:
				throw std::logic_error("Unknown pixel depth");
		}
	}
	
	return data;
}
