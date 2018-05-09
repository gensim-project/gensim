/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "libgvnc/Client.h"
#include "libgvnc/Framebuffer.h"
#include "libgvnc/Encoder.h"

#include <functional>
#include <map>

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

static std::map<EncodingType, std::function<encoder_factory_t>> encoders {
	std::make_pair(EncodingType::Raw, [](const FB_PixelFormat& format, const RectangleShape& shape){return (Encoder*)new RawEncoder(format, shape);})
};

void Convert(uint32_t &data, const FB_PixelFormat &start_format, const FB_PixelFormat &target_format) 
{
	float true_red = ((data >> start_format.red_shift) & start_format.red_max) / (float)start_format.red_max;
	float true_green = ((data >> start_format.green_shift) & start_format.green_max) / (float)start_format.green_max;
	float true_blue = ((data >> start_format.blue_shift) & start_format.blue_max) / (float)start_format.blue_max;
	
	uint32_t output_red = true_red * target_format.red_max;
	uint32_t output_green = true_green * target_format.green_max;
	uint32_t output_blue = true_blue * target_format.blue_max;
	
	output_red &= target_format.red_max;
	output_green &= target_format.green_max;
	output_blue &= target_format.blue_max;
	
	data = (output_red << target_format.red_shift) | (output_green << target_format.green_shift) | (output_blue << target_format.blue_shift);
}

uint32_t Framebuffer::GetPixel(uint32_t x, uint32_t y) const
{
	uint32_t data = *(uint32_t*)((char*)data_ + (y * GetWidth() * GetPixelFormat().bits_per_pixel/8) + (x * GetPixelFormat().bits_per_pixel/8));
	switch(GetPixelFormat().bits_per_pixel) {
		case 8:  data &= 0xff; break;
		case 16: data &= 0xffff; break;
		case 24: data &= 0xffffff; break;
		case 32: data &= 0xffffffff; break;
		default:
			throw std::logic_error("Unsupported pixel format");
	}
	return data;
}

void Framebuffer::FillRectangle(Rectangle& rect, const FB_PixelFormat& target_format, EncodingType encoding)
{
	Encoder *encoder = encoders[encoding](target_format, rect.Shape);
	
	for(int y = 0; y < rect.Shape.Height; ++y) {
		for(int x = 0; x < rect.Shape.Width; ++x) {
			uint32_t pixeldata = GetPixel(x, y);
			Convert(pixeldata, GetPixelFormat(), target_format);
			encoder->AddPixel(pixeldata);
		}
	}
	
	rect.Data = encoder->GetData();
	
	delete encoder;
}
