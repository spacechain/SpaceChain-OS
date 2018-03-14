#*********************************************************************************************************
# genarate symbol.c and symbol.h
#*********************************************************************************************************
UNAME = $(shell uname -sm)

ifneq (,$(findstring Linux, $(UNAME)))
HOST_OS = linux
endif
ifneq (,$(findstring Darwin, $(UNAME)))
HOST_OS = darwin
endif
ifneq (,$(findstring Macintosh, $(UNAME)))
HOST_OS = darwin
endif
ifneq (,$(findstring CYGWIN, $(UNAME)))
HOST_OS = windows
endif
ifneq (,$(findstring windows, $(UNAME)))
HOST_OS = windows
endif
ifneq (,$(findstring MINGW, $(UNAME)))
HOST_OS = linux
endif

ifeq ($(HOST_OS),)
$(error Unable to determine HOST_OS from uname -sm: $(UNAME)!)
endif

ifeq ($(HOST_OS), windows) 
all:
	makesymbol.bat
else
all:
	chmod +x makesymbol.sh
	./makesymbol.sh
endif

#*********************************************************************************************************
# END
#*********************************************************************************************************
