##############################################################
##############                                 ###############
#############  Laidout src/interfaces Makefile  ##############
##############                                 ###############
##############################################################

include ../../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm -lpng `imlib2-config --libs` -L$(LAXDIR) -L$(LAXIDIR)
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= $(HIDEGARBAGE) -Wall $(DEBUGFLAGS) -I$(LAXDIR)/.. `pkg-config --cflags freetype2`


objs= \
	actionarea.o \
	nupinterface.o \
	aligninterface.o \
	anchorinterface.o \
	pagerangeinterface.o \
	graphicalshell.o \
	objectindicator.o \
	paperinterface.o \
	cloneinterface.o \
	animationinterface.o \
	objecttree.o \
	pagemarkerinterface.o \
	objectfilterinterface.o \



interfaces: $(objs)

depends:
	../utils/makedependencies -fmakedepend -I$(LAXDIR)/.. *.cc

include makedepend

hidegarbage:
	../hidegarbage *.cc

unhidegarbage:
	../hidegarbage -0 *.cc


.PHONY: clean interfaces hidegarbage unhidegarbage
clean:
	rm -f *.o

