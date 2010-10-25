/**
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA  02110-1301 USA
 *
 * \author Pablo Virolainen <pablo.virolainen@nomovok.com>
 * \author Johannes Lahti <johannes.lahti@nomovok.com>
 * \author Aki Honkasuo <aki.honkasuo@nomovok.com>
 *
 */
#ifndef GSTSHVIDEOBUFFER_H
#define GSTSHVIDEOBUFFER_H

#include <linux/videodev2.h>
#include <uiomux/uiomux.h>
#include <gst/gst.h>
#include <gst/video/video.h>

#include "gstshvideosink.h"

#define GST_TYPE_SH_VIDEO_BUFFER (gst_sh_video_buffer_get_type())
#define GST_IS_SH_VIDEO_BUFFER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_SH_VIDEO_BUFFER))
#define GST_SH_VIDEO_BUFFER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_SH_VIDEO_BUFFER, GstSHVideoBuffer))
#define GST_SH_VIDEO_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_SH_VIDEO_BUFFER, GstSHVideoBufferclass))
#define GST_SH_VIDEO_BUFFER_CAST(obj)  ((GstSHVideoBuffer *)(obj))


typedef struct _GstSHVideoBuffer GstSHVideoBuffer;
typedef struct _GstSHVideoBufferclass GstSHVideoBufferclass;

/**
 * \struct _GstSHVideoBuffer
 * \brief SuperH HW buffer for YUV-data
 * \var buffer Parent buffer
 * \var y_data Pointer to the Y-data
 * \var y_size Size of the Y-data
 * \var c_data Pointer to the C-data
 * \var c_size Size of the C-data
 */
struct _GstSHVideoBuffer 
{
	GstBuffer buffer;

	UIOMux *uiomux;
	int v4l2format;
	int allocated;
	gint allocated_size;
	guint8   *y_data;
	guint    y_size;
	guint8   *c_data;
	guint    c_size;
};

/**
 * \struct _GstSHVideoBufferclass
 * \var parent Parent
 */
struct _GstSHVideoBufferclass
{
	GstBufferClass parent;
};

// Some macros

#define GST_SH_VIDEO_BUFFER_Y_DATA(buf)  (GST_SH_VIDEO_BUFFER_CAST(buf)->y_data)
#define GST_SH_VIDEO_BUFFER_Y_SIZE(buf)  (GST_SH_VIDEO_BUFFER_CAST(buf)->y_size)
#define GST_SH_VIDEO_BUFFER_C_DATA(buf)  (GST_SH_VIDEO_BUFFER_CAST(buf)->c_data)
#define GST_SH_VIDEO_BUFFER_C_SIZE(buf)  (GST_SH_VIDEO_BUFFER_CAST(buf)->c_size)
#define GST_SH_VIDEO_BUFFER_V4l2_FMT(buf) (GST_SH_VIDEO_BUFFER_CAST(buf)->v4l2format)

/** 
 * Get Gstshbuffer object type
 * @return object type
 */
GType gst_sh_video_buffer_get_type (void);

/**
 * Allocate a buffer that can be directly accessed by the SH hardware
 */
GstBuffer *gst_sh_video_buffer_new(UIOMux *uiomux, gint width, gint height, int v4l2fmt);


/***************************** Helper functions *****************************/

/* Create additional video formats - hopefully clear of any others */
#ifndef GST_VIDEO_FORMAT_NV12
#define GST_VIDEO_FORMAT_NV12 (1000)
#endif
#ifndef GST_VIDEO_FORMAT_NV16
#define GST_VIDEO_FORMAT_NV16 (1001)
#endif
#ifndef GST_VIDEO_FORMAT_RGB16
#define GST_VIDEO_FORMAT_RGB16 (1002)
#endif


/* Extend gst_video_format_get_size to support NV12, NV16 & RGB16 */
int gst_sh_video_format_get_size(GstVideoFormat format, int width, int height);

/* Extend gst_video_format_parse_caps to support NV12, NV16 & RGB16 */
gboolean gst_sh_video_format_parse_caps (
	GstCaps *caps,
	GstVideoFormat *format,
	int *width,
	int *height);

gboolean gst_caps_to_v4l2_format (GstCaps *caps, int *v4l2format);

#endif //GSTSHVIDEOBUFFER_H
