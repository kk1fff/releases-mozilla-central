/* ***** BEGIN LICENSE BLOCK *****
 * 
 * Copyright (c) 2008, Mozilla Corporation
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of the Mozilla Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * Contributor(s):
 *   Josh Aas <josh@mozilla.com>
 * 
 * ***** END LICENSE BLOCK ***** */

#ifndef nptest_platform_h_
#define nptest_platform_h_

#include "npipctest.h"

/**
 * Returns true if the plugin supports windowed mode
 */
bool    pluginSupportsWindowMode();

/**
 * Returns true if the plugin supports windowless mode. At least one of
 * "pluginSupportsWindowMode" and "pluginSupportsWindowlessMode" must
 * return true.
 */
bool    pluginSupportsWindowlessMode();

/**
 * Initialize the plugin instance. Returning an error here will cause the
 * plugin instantiation to fail.
 */
NPError pluginInstanceInit(InstanceData* instanceData);

/**
 * Shutdown the plugin instance.
 */
void    pluginInstanceShutdown(InstanceData* instanceData);

/**
 * Set the instanceData's window to newWindow.
 */
void    pluginDoSetWindow(InstanceData* instanceData, NPWindow* newWindow);

/**
 * Initialize the window for a windowed plugin. oldWindow is the old
 * native window value. This will never be called for windowless plugins.
 */
void    pluginWidgetInit(InstanceData* instanceData, void* oldWindow);

/**
 * Handle an event for a windowless plugin. (Windowed plugins are
 * responsible for listening for their own events.)
 */
int16_t pluginHandleEvent(InstanceData* instanceData, void* event);

enum RectEdge {
  EDGE_LEFT = 0,
  EDGE_TOP = 1,
  EDGE_RIGHT = 2,
  EDGE_BOTTOM = 3
};

enum {
  NPTEST_INT32_ERROR = 0x7FFFFFFF
};

/**
 * Return the coordinate of the given edge of the plugin's area, relative
 * to the top-left corner of the toplevel window containing the plugin,
 * including window decorations. Only works for window-mode plugins
 * and Mac plugins.
 * Returns NPTEST_ERROR on error.
 */
int32_t pluginGetEdge(InstanceData* instanceData, RectEdge edge);

/**
 * Return the number of rectangles in the plugin's clip region. Only
 * works for window-mode plugins and Mac plugins.
 * Returns NPTEST_ERROR on error.
 */
int32_t pluginGetClipRegionRectCount(InstanceData* instanceData);

/**
 * Return the coordinate of the given edge of a rectangle in the plugin's
 * clip region, relative to the top-left corner of the toplevel window
 * containing the plugin, including window decorations. Only works for
 * window-mode plugins and Mac plugins.
 * Returns NPTEST_ERROR on error.
 */
int32_t pluginGetClipRegionRectEdge(InstanceData* instanceData, 
    int32_t rectIndex, RectEdge edge);

#endif // nptest_platform_h_
