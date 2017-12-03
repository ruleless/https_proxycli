ROOTDIR=..

CFLAGS:= -I$(ROOTDIR) -D_USE_KLOG
CXXFLAGS:= -I$(ROOTDIR) -D_USE_KLOG

LDFLAGS:= -L$(ROOTDIR)/lib -llog -lkmem -lpthread

TARGET:=proxy.out

include $(ROOTDIR)/build.mak
