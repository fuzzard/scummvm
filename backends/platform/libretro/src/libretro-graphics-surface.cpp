/* Copyright (C) 2023 Giovanni Cascione <ing.cascione@gmail.com>
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
#define FORBIDDEN_SYMBOL_ALLOW_ALL

#include "graphics/colormasks.h"
#include "graphics/palette.h"
#include "graphics/managed_surface.h"

#include "backends/platform/libretro/include/libretro-defs.h"
#include "backends/platform/libretro/include/libretro-core.h"
#include "backends/platform/libretro/include/libretro-os.h"
#include "backends/platform/libretro/include/libretro-timer.h"
#include "backends/platform/libretro/include/libretro-graphics-surface.h"

#include "gui/message.h"

LibretroSurfaceGraphics::LibretroSurfaceGraphics() : _mousePaletteEnabled(false),
	_mouseVisible(false),
	_mouseKeyColor(0),
	_mouseDontScale(false),
	_screenUpdatePending(false),
	_gamePalette(256),
	_mousePalette(256),
	_screen(RES_W_OVERLAY,RES_H_OVERLAY,Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0)){
		_overlay.create(RES_W_OVERLAY, RES_H_OVERLAY, Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0));
		_overlayDrawRect = Common::Rect(RES_W_OVERLAY, RES_H_OVERLAY);
		handleResize(RES_W_OVERLAY, RES_H_OVERLAY);
		}

LibretroSurfaceGraphics::~LibretroSurfaceGraphics() {
	_gameScreen.free();
	_overlay.free();
	_mouseImage.free();
	_screen.free();
}

Common::List<Graphics::PixelFormat> LibretroSurfaceGraphics::getSupportedFormats() const {
	Common::List<Graphics::PixelFormat> result;

#ifdef SCUMM_LITTLE_ENDIAN
	/* RGBA8888 */
	result.push_back(Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0));
#else
	/* ABGR8888 */
	result.push_back(Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));
#endif
	/* RGB565 - overlay */
	result.push_back(Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0));

	/* RGB555 - fmtowns */
	result.push_back(Graphics::PixelFormat(2, 5, 5, 5, 1, 10, 5, 0, 15));

	/* Palette - most games */
	result.push_back(Graphics::PixelFormat::createFormatCLUT8());

	return result;
}

const OSystem::GraphicsMode *LibretroSurfaceGraphics::getSupportedGraphicsModes() const {
	static const OSystem::GraphicsMode s_noGraphicsModes[] = {{0, 0, 0}};
	return s_noGraphicsModes;
}

void LibretroSurfaceGraphics::initSize(uint width, uint height, const Graphics::PixelFormat *format) {
	if (_overlayInGUI) {
		_overlay.create(width, height, format ? *format : Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0));
		_overlayDrawRect = Common::Rect(width, height);
	} else {
		_gameScreen.create(width, height, format ? *format : Graphics::PixelFormat::createFormatCLUT8());
		_gameDrawRect = Common::Rect(width, height);
	}
	handleResize(width, height);
	recalculateDisplayAreas();	
	LIBRETRO_G_SYSTEM->refreshRetroSettings();
}

int16 LibretroSurfaceGraphics::getHeight() const {
	return _gameScreen.h;
}

int16 LibretroSurfaceGraphics::getWidth() const {
	return _gameScreen.w;
}

Graphics::PixelFormat LibretroSurfaceGraphics::getScreenFormat() const {
	return _gameScreen.format;
}

void LibretroSurfaceGraphics::copyRectToScreen(const void *buf, int pitch, int x, int y, int w, int h) {
	_gameScreen.copyRectToSurface(buf, pitch, x, y, w, h);
}

void LibretroSurfaceGraphics::updateScreen() {
	_screenUpdatePending = true;
	if (! retro_setting_get_timing_inaccuracies_enabled() && !_overlayInGUI)
		dynamic_cast<LibretroTimerManager *>(LIBRETRO_G_SYSTEM->getTimerManager())->checkThread(THREAD_SWITCH_UPDATE);
}

void LibretroSurfaceGraphics::realUpdateScreen(void) {
	const Graphics::Surface &srcSurface = (_overlayInGUI) ? _overlay : _gameScreen;
	
	if (srcSurface.w && srcSurface.h)
		_screen.blitFrom(srcSurface, Common::Rect(srcSurface.w,srcSurface.h),Common::Rect(_screen.w,_screen.h),&_gamePalette);

	Common::Point mouse = getMouseVPosition();
	// Draw Mouse
	if (_mouseVisible && _mouseImage.w && _mouseImage.h)
		_screen.blitFrom(_mouseImage, Common::Point(mouse.x - _mouseHotspotX, mouse.y - _mouseHotspotY), _mousePaletteEnabled ? &_mousePalette : &_gamePalette);

	_screenUpdatePending = false;
}

void LibretroSurfaceGraphics::clearOverlay() {
	_overlay.fillRect(Common::Rect(_overlay.w, _overlay.h), 0);
}

void LibretroSurfaceGraphics::grabOverlay(Graphics::Surface &surface) const {
	surface.copyFrom(_overlay);
}

void LibretroSurfaceGraphics::copyRectToOverlay(const void *buf, int pitch, int x, int y, int w, int h) {
	_overlay.copyRectToSurface(buf, pitch, x, y, w, h);
}

int16 LibretroSurfaceGraphics::getOverlayHeight() const {
	return _overlay.h;
}

int16 LibretroSurfaceGraphics::getOverlayWidth() const {
	return _overlay.w;
}

Graphics::PixelFormat LibretroSurfaceGraphics::getOverlayFormat() const {
	return _overlay.format;
}

bool LibretroSurfaceGraphics::showMouse(bool visible) {
	const bool wasVisible = _mouseVisible;
	_mouseVisible = visible; 
	return wasVisible;
}

void LibretroSurfaceGraphics::setMouseCursor(const void *buf, uint w, uint h, int hotspotX, int hotspotY, uint32 keycolor, bool dontScale, const Graphics::PixelFormat *format, const byte *mask) {
	if (!buf || !w || !h)
		return;
	
	const Graphics::PixelFormat mformat = format ? *format : Graphics::PixelFormat::createFormatCLUT8();

	if (_mouseImage.w != w || _mouseImage.h != h || _mouseImage.format != mformat)
		_mouseImage.create(w, h, mformat);

	_mouseImage.copyRectToSurface(buf, _mouseImage.pitch, 0, 0, w, h);

	_mouseHotspotX = hotspotX;
	_mouseHotspotY = hotspotY;
	_mouseKeyColor = keycolor;
	_mouseDontScale = dontScale;
}

void LibretroSurfaceGraphics::setCursorPalette(const byte *colors, uint start, uint num) {
	_mousePalette.set(colors, start, num);
	_mousePaletteEnabled = true;
}

const Graphics::ManagedSurface *LibretroSurfaceGraphics::getScreen() {
	return &_screen;
}

void LibretroSurfaceGraphics::setPalette(const byte *colors, uint start, uint num) {
	_gamePalette.set(colors, start, num);
}

void LibretroSurfaceGraphics::grabPalette(byte *colors, uint start, uint num) const {
	_gamePalette.grab(colors, start, num);
}

bool LibretroSurfaceGraphics::hasFeature(OSystem::Feature f) const {
	return (f == OSystem::kFeatureCursorPalette) || (f == OSystem::kFeatureCursorAlpha);
}

void LibretroSurfaceGraphics::setFeatureState(OSystem::Feature f, bool enable) {
	if (f == OSystem::kFeatureCursorPalette)
		_mousePaletteEnabled = enable;
}

void LibretroSurfaceGraphics::displayMessageOnOSD(const Common::U32String &msg) {
	// Display the message for 3 seconds
	GUI::TimedMessageDialog dialog(msg, 3000);
	dialog.runModal();
}

bool LibretroSurfaceGraphics::getFeatureState(OSystem::Feature f) const {
	return (f == OSystem::kFeatureCursorPalette) ? _mousePaletteEnabled : false;
}

