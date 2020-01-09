NULL = #

DOC_MODULE = clutter-gst
DOC_MODULE_VERSION = @CLUTTER_GST_API_VERSION@
DOC_MAIN_SGML_FILE = clutter-gst-docs.xml
DOC_SOURCE_DIR = $(top_srcdir)/clutter-gst $(top_builddir)/clutter-gst

SCANGOBJ_OPTIONS = --type-init-func="extern void clutter_gst_init (int *, char **); clutter_gst_init(0,0)"
SCAN_OPTIONS = --rebuild-types
MKDB_OPTIONS = --xml-mode --output-format=xml --name-space=clutter_gst
MKTMPL_OPTIONS =
FIXXREF_OPTIONS = \
	--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/glib \
	--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/gobject \
	--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/cogl \
	--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/clutter \
	--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/gdk-pixbuf \
	--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/gstreamer-1.0 \
	$(NULL)

HFILE_GLOB = $(top_srcdir)/clutter-gst/*.h $(top_builddir)/clutter-gst/*.h
CFILE_GLOB = $(top_srcdir)/clutter-gst/*.c

IGNORE_HFILES =				\
	clutter-gst.h			\
	clutter-gst-debug.h		\
	clutter-gst-private.h		\
	clutter-gst-auto-video-sink.h	\
	$(NULL)

HTML_IMAGES =

content_files = version.xml
expand_content_files =

GTKDOC_CFLAGS = -I$(top_srcdir) -I$(top_builddir) $(CLUTTER_GST_CFLAGS) $(GST_CFLAGS)
GTKDOC_LIBS = $(top_builddir)/clutter-gst/libclutter-gst-@CLUTTER_GST_MAJORMINOR@.la $(CLUTTER_GST_LIBS)

include $(top_srcdir)/gtk-doc.make

EXTRA_DIST += version.xml.in
