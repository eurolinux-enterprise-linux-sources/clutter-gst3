/*
 * Clutter-GStreamer.
 *
 * GStreamer integration library for Clutter.
 *
 * clutter-gst-private.h - a private header, put whatever you want here.
 *
 * Copyright (C) 2010 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __CLUTTER_GST_PRIVATE_H__
#define __CLUTTER_GST_PRIVATE_H__

#include <glib.h>
#include <gst/video/video.h>

#include "clutter-gst.h"

G_BEGIN_DECLS

/* GLib has some define for that, but defining it ourselves allows to require a
 * lower version of GLib */
#define CLUTTER_GST_PARAM_STATIC        \
  (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)

#define CLUTTER_GST_PARAM_READABLE      \
  (G_PARAM_READABLE | CLUTTER_GST_PARAM_STATIC)

#define CLUTTER_GST_PARAM_WRITABLE      \
  (G_PARAM_READABLE | CLUTTER_GST_PARAM_STATIC)

#define CLUTTER_GST_PARAM_READWRITE     \
  (G_PARAM_READABLE | G_PARAM_WRITABLE | CLUTTER_GST_PARAM_STATIC)

#define clutter_paint_node_add_rectangle_custom(node,x1,y1,x2,y2) \
  do {                                                            \
    ClutterActorBox _box = { x1, y1, x2, y2 };                    \
    clutter_paint_node_add_rectangle (node, &_box);               \
  } while (0)

#define clutter_paint_node_add_texture_rectangle_custom(node,x1,y1,x2,y2,tx1,ty1,tx2,ty2) \
  do {                                                                  \
    ClutterActorBox _box = { x1, y1, x2, y2 };                          \
    clutter_paint_node_add_texture_rectangle (node, &_box,              \
                                              tx1, ty1, tx2, ty2);      \
  } while (0)

gboolean
_internal_plugin_init (GstPlugin *plugin);


CoglContext *clutter_gst_get_cogl_context (void);

ClutterGstFrame *clutter_gst_frame_new (void);

ClutterGstFrame *clutter_gst_create_blank_frame (const ClutterColor *color);

ClutterGstOverlay *clutter_gst_overlay_new (void);

ClutterGstOverlays *clutter_gst_overlays_new (void);

void clutter_gst_player_update_frame (ClutterGstPlayer *player,
                                      ClutterGstFrame **frame,
                                      ClutterGstFrame  *new_frame);

void clutter_gst_frame_update_pixel_aspect_ratio (ClutterGstFrame     *frame,
                                                  ClutterGstVideoSink *sink);

void clutter_gst_video_resolution_from_video_info (ClutterGstVideoResolution *resolution,
                                                   GstVideoInfo              *info);


gboolean clutter_gst_content_get_paint_frame (ClutterGstContent *content);
gboolean clutter_gst_content_get_paint_overlays (ClutterGstContent *content);

G_END_DECLS

#endif /* __CLUTTER_GST_PRIVATE_H__ */
