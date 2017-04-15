/*
  Image Loader
  Copyright (C) 2017 James Heggie

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#pragma once

namespace james {

  struct Image {
    Image();
    Image(const Image&);
    Image(Image&&) noexcept;

    Image(unsigned int w, unsigned int h);

    ~Image() noexcept;

    Image& operator= (const Image&);
    Image& operator= (Image&&) noexcept;

    void Swap(Image&) noexcept;

    unsigned int Width() const noexcept { return w_; }
    unsigned int Height() const noexcept { return h_; }

    const unsigned char* Pixels() const noexcept { return pixels_; }
    unsigned char* Pixels() noexcept { return pixels_; }

  private:
    unsigned int w_, h_;
    unsigned char* pixels_;
  };

}