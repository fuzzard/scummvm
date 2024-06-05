/* Copyright (C) 2024 Giovanni Cascione <ing.cascione@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef BACKENDS_LIBRETRO_GRAPHICS_OPENGL_H
#define BACKENDS_LIBRETRO_GRAPHICS_OPENGL_H

#include "backends/graphics/opengl/opengl-graphics.h"
#include "backends/platform/libretro/include/libretro-graphics.h"

class LibretroOpenGLGraphics : public OpenGL::OpenGLGraphicsManager, public LibretroGraphics {
public:
	LibretroOpenGLGraphics(OpenGL::ContextType contextType);
	~LibretroOpenGLGraphics();
	bool loadVideoMode(uint requestedWidth, uint requestedHeight, const Graphics::PixelFormat &format) override { return true; };
	void refreshScreen() override;
};

class LibretroHWFramebuffer : public OpenGL::Backbuffer {

protected:
	void activateInternal() override;
};

#endif //BACKENDS_LIBRETRO_GRAPHICS_OPENGL_H
