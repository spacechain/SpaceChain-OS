AM_CFLAGS = -Wall -g

if SYMBOLIC_FUNCTIONS
SYMFUNCS = -Wl,-Bsymbolic-functions
else
SYMFUNCS =
endif

lib_LTLIBRARIES = libsalsa.la
libsalsa_la_SOURCES = control.c cards.c
if BUILD_PCM
libsalsa_la_SOURCES += pcm.c pcm_params.c pcm_misc.c
endif
if BUILD_ASYNC
libsalsa_la_SOURCES += async.c
endif
if BUILD_MIXER
libsalsa_la_SOURCES += hcontrol.c mixer.c
endif
if BUILD_RAWMIDI
libsalsa_la_SOURCES += rawmidi.c
endif
if BUILD_HWDEP
libsalsa_la_SOURCES += hwdep.c
endif
if BUILD_TIMER
libsalsa_la_SOURCES += timer.c
endif

libsalsa_la_LDFLAGS = -version-info 0:1:0 $(SYMFUNCS)
libsalsa_la_LIBADD = @SALSA_DEPLIBS@

alsaincludedir = $(includedir)/alsa

alsainclude_HEADERS = asoundlib.h version.h recipe.h asound.h asoundef.h \
	global.h input.h output.h error.h \
	control.h ctl_func.h ctl_macros.h \
	pcm.h pcm_func.h pcm_macros.h

if BUILD_MIXER
alsainclude_HEADERS += \
	hcontrol.h hctl_func.h hctl_macros.h \
	mixer.h mixer_func.h mixer_macros.h
endif
if BUILD_RAWMIDI
alsainclude_HEADERS += \
	rawmidi.h rawmidi_func.h rawmidi_macros.h
endif
if BUILD_HWDEP
alsainclude_HEADERS += \
	hwdep.h hwdep_func.h hwdep_macros.h
endif
if BUILD_TIMER
alsainclude_HEADERS += \
	timer.h timer_func.h timer_macros.h
endif
if BUILD_CONF
alsainclude_HEADERS += \
	conf.h
endif
if BUILD_SEQ
alsainclude_HEADERS += \
	seq.h seq_event.h seqmid.h
endif

noinst_HEADERS = local.h 

EXTRA_DIST = asoundlib-head.h asoundlib-tail.h recipe.h.in version.h.in Versions

DISTCLEANFILES = asoundlib.h recipe.h version.h

if COMPAT_ABI
lib_LTLIBRARIES += libasound.la
libasound_la_SOURCES = ctl_abi.c input_abi.c output_abi.c global_abi.c error_abi.c
if BUILD_PCM
libasound_la_SOURCES += pcm_abi.c
endif
if BUILD_MIXER
libasound_la_SOURCES += hctl_abi.c mixer_abi.c
endif
if BUILD_RAWMIDI
libasound_la_SOURCES += rawmidi_abi.c
endif
if BUILD_HWDEP
libasound_la_SOURCES += hwdep_abi.c
endif
if BUILD_TIMER
libasound_la_SOURCES += timer_abi.c
endif
if BUILD_CONF
libasound_la_SOURCES += conf_abi.c
endif

libasound_la_SOURCES += $(libsalsa_la_SOURCES)

if COMPAT_ABI
VSYMS = -Wl,--version-script=Versions
else
VSYMS =
endif

libasound_la_LDFLAGS = -version-info 2:0:0 $(VSYMS) $(SYMFUNCS)
libasound_la_LIBADD = @SALSA_DEPLIBS@

endif
