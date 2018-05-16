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

#include "Framebuffer.h"

namespace libgvnc {

    class Encoder {
    public:
        Encoder(const struct FB_PixelFormat &format, const RectangleShape &shape);

        virtual ~Encoder() {
        }

        virtual std::vector<char> Encode(const std::vector<uint32_t> &data) = 0;

        const RectangleShape &GetShape() const {
            return shape_;
        }

        const struct FB_PixelFormat &GetFormat() const {
            return encoded_format_;
        }

    private:
        struct FB_PixelFormat encoded_format_;
        RectangleShape shape_;
    };

    class RawEncoder : public Encoder {
    public:
        RawEncoder(const struct FB_PixelFormat &format, const RectangleShape &shape);

        virtual ~RawEncoder() {

        }

        virtual std::vector<char> Encode(const std::vector<uint32_t> &data);
    };
}
