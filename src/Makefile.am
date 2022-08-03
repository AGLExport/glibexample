AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

bin_PROGRAMS = \
	glib_sv_loop glib_sv_loop_mt \
	glib_cli_loop glib_cli_loop_mt

glib_sv_loop_SOURCES = \
	server-main.c

# options
# Additional library
glib_sv_loop_LDADD = \
	$(top_srcdir)/lib/libglib_support.a \
	-lrt -lpthread \
	@GLIB2_LIBS@ \
	@GIO2_LIBS@

# C compiler options
glib_sv_loop_CFLAGS = \
	-g \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/include \
	@GLIB2_CFLAGS@ \
	@GIO2_CFLAGS@ \
	-D_GNU_SOURCE

# Linker options
glib_sv_loop_LDFLAGS = 


glib_sv_loop_mt_SOURCES = \
	server-main-mt.c

# options
# Additional library
glib_sv_loop_mt_LDADD = \
	$(top_srcdir)/lib/libglib_support.a \
	-lrt -lpthread \
	@GLIB2_LIBS@ \
	@GIO2_LIBS@

# C compiler options
glib_sv_loop_mt_CFLAGS = \
	-g \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/include \
	@GLIB2_CFLAGS@ \
	@GIO2_CFLAGS@ \
	-D_GNU_SOURCE

# Linker options
glib_sv_loop_mt_LDFLAGS = 


glib_cli_loop_SOURCES = \
	client-main.c

# options
# Additional library
glib_cli_loop_LDADD = \
	$(top_srcdir)/lib/libglib_support.a \
	-lrt -lpthread \
	@GLIB2_LIBS@ \
	@GIO2_LIBS@

# C compiler options
glib_cli_loop_CFLAGS = \
	-g \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/include \
	@GLIB2_CFLAGS@ \
	@GIO2_CFLAGS@ \
	-D_GNU_SOURCE

# Linker options
glib_cli_loop_LDFLAGS = 


glib_cli_loop_mt_SOURCES = \
	client-main-mt.c

# options
# Additional library
glib_cli_loop_mt_LDADD = \
	$(top_srcdir)/lib/libglib_support.a \
	-lrt -lpthread \
	@GLIB2_LIBS@ \
	@GIO2_LIBS@

# C compiler options
glib_cli_loop_mt_CFLAGS = \
	-g \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/include \
	@GLIB2_CFLAGS@ \
	@GIO2_CFLAGS@ \
	-D_GNU_SOURCE

# Linker options
glib_cli_loop_mt_LDFLAGS = 

# configure option 
if ENABLE_ADDRESS_SANITIZER
CFLAGS   += -fsanitize=address
endif

if ENABLE_GCOV
CFLAGS   += -coverage
endif

CLEANFILES = *.gcda *.gcno
