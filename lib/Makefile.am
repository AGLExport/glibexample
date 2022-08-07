AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

noinst_LIBRARIES = \
	libglib_support.a

libglib_support_a_SOURCES = \
	glibhelper-unix-socket-support-util.c \
	glibhelper-unix-socket-support-server.c \
	glibhelper-unix-socket-support-client.c \
	glibhelper-unix-socket-support-internal.c \
	glibhelper-timerfd-support.c \
	glibhelper-signal.c

libglib_support_a_CFLAGS = \
	-g \
	-I$(top_srcdir)/include \
	@GLIB2_CFLAGS@ \
	@GIO2_CFLAGS@ \
	-D_GNU_SOURCE

# configure option 
if ENABLE_ADDRESS_SANITIZER
CFLAGS   += -fsanitize=address
endif

if ENABLE_GCOV
CFLAGS   += -coverage
endif

CLEANFILES = *.gcda *.gcno
