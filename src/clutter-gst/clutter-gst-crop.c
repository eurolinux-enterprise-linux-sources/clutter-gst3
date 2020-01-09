/*
 * Clutter-GStreamer.
 *
 * GStreamer integration library for Clutter.
 *
 * clutter-gst-aspectratio.c - An actor rendering a video with respect
 * to its aspect ratio.
 *
 * Authored by Lionel Landwerlin <lionel.g.landwerlin@linux.intel.com>
 *
 * Copyright (C) 2013 Intel Corporation
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

/**
 * SECTION:clutter-gst-crop
 * @short_description: A #ClutterContent for displaying part of video frames
 *
 * #ClutterGstCrop sub-classes #ClutterGstContent.
 */

#include "clutter-gst-crop.h"
#include "clutter-gst-private.h"

static void content_iface_init (ClutterContentIface *iface);

G_DEFINE_TYPE_WITH_CODE (ClutterGstCrop,
                         clutter_gst_crop,
                         CLUTTER_GST_TYPE_CONTENT,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTENT,
                                                content_iface_init))

#define CROP_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CLUTTER_GST_TYPE_CROP, ClutterGstCropPrivate))

struct _ClutterGstCropPrivate
{
  ClutterGstBox input_region;
  ClutterGstBox output_region;

  gboolean paint_borders;
  gboolean cull_backface;
};

enum
{
  PROP_0,

  PROP_PAINT_BORDERS,
  PROP_CULL_BACKFACE,
  PROP_INPUT_REGION,
  PROP_OUTPUT_REGION
};

/**/

static gboolean
clutter_gst_crop_get_preferred_size (ClutterContent *content,
                                     gfloat         *width,
                                     gfloat         *height)
{
  ClutterGstFrame *frame =
    clutter_gst_content_get_frame (CLUTTER_GST_CONTENT (content));

  if (!frame)
    return FALSE;

  if (width)
    *width = frame->resolution.width;
  if (height)
    *height = frame->resolution.height;

  return TRUE;
}

static gboolean
clutter_gst_crop_get_overlay_box (ClutterGstCrop      *self,
                                  ClutterGstBox       *input_box,
                                  ClutterGstBox       *paint_box,
                                  const ClutterGstBox *frame_box,
                                  ClutterGstFrame     *frame,
                                  ClutterGstOverlay   *overlay)
{
  ClutterGstCropPrivate *priv = self->priv;
  ClutterGstBox overlay_input_box;
  ClutterGstBox frame_input_box;

  /* Clamped frame input */
  frame_input_box.x1 = priv->input_region.x1 * frame->resolution.width;
  frame_input_box.y1 = priv->input_region.y1 * frame->resolution.height;
  frame_input_box.x2 = priv->input_region.x2 * frame->resolution.width;
  frame_input_box.y2 = priv->input_region.y2 * frame->resolution.height;

  /* Clamp overlay box to frame's clamping */
  overlay_input_box.x1 = MAX (priv->input_region.x1 * frame->resolution.width, overlay->position.x1);
  overlay_input_box.y1 = MAX (priv->input_region.y1 * frame->resolution.height, overlay->position.y1);
  overlay_input_box.x2 = MIN (priv->input_region.x2 * frame->resolution.width, overlay->position.x2);
  overlay_input_box.y2 = MIN (priv->input_region.y2 * frame->resolution.height, overlay->position.y2);

  /* normalize overlay input */
  input_box->x1 = (overlay_input_box.x1 - overlay->position.x1) / (overlay->position.x2 - overlay->position.x1);
  input_box->y1 = (overlay_input_box.y1 - overlay->position.y1) / (overlay->position.y2 - overlay->position.y1);
  input_box->x2 = (overlay_input_box.x2 - overlay->position.x1) / (overlay->position.x2 - overlay->position.x1);
  input_box->y2 = (overlay_input_box.y2 - overlay->position.y1) / (overlay->position.y2 - overlay->position.y1);

  /* bail if not in the visible scope */
  if (input_box->x1 >= input_box->x2 ||
      input_box->y1 >= input_box->y2)
    return FALSE;

  /* Clamp overlay output */
  paint_box->x1 = frame_box->x1 + (frame_box->x2 - frame_box->x1) * ((overlay_input_box.x1 - frame_input_box.x1) / (frame_input_box.x2 - frame_input_box.x1));
  paint_box->y1 = frame_box->y1 + (frame_box->y2 - frame_box->y1) * ((overlay_input_box.y1 - frame_input_box.y1) / (frame_input_box.y2 - frame_input_box.y1));
  paint_box->x2 = frame_box->x1 + (frame_box->x2 - frame_box->x1) * ((overlay_input_box.x2 - frame_input_box.x1) / (frame_input_box.x2 - frame_input_box.x1));
  paint_box->y2 = frame_box->y1 + (frame_box->y2 - frame_box->y1) * ((overlay_input_box.y2 - frame_input_box.y1) / (frame_input_box.y2 - frame_input_box.y1));

  return TRUE;
}

static void
clutter_gst_crop_paint_content (ClutterContent   *content,
                                ClutterActor     *actor,
                                ClutterPaintNode *root)
{
  ClutterGstCrop *self = CLUTTER_GST_CROP (content);
  ClutterGstCropPrivate *priv = self->priv;
  ClutterGstContent *gst_content = CLUTTER_GST_CONTENT (content);
  ClutterGstFrame *frame = clutter_gst_content_get_frame (gst_content);
  guint8 paint_opacity = clutter_actor_get_paint_opacity (actor);
  ClutterActorBox content_box;
  ClutterGstBox frame_box;
  gfloat box_width, box_height;
  ClutterColor color;
  ClutterPaintNode *node;

  clutter_actor_get_content_box (actor, &content_box);

  if (!frame)
    {
      /* No frame to paint, just paint the background color of the
         actor. */
      if (priv->paint_borders)
        {
          clutter_actor_get_background_color (actor, &color);
          color.alpha = paint_opacity;

          node = clutter_color_node_new (&color);
          clutter_paint_node_set_name (node, "CropIdleVideo");

          clutter_paint_node_add_rectangle_custom (node,
                                                   content_box.x1, content_box.y1,
                                                   content_box.x2, content_box.y2);
          clutter_paint_node_add_child (root, node);
          clutter_paint_node_unref (node);
        }

      return;
    }

  box_width = clutter_actor_box_get_width (&content_box);
  box_height = clutter_actor_box_get_height (&content_box);

  if (priv->paint_borders &&
      (priv->output_region.x1 > 0 ||
       priv->output_region.x2 < 1 ||
       priv->output_region.y1 > 0 ||
       priv->output_region.y2 < 1))
    {
      clutter_actor_get_background_color (actor, &color);
      color.alpha = paint_opacity;

      node = clutter_color_node_new (&color);
      clutter_paint_node_set_name (node, "CropVideoBorders");

      if (priv->output_region.x1 > 0)
        clutter_paint_node_add_rectangle_custom (node,
                                                 content_box.x1,
                                                 content_box.y1,
                                                 content_box.x1 + box_width * priv->output_region.x1,
                                                 content_box.y2);
      if (priv->output_region.x2 < 1)
        clutter_paint_node_add_rectangle_custom (node,
                                                 content_box.x1 + box_width * priv->output_region.x2,
                                                 content_box.y1,
                                                 content_box.x2,
                                                 content_box.y2);
      if (priv->output_region.y1 > 0)
        clutter_paint_node_add_rectangle_custom (node,
                                                 content_box.x1 + box_width * priv->output_region.x1,
                                                 content_box.y1,
                                                 content_box.x1 + box_width * priv->output_region.x2,
                                                 content_box.y1 + box_height * priv->output_region.y1);
      if (priv->output_region.y2 < 1)
        clutter_paint_node_add_rectangle_custom (node,
                                                 content_box.x1 + box_width * priv->output_region.x1,
                                                 content_box.y1 + box_height * priv->output_region.y2,
                                                 content_box.x1 + box_width * priv->output_region.x2,
                                                 content_box.y2);

      clutter_paint_node_add_child (root, node);
      clutter_paint_node_unref (node);
    }


  frame_box.x1 = content_box.x1 + box_width * priv->output_region.x1;
  frame_box.y1 = content_box.y1 + box_height * priv->output_region.y1;
  frame_box.x2 = content_box.x1 + box_width * priv->output_region.x2;
  frame_box.y2 = content_box.y1 + box_height * priv->output_region.y2;

  if (clutter_gst_content_get_paint_frame (gst_content))
    {
      cogl_pipeline_set_color4ub (frame->pipeline,
                                  paint_opacity,
                                  paint_opacity,
                                  paint_opacity,
                                  paint_opacity);
      if (priv->cull_backface)
        cogl_pipeline_set_cull_face_mode (frame->pipeline,
                                          COGL_PIPELINE_CULL_FACE_MODE_BACK);

      node = clutter_pipeline_node_new (frame->pipeline);
      clutter_paint_node_set_name (node, "CropVideoFrame");

      clutter_paint_node_add_texture_rectangle_custom (node,
                                                       frame_box.x1,
                                                       frame_box.y1,
                                                       frame_box.x2,
                                                       frame_box.y2,
                                                       priv->input_region.x1,
                                                       priv->input_region.y1,
                                                       priv->input_region.x2,
                                                       priv->input_region.y2);
      clutter_paint_node_add_child (root, node);
      clutter_paint_node_unref (node);
    }


  if (clutter_gst_content_get_paint_overlays (gst_content))
    {
      ClutterGstOverlays *overlays = clutter_gst_content_get_overlays (gst_content);

      if (overlays)
        {
          guint i;

          for (i = 0; i < overlays->overlays->len; i++)
            {
              ClutterGstOverlay *overlay =
                g_ptr_array_index (overlays->overlays, i);
              ClutterGstBox overlay_box;
              ClutterGstBox overlay_input_box;

              /* overlay outside the visible scope? -> next */
              if (!clutter_gst_crop_get_overlay_box (self,
                                                     &overlay_input_box,
                                                     &overlay_box,
                                                     /* &content_box, */
                                                     &frame_box,
                                                     frame,
                                                     overlay))
                continue;

              cogl_pipeline_set_color4ub (overlay->pipeline,
                                          paint_opacity, paint_opacity,
                                          paint_opacity, paint_opacity);

              node = clutter_pipeline_node_new (overlay->pipeline);
              clutter_paint_node_set_name (node, "CropVideoOverlay");

              clutter_paint_node_add_texture_rectangle_custom (node,
                                                               overlay_box.x1, overlay_box.y1,
                                                               overlay_box.x2, overlay_box.y2,
                                                               overlay_input_box.x1, overlay_input_box.y1,
                                                               overlay_input_box.x2, overlay_input_box.y2);

              clutter_paint_node_add_child (root, node);
              clutter_paint_node_unref (node);
            }
        }
    }
}

static void
content_iface_init (ClutterContentIface *iface)
{
  iface->get_preferred_size = clutter_gst_crop_get_preferred_size;
  iface->paint_content = clutter_gst_crop_paint_content;
}

/**/

static gboolean
_validate_box (ClutterGstBox *box)
{
  if (box->x1 >= 0 &&
      box->x1 <= 1 &&
      box->y1 >= 0 &&
      box->y1 <= 1 &&
      box->x2 >= 0 &&
      box->x2 <= 1 &&
      box->y2 >= 0 &&
      box->y2 <= 1)
    return TRUE;

  return FALSE;
}

static void
clutter_gst_crop_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  ClutterGstCropPrivate *priv = CLUTTER_GST_CROP (object)->priv;
  ClutterGstBox *box;

  switch (property_id)
    {
    case PROP_PAINT_BORDERS:
      g_value_set_boolean (value, priv->paint_borders);
      break;
    case PROP_CULL_BACKFACE:
      g_value_set_boolean (value, priv->cull_backface);
      break;
    case PROP_INPUT_REGION:
      box = (ClutterGstBox *) g_value_get_boxed (value);
      *box = priv->input_region;
      break;
    case PROP_OUTPUT_REGION:
      box = (ClutterGstBox *) g_value_get_boxed (value);
      *box = priv->output_region;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
clutter_gst_crop_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  ClutterGstCropPrivate *priv = CLUTTER_GST_CROP (object)->priv;
  ClutterGstBox *box;

  switch (property_id)
    {
    case PROP_PAINT_BORDERS:
      if (priv->paint_borders != g_value_get_boolean (value))
        {
          priv->paint_borders = g_value_get_boolean (value);
          clutter_content_invalidate (CLUTTER_CONTENT (object));
        }
      break;
    case PROP_CULL_BACKFACE:
      priv->cull_backface = g_value_get_boolean (value);
      break;
    case PROP_INPUT_REGION:
      box = (ClutterGstBox *) g_value_get_boxed (value);
      if (_validate_box (box)) {
        priv->input_region = *box;
        clutter_content_invalidate (CLUTTER_CONTENT (object));
      } else
        g_warning ("Input region must be given in [0, 1] values.");
      break;
    case PROP_OUTPUT_REGION:
      box = (ClutterGstBox *) g_value_get_boxed (value);
      if (_validate_box (box)) {
        priv->output_region = *box;
        clutter_content_invalidate (CLUTTER_CONTENT (object));
      } else
        g_warning ("Output region must be given in [0, 1] values.");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
clutter_gst_crop_dispose (GObject *object)
{
  G_OBJECT_CLASS (clutter_gst_crop_parent_class)->dispose (object);
}

static void
clutter_gst_crop_finalize (GObject *object)
{
  G_OBJECT_CLASS (clutter_gst_crop_parent_class)->finalize (object);
}

static void
clutter_gst_crop_class_init (ClutterGstCropClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (ClutterGstCropPrivate));

  object_class->get_property = clutter_gst_crop_get_property;
  object_class->set_property = clutter_gst_crop_set_property;
  object_class->dispose = clutter_gst_crop_dispose;
  object_class->finalize = clutter_gst_crop_finalize;

  /**
   * ClutterGstCrop:paint-borders:
   *
   * Whether or not paint borders on the sides of the video
   *
   * Since: 3.0
   */
  pspec = g_param_spec_boolean ("paint-borders",
                                "Paint borders",
                                "Paint borders on side of video",
                                FALSE,
                                CLUTTER_GST_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_PAINT_BORDERS, pspec);

  /**
   * ClutterGstCrop:cull-backface:
   *
   * Whether to cull the backface of the actor
   *
   * Since: 3.0
   */
  pspec = g_param_spec_boolean ("cull-backface",
                                "Cull Backface",
                                "Cull the backface of the actor",
                                FALSE,
                                CLUTTER_GST_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_CULL_BACKFACE, pspec);

  /**
   * ClutterGstCrop:input-region:
   *
   * Input region in the video frame (all values between 0 and 1).
   *
   * Since: 3.0
   */
  pspec = g_param_spec_boxed ("input-region",
                              "Input Region",
                              "Input Region",
                              CLUTTER_GST_TYPE_BOX,
                              CLUTTER_GST_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_INPUT_REGION, pspec);

  /**
   * ClutterGstCrop:output-region:
   *
   * Output region in the actor's allocation (all values between 0 and 1).
   *
   * Since: 3.0
   */
  pspec = g_param_spec_boxed ("output-region",
                              "Output Region",
                              "Output Region",
                              CLUTTER_GST_TYPE_BOX,
                              CLUTTER_GST_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_OUTPUT_REGION, pspec);
}

static void
clutter_gst_crop_init (ClutterGstCrop *self)
{
  ClutterGstCropPrivate *priv;

  priv = self->priv = CROP_PRIVATE (self);

  priv->input_region.x1 = 0;
  priv->input_region.y1 = 0;
  priv->input_region.x2 = 1;
  priv->input_region.y2 = 1;

  priv->output_region = priv->input_region;
}

/**
 * clutter_gst_crop_new:
 *
 * Returns: (transfer full): a new #ClutterGstCrop instance
 */
ClutterActor *
clutter_gst_crop_new (void)
{
  return g_object_new (CLUTTER_GST_TYPE_CROP, NULL);
}
