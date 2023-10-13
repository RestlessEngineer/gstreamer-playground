/*
 * GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 * Copyright (C) 2023 vladislav <<user@hostname.org>>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-locfilter
 *
 * FIXME:Describe locfilter here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! locfilter ! fakesink show_box=TRUE
 * ]|
 * </refsect2>
 */


#include "config.h"
#include <gst/gst.h>
#include <gst/base/base.h>
#include <gst/controller/controller.h>
#include <gst/video/video.h>
#include <opencv2/opencv.hpp>

#include "gstlocfilter.h"

GST_DEBUG_CATEGORY_STATIC (gst_locfilter_debug);
#define GST_CAT_DEFAULT gst_locfilter_debug

enum
{
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SHOW_BOXES,
};

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("video/x-raw,"
      " format = (string)RGB," 
      " width = (int) [ 16, 4096 ],"
      " height = (int) [ 16, 4096 ],"
      " framerate = (fraction) [ 0/1, 2147483647/1 ]")
    );

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("video/x-raw,"
      " format = (string)RGB," 
      " width = (int) [ 16, 4096 ],"
      " height = (int) [ 16, 4096 ],"
      " framerate = (fraction) [ 0/1, 2147483647/1 ]")
    );

#define gst_locfilter_parent_class parent_class
G_DEFINE_TYPE (Gstlocfilter, gst_locfilter, GST_TYPE_BASE_TRANSFORM);
GST_ELEMENT_REGISTER_DEFINE (locfilter, "locfilter", GST_RANK_NONE,
    GST_TYPE_LOCFILTER);

static void gst_locfilter_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_locfilter_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_locfilter_transform_ip (GstBaseTransform *
    base, GstBuffer * outbuf);


/* initialize the locfilter's class */
static void
gst_locfilter_class_init (GstlocfilterClass * klass)
{

  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_locfilter_set_property;
  gobject_class->get_property = gst_locfilter_get_property;

  g_object_class_install_property (gobject_class, PROP_SHOW_BOXES,
      g_param_spec_boolean ("show-boxes", "Show", "Show boxes on the screen?",
          FALSE, (GParamFlags)(G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE)));

  gst_element_class_set_details_simple (gstelement_class,
      "locfilter",
      "Generic/Filter",
      "FIXME:Generic Template Filter", "vladislav <<user@hostname.org>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_template));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_template));

  GST_BASE_TRANSFORM_CLASS (klass)->transform_ip =
      GST_DEBUG_FUNCPTR (gst_locfilter_transform_ip);


  GST_DEBUG_CATEGORY_INIT (gst_locfilter_debug, "locfilter", 0,
      "Template locfilter");
}

static void
gst_locfilter_init (Gstlocfilter * filter)
{
  filter->show_box = TRUE;
}

static void
gst_locfilter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{

  Gstlocfilter *filter = GST_LOCFILTER (object);

  switch (prop_id) {
    case PROP_SHOW_BOXES:
      filter->show_box = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_locfilter_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  Gstlocfilter *filter = GST_LOCFILTER (object);

  switch (prop_id) {
    case PROP_SHOW_BOXES:
      g_value_set_boolean (value, filter->show_box);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static std::vector<cv::Rect> 
make_localisation_boxes(guint8 *data, int width, int height){
    
  cv::Mat image(height, width, CV_8UC3, (void*)data);
    
  cv::cvtColor(image, image, cv::COLOR_BGR2GRAY);
  cv::threshold(image, image, 190, 255, cv::THRESH_BINARY);

  std::vector<std::vector<cv::Point>> contours;
  std::vector<cv::Vec4i> hierarchy;
  cv::findContours(image, contours, hierarchy, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

  auto frame_box = std::max_element(contours.begin(), contours.end(), [](const std::vector<cv::Point>& a, const std::vector<cv::Point>& b) {
      return cv::contourArea(a) < cv::contourArea(b);
  });

  // Remove the largest contour (the outermost one)
  contours.erase(frame_box);

  std::vector<cv::Rect> bboxes;
  for (const auto& cnt : contours) {
      cv::Rect bbox = cv::boundingRect(cnt);
      bboxes.push_back(bbox);
  }
  return bboxes;
}

static void
add_boxes_into_buf(GstBuffer * outbuf, std::vector<cv::Rect>& bboxes){
    
    static const gchar* tags[] = { NULL };
    if (g_type_from_name("metaboxes") == 0) {
      auto meta_info = gst_meta_register_custom("metaboxes", tags, NULL, NULL, NULL);
    }
    auto meta = gst_buffer_add_custom_meta (outbuf, "metaboxes");
    auto metadata = gst_custom_meta_get_structure (meta);
    
    GValueArray *value_array = g_value_array_new(-1);

    auto gvalue_set = [&value_array](int in_val){
        GValue value = G_VALUE_INIT;
        g_value_init(&value, G_TYPE_INT); 
        g_value_set_int(&value, in_val);
        g_value_array_append(value_array, &value);
    };

    for(auto& box: bboxes){
        gvalue_set(box.x);
        gvalue_set(box.y);
        gvalue_set(box.width);
        gvalue_set(box.height);
    }

    gst_structure_set_array( metadata, "boxes", value_array);
      
}

 /// for checking
static std::vector<cv::Rect>
get_boxes_from_buf(GstBuffer * outbuf){
     
      GValueArray *value_array_get = g_value_array_new(-1);
      auto meta_get = gst_buffer_get_custom_meta (outbuf, "metaboxes");
      auto metadata_get = gst_custom_meta_get_structure (meta_get);
      
      gst_structure_get_array (metadata_get, "boxes", &value_array_get);
      guint num_values = value_array_get->n_values;

      auto gvalue_get = [&value_array_get](uint i) -> int {
          GValue *value = g_value_array_get_nth(value_array_get, i); 
          return g_value_get_int(value);
      };

      std::vector<cv::Rect> boxes;
      for (guint i = 0; i < num_values; i+=4) {
          cv::Rect rect;
          rect.x = gvalue_get(i);
          rect.y = gvalue_get(i+1);
          rect.width = gvalue_get(i+2);
          rect.height = gvalue_get(i+3);

          boxes.emplace_back(std::move(rect));
      }
  return boxes;
}


static GstFlowReturn
gst_locfilter_transform_ip (GstBaseTransform * base, GstBuffer * outbuf)
{
  Gstlocfilter *filter = GST_LOCFILTER (base);
  GstVideoInfo video_info;

  GstCaps *sink_caps = gst_pad_get_current_caps(GST_BASE_TRANSFORM_SINK_PAD(base));

  if (!sink_caps || !gst_video_info_from_caps(&video_info, sink_caps)) {
      g_warning("Failed to get video info from caps");
      return GST_FLOW_ERROR;
  }

  if (GST_CLOCK_TIME_IS_VALID (GST_BUFFER_TIMESTAMP (outbuf)))
    gst_object_sync_values (GST_OBJECT (filter), GST_BUFFER_TIMESTAMP (outbuf));


  GstMapInfo map;
  if ( gst_buffer_map(outbuf, &map, GST_MAP_READWRITE)) {
      guint8 *data = map.data;
      
      // Extract video frame information
      int width = GST_VIDEO_INFO_WIDTH(&video_info);
      int height = GST_VIDEO_INFO_HEIGHT(&video_info);
      int stride = GST_VIDEO_INFO_PLANE_STRIDE(&video_info, 0); 

      cv::Mat original_image(height, width, CV_8UC3, (void*)data);

      auto bboxes = make_localisation_boxes(data, width, height);
      
      // for attaching information about boxes into outbuf
      add_boxes_into_buf(outbuf, bboxes);

      // TODO: only for cheking buffer information
      //! bboxes = get_boxes_from_buf(outbuf);

      if(filter->show_box == TRUE){

          for (const auto& box : bboxes) {
              cv::Point tlc(box.x, box.y);
              cv::Point brc(box.x + box.width, box.y + box.height);
              cv::rectangle(original_image, tlc, brc, cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
          }

      }

      memcpy(data, original_image.data,  height * stride);

      gst_buffer_unmap(outbuf, &map);
  }

  return GST_FLOW_OK;
}


static gboolean
locfilter_init (GstPlugin * locfilter)
{
  return GST_ELEMENT_REGISTER (locfilter, locfilter);
}


GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    locfilter,
    "locfilter",
    locfilter_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
