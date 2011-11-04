.PHONY: clean all pre

prefix=usr
sbindir=$(prefix)/sbin
bindir=$(prefix)/bin
libdir=$(prefix)/lib
datadir=$(prefix)/share
sysconfigdir=/etc


FTD=ftd
FTD_SRC= CurlDownloader.cpp \
		 DirWatcher.cpp \
		 Downloader.cpp \
		 filetransferdaemon.cpp \
		 FtdApp.cpp \
		 FtdConfig.cpp \
		 SocketFdDownloader.cpp \
		 TorrentDownloader.cpp 

FTDCLIENT=ftdclient
FTDCLIENT_SRC= ftdclient.cpp

UPLOAD_CGI=upload.cgi
UPLOAD_CGI_SRC= upload.cpp \
			   FtdConfig.cpp

VPATH = src

VERSION ?= unknown
CFGPATH ?= $(CURDIR)/ftdconfig.ini

CXXFLAGS_EXTRA = -g -D_FILE_OFFSET_BITS=64  -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE -D__STDC_FORMAT_MACROS -Wall \
				 -DPACKAGE_VERSION="\"$(VERSION)\"" -DCFGPATH="\"$(CFGPATH)\"" \
				 $(shell pkg-config --cflags libeutils libtorrent-rasterbar sigc++-2.0 libcurl)
				 #-Wextra -Wold-style-cast -Woverloaded-virtual -Wsign-promo
 
#LD_LIBTORRENT=-Wl,-Bstatic $(shell pkg-config --libs libtorrent-rasterbar geoip libssl) \
				#-lboost_system-mt -lboost_date_time-mt -lboost_filesystem-mt -lboost_thread-mt -Wl,-Bdynamic

LD_LIBTORRENT= $(shell pkg-config --libs libtorrent-rasterbar) \
				-lboost_system-mt -lboost_date_time-mt -lboost_filesystem-mt -lboost_thread-mt

LDFLAGS_EXTRA = $(shell pkg-config --libs libeutils sigc++-2.0) $(shell curl-config --libs) -lpopt -ltcl8.5

FTD_OBJS=$(FTD_SRC:%.cpp=%.o)
FTDCLIENT_OBJS=$(FTDCLIENT_SRC:%.cpp=%.o)
UPLOAD_CGI_OBJS=$(UPLOAD_CGI_SRC:%.cpp=%.o)

DEPDIR = .deps
%.o : %.cpp
	$(COMPILE.cpp) $(CXXFLAGS_EXTRA) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -o $@ $<
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

-include $(SRCS:%.cpp=.deps/%.Po)

all: pre $(FTD) $(FTDCLIENT) $(UPLOAD_CGI)

pre:
	@@if [ ! -d .deps ]; then mkdir .deps; fi

$(FTD): $(FTD_OBJS)
	$(CXX) $^ $(LDFLAGS) $(LDFLAGS_EXTRA) $(LD_LIBTORRENT) -o $@
$(FTDCLIENT): $(FTDCLIENT_OBJS)
	$(CXX) $(LDFLAGS) $(LDFLAGS_EXTRA) $^ -o $@
$(UPLOAD_CGI): $(UPLOAD_CGI_OBJS)
	$(CXX) $(LDFLAGS) $(LDFLAGS_EXTRA) $^ -o $@

clean:                                                                          
	rm -f *~ $(FTD) $(FTDCLIENT) $(UPLOAD_CGI) $(FTD_OBJS) $(FTDCLIENT_OBJS) $(UPLOAD_CGI_OBJS)
	rm -rf .deps

install:
	install -D --mode=0700 ftd $(DESTDIR)/$(sbindir)/ftd
	install -D --mode=0755 ftdclient $(DESTDIR)/$(bindir)/ftdclient
	install -D --mode=0644 ftdconfig.ini $(DESTDIR)/$(datadir)/ftd/ftdconfig.default
	install -D --mode=0644 php/ipc.php $(DESTDIR)/$(datadir)/ftd/ipc.php
	install -D --mode=0700 --owner=www-data upload.cgi $(DESTDIR)/$(libdir)/cgi-bin/upload.cgi
