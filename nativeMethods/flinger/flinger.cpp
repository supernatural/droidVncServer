/*
	 droid vnc server - Android VNC server
	 Copyright (C) 2009 Jose Pereira <onaips@gmail.com>

	 This library is free software; you can redistribute it and/or
	 modify it under the terms of the GNU Lesser General Public
	 License as published by the Free Software Foundation; either
	 version 3 of the License, or (at your option) any later version.

	 This library is distributed in the hope that it will be useful,
	 but WITHOUT ANY WARRANTY; without even the implied warranty of
	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	 Lesser General Public License for more details.

	 You should have received a copy of the GNU Lesser General Public
	 License along with this library; if not, write to the Free Software
	 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
	 */

#include "flinger.h"
#include "screenFormat.h"

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

#include <binder/IMemory.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>

#include <ui/DisplayInfo.h>
#include <ui/PixelFormat.h>

using namespace android;

static uint32_t DEFAULT_DISPLAY_ID = ISurfaceComposer::eDisplayIdMain;

ScreenshotClient *screenshotClient=NULL;
sp<IBinder> display;
uint32_t captureOrientation;

extern "C" screenFormat getscreenformat_flinger()
{
  //get format on PixelFormat struct
	PixelFormat f=screenshotClient->getFormat();

	screenFormat format;
        bzero(&format, sizeof(screenFormat));

	format.bitsPerPixel = bitsPerPixel(f);
	format.width = screenshotClient->getWidth();
	format.height =     screenshotClient->getHeight();
	format.size = screenshotClient->getSize();
        format.line_length = screenshotClient->getStride();

        switch (f) {
            case PIXEL_FORMAT_RGBA_8888:
                format.alphaShift = 24;
                format.alphaMax = 8;
            case PIXEL_FORMAT_RGBX_8888:
            case PIXEL_FORMAT_RGB_888:
                format.redShift = 0;
                format.redMax = 8;
                format.greenShift = 8;
                format.greenMax= 8;
                format.blueShift = 16;
                format.blueMax = 8;
                break;
            case HAL_PIXEL_FORMAT_RGB_565:
                format.redShift = 11;
                format.redMax = 5;
                format.greenShift = 5;
                format.greenMax= 6;
                format.blueShift = 0;
                format.blueMax = 5;
                break;
            case HAL_PIXEL_FORMAT_BGRA_8888:
                format.alphaShift = 24;
                format.alphaMax = 8;
                format.redShift = 16;
                format.redMax = 8;
                format.greenShift = 8;
                format.greenMax= 8;
                format.blueShift = 0;
                format.blueMax = 8;
                break;
            case PIXEL_FORMAT_RGBA_5551:
            case PIXEL_FORMAT_RGBA_4444:
            default:
                break;
        }

	return format;
}

extern "C" int init_flinger()
{
    int32_t displayId = DEFAULT_DISPLAY_ID;
    int errno;

    // Maps orientations from DisplayInfo to ISurfaceComposer
    static const uint32_t ORIENTATION_MAP[] = {
        ISurfaceComposer::eRotateNone, // 0 == DISPLAY_ORIENTATION_0
        ISurfaceComposer::eRotate270, // 1 == DISPLAY_ORIENTATION_90
        ISurfaceComposer::eRotate180, // 2 == DISPLAY_ORIENTATION_180
        ISurfaceComposer::eRotate90, // 3 == DISPLAY_ORIENTATION_270
    };
    L("--Initializing gingerbread access method--\n");

    display = SurfaceComposerClient::getBuiltInDisplay(displayId);
    if (display == NULL) {
        L("Unable to get handle for display %d\n", displayId);
        return -1;
    }
    Vector<DisplayInfo> configs;
    SurfaceComposerClient::getDisplayConfigs(display, &configs);
    int activeConfig = SurfaceComposerClient::getActiveConfig(display);
    if (static_cast<size_t>(activeConfig) >= configs.size()) {
        L("Active config %d not inside configs (size %zu)\n",
                activeConfig, configs.size());
        return -1;
    }
    uint8_t displayOrientation = configs[activeConfig].orientation;
    captureOrientation = ORIENTATION_MAP[displayOrientation];

    screenshotClient = new ScreenshotClient();
    status_t result = screenshotClient->update(display, Rect(), 384, 640, 0, -1U,
            false, captureOrientation);
    if (result != NO_ERROR) {
        free(screenshotClient);
        screenshotClient = NULL;
        return -1;
    }

    if (!screenshotClient->getPixels()) {
        free(screenshotClient);
        screenshotClient = NULL;
        return -1;
    }

    return 0;
}

extern "C" unsigned int *readfb_flinger()
{
	screenshotClient->update(display, Rect(), 384, 640, 0, -1U,
                false, captureOrientation);
	return (unsigned int*)screenshotClient->getPixels();
}

extern "C" void close_flinger()
{
  free(screenshotClient);
}
