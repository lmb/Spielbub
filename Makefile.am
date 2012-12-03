ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

bin_PROGRAMS = spielbub
spielbub_nomain_SOURCES = \
	debug.pb-c.c \
	buffers.c \
	context.c \
	cpu.c \
	cpu_ops.c \
	graphics.c \
	ipc.c \
	joypad.c \
	logging.c \
	memory.c \
	meta.c \
	rom.c \
	timers.c

spielbub_SOURCES = $(spielbub_nomain_SOURCES) main.c

spielbub_LDADD = $(SDL_LIBS) $(ZeroMQ_LIBS) $(protobufc_LIBS)
spielbub_CFLAGS =$(SDL_CFLAGS) $(ZeroMQ_CFLAGS) $(AM_CFLAGS) $(protobufc_CFLAGS)
dist_bin_SCRIPTS =

TESTS = check_cpu
check_PROGRAMS = check_cpu
check_cpu_SOURCES = check_cpu.c $(spielbub_nomain_SOURCES)
check_cpu_CFLAGS = @CHECK_CFLAGS@ $(spielbub_CFLAGS)
check_cpu_LDADD = @CHECK_LIBS@ $(spielbub_LDADD)

debug.pb-c.c: debug.proto
	$(AM_V_GEN) $(PROTOC) --c_out=. $<



man_MANS =

# Uncomment this line to output gprof profiling info.
#CFLAGS=-pg
