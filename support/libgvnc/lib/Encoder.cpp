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

void RawEncoder::AddPixel(uint32_t pixel_data)
{
	switch(GetFormat().bits_per_pixel) {
		case 8:
			data_.push_back(pixel_data);
			break;
		case 16:
			data_.push_back(pixel_data >> 8);
			data_.push_back(pixel_data);
			break;
		case 24:
			data_.push_back(pixel_data >> 16);
			data_.push_back(pixel_data >> 8);
			data_.push_back(pixel_data);
			break;
		case 32:
			data_.push_back(pixel_data);
			data_.push_back(pixel_data >> 8);
			data_.push_back(pixel_data >> 16);
			data_.push_back(pixel_data >> 24);
			
			
			
			break;
			
		default:
			throw std::logic_error("Unknown pixel depth");
	}
}

std::vector<char> RawEncoder::GetData()
{
	return data_;
}
