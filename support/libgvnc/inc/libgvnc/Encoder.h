/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Encoder.h
 * Author: harry
 *
 * Created on 08 May 2018, 16:29
 */

#ifndef LIBGVNC_ENCODER_H
#define LIBGVNC_ENCODER_H

#include "libgvnc/Framebuffer.h"

namespace libgvnc {
	class Encoder {
	public:
		Encoder(const struct FB_PixelFormat &format, const RectangleShape &shape);
		virtual ~Encoder() {}
		
		virtual void AddPixel(uint32_t pixel_data) = 0;
		virtual std::vector<char> GetData() = 0;
		
		const RectangleShape &GetShape() const { return shape_; }
		const struct FB_PixelFormat &GetFormat() const { return encoded_format_; }
		
	private:
		struct FB_PixelFormat encoded_format_;
		RectangleShape shape_;
		
	};
	
	class RawEncoder : public Encoder {
	public:
		RawEncoder(const struct FB_PixelFormat &format, const RectangleShape &shape);
		virtual ~RawEncoder()
		{

		}

		virtual void AddPixel(uint32_t pixel_data);

		virtual std::vector<char> GetData();
	private:
		std::vector<char> data_;

	};
}

#endif /* ENCODER_H */

