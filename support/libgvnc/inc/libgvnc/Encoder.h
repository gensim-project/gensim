/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#pragma once

#include "Framebuffer.h"

namespace libgvnc
{

	class Encoder
	{
	public:
		Encoder(const struct FB_PixelFormat &format, const RectangleShape &shape);

		virtual ~Encoder()
		{
		}

		virtual std::vector<char> Encode(const std::vector<uint32_t> &data) = 0;

		const RectangleShape &GetShape() const
		{
			return shape_;
		}

		const struct FB_PixelFormat &GetFormat() const {
			return encoded_format_;
		}

	private:
		struct FB_PixelFormat encoded_format_;
		RectangleShape shape_;
	};

	class RawEncoder : public Encoder
	{
	public:
		RawEncoder(const struct FB_PixelFormat &format, const RectangleShape &shape);

		virtual ~RawEncoder()
		{

		}

		virtual std::vector<char> Encode(const std::vector<uint32_t> &data);
	};
}
