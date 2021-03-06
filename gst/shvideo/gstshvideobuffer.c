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
#include <uiomux/uiomux.h>
#include <shveu/shveu.h>

#include "gstshvideobuffer.h"

static GstBufferClass *parent_class;

/**
 * Initialize the buffer
 * \param shbuffer GstSHVideoBuffer object
 * \param g_class GClass pointer
 */
static void
gst_sh_video_buffer_init (GstSHVideoBuffer *shbuffer, gpointer g_class)
{
	/* Mark the buffer as not allocated by us */
	shbuffer->allocated = 0;
}

/**
 * Create a new SH buffer
 * \param width Width of frame
 * \param height Height of frame
 * \param fmt Video format
 */
GstBuffer *gst_sh_video_buffer_new(UIOMux *uiomux, gint width, gint height, int fmt)
{
	GstSHVideoBuffer *buf;
	gint size;

	// TODO the size calc should really take into account that the chroma plane needs to
	// be 32-byte aligned. We should also cover min width/height requirements of all IP
	// so that the buffer can be used with all HW.
	// This also means that the buffer can't be used by non-SH elements.

	size = size_y(fmt, width * height) + size_c(fmt, width * height);

	buf = (GstSHVideoBuffer*)gst_mini_object_new(GST_TYPE_SH_VIDEO_BUFFER);
	g_return_val_if_fail(buf != NULL, NULL);

	/* User space address */
	GST_BUFFER_DATA(buf) = uiomux_malloc(uiomux, UIOMUX_SH_VEU, size, 32);
	GST_BUFFER_SIZE(buf) = size;

	if (GST_BUFFER_DATA(buf) == NULL) {
		gst_mini_object_unref(GST_MINI_OBJECT(buf));
		return NULL;
	}

	/* Mark the buffer as allocated by us, so it needs freeing */
	buf->allocated = 1;
	buf->allocated_size = size;
	buf->uiomux = uiomux;
	buf->format = fmt;

	return GST_BUFFER(buf);
}


/**
 * Finalize the buffer
 * \param shbuffer GstSHVideoBuffer object
 */
static void
gst_sh_video_buffer_finalize (GstSHVideoBuffer *shbuffer)
{
	if (shbuffer->allocated && shbuffer->uiomux) {
		/* Free the buffer */
		uiomux_free (shbuffer->uiomux, UIOMUX_SH_VEU,
			GST_BUFFER_DATA(shbuffer), shbuffer->allocated_size);
	}

	/* Set malloc_data to NULL to prevent parent class finalize
	* from trying to free allocated data. This buffer is used only in HW
	* address space, where we don't allocate data but just map it.
	*/
	GST_BUFFER_MALLOCDATA(shbuffer) = NULL;
	GST_BUFFER_DATA(shbuffer) = NULL;

	GST_MINI_OBJECT_CLASS (parent_class)->finalize (GST_MINI_OBJECT (shbuffer));
}

/**
 * Initialize the buffer class
 * \param g_class GClass pointer
 * \param class_data Optional data pointer
 */
static void
gst_sh_video_buffer_class_init (gpointer g_class, gpointer class_data)
{
	GstMiniObjectClass *mini_object_class = GST_MINI_OBJECT_CLASS (g_class);

	parent_class = g_type_class_peek_parent (g_class);

	mini_object_class->finalize = (GstMiniObjectFinalizeFunction)
			gst_sh_video_buffer_finalize;
}

GType
gst_sh_video_buffer_get_type (void)
{
	static GType gst_sh_video_buffer_type;

	if (G_UNLIKELY (gst_sh_video_buffer_type == 0)) {
		static const GTypeInfo gst_sh_video_buffer_info = {
			sizeof (GstBufferClass),
			NULL,
			NULL,
			gst_sh_video_buffer_class_init,
			NULL,
			NULL,
			sizeof (GstSHVideoBuffer),
			0,
			(GInstanceInitFunc) gst_sh_video_buffer_init,
			NULL
		};
		gst_sh_video_buffer_type = g_type_register_static (GST_TYPE_BUFFER,
				"GstSHVideoBuffer", &gst_sh_video_buffer_info, 0);
	}
	return gst_sh_video_buffer_type;
}


/***************************** Helper functions *****************************/

/* Extend gst_video_format_get_size to support NV12, NV16 & RGB16 */
int gst_sh_video_format_get_size(GstVideoFormat format, int width, int height)
{
	int size = 0;

	if (format == GST_VIDEO_FORMAT_NV12)
		size = (width * height * 3)/2;
	else if (format == GST_VIDEO_FORMAT_NV16)
		size = width * height * 2;
	else if (format == GST_VIDEO_FORMAT_RGB16)
		size = width * height * 2;
	else
		size = gst_video_format_get_size(format, width, height);

	return size;
}

/* Extend gst_video_format_parse_caps to support NV12, NV16 & RGB16 */
gboolean gst_sh_video_format_parse_caps (
	GstCaps *caps,
	GstVideoFormat *format,
	int *width,
	int *height)
{
	GstStructure *structure;
	guint32 fourcc;
	gint bpp = 0;
	gboolean found = FALSE;

	*format = GST_VIDEO_FORMAT_UNKNOWN;

	structure = gst_caps_get_structure(caps, 0);

	if (structure && format) {
		gst_structure_get_fourcc(structure, "format", &fourcc);
		gst_structure_get_int(structure, "bpp", &bpp);

		if (fourcc == GST_MAKE_FOURCC('N', 'V', '1', '2')) {
			*format = GST_VIDEO_FORMAT_NV12;
		} else if (fourcc == GST_MAKE_FOURCC('N', 'V', '1', '6')) {
			*format = GST_VIDEO_FORMAT_NV16;
		} else if (gst_video_format_parse_caps (caps, format, width, height)) {
			/* Nothing */
		} else if (gst_structure_has_name (structure, "video/x-raw-rgb")) {
			if (bpp == 16)
				*format = GST_VIDEO_FORMAT_RGB16;
			if (bpp == 32)
				*format = GST_VIDEO_FORMAT_RGBx;
		}
	}

	if (*format != GST_VIDEO_FORMAT_UNKNOWN)
		found = TRUE;

	if (width)
		found &= gst_structure_get_int(structure, "width", width);
	if (height)
		found &= gst_structure_get_int(structure, "height", height);

	return found;
}

gboolean gst_caps_to_renesas_format (GstCaps *caps, ren_vid_format_t *ren_fmt)
{
	GstVideoFormat format;

	*ren_fmt = 0;

	if (gst_sh_video_format_parse_caps (caps, &format, NULL, NULL)) {
		if (format == GST_VIDEO_FORMAT_RGB16)
			*ren_fmt = REN_RGB565;
		else if (format == GST_VIDEO_FORMAT_RGBx)
			*ren_fmt = REN_RGB32;
		else if (format == GST_VIDEO_FORMAT_NV12)
			*ren_fmt = REN_NV12;
		else if (format == GST_VIDEO_FORMAT_NV16)
			*ren_fmt = REN_NV16;
	}

	if (*ren_fmt == 0) {
		GST_ERROR("failed to get format from cap");
		return FALSE;
	}

	return TRUE;
}

int get_renesas_format (GstVideoFormat format)
{
	int ren_fmt = 0;

	if (format == GST_VIDEO_FORMAT_RGB16)
		ren_fmt = REN_RGB565;
	else if (format == GST_VIDEO_FORMAT_RGBx)
		ren_fmt = REN_RGB32;
	else if (format == GST_VIDEO_FORMAT_NV12)
		ren_fmt = REN_NV12;
	else if (format == GST_VIDEO_FORMAT_NV16)
		ren_fmt = REN_NV16;

	return ren_fmt;
}

void *get_c_addr (void *y, ren_vid_format_t ren_format, int width, int height)
{
	if (size_c(ren_format, width*height)) {
		return (y + size_y(ren_format, width*height));
	}
	return NULL;
}
