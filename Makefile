INSTALLDIR=/usr/local
LIBNAME=libeval
VER=1
REV=0
BLD=8
DLLNAME=$(LIBNAME).so
DLLNAMEV=$(DLLNAME).$(VER)
DLLNAMEVR=$(DLLNAMEV).$(REV)
DLLNAMEVRB=$(DLLNAMEVR).$(BLD)

TEST=eval_test
EXES=$(TEST)
ALIB=$(LIBNAME).a
DLIB=$(DLLNAMEVRB)
LIBS=$(ALIB) $(DLIB)
OBJS=eval.o func.o hashtable.o
SRCS=eval.c func.c hashtable.c
HDRS=eval.h func.h hashtable.h

AR=ar
RM=rm -f
CC=gcc
CCOPTS=-fPIC -DVER=$(VER) -DREV=$(REV) -DBLD=$(BLD)
LNOPTS=-lm
SOOPTS=-shared -Wl,-soname,$(LIBNAME)
INSTALL_SRC=install -D
INSTALL_BIN=install -D -m 644
LN=ln -s

MKOBJ=$(CC) $(CCOPTS) -c
MKEXE=$(CC) $(CCOPTS) $(LNOPTS) -o
MKLIB=$(AR) rcs $(LIBNAME).a
MKDLL=$(CC) $(CCOPTS) $(SOOPTS) -o

none:
	@echo "no target specified, please choose one of:"
	@echo "   clean, libs, test, backup, dist, install or remove"

clean:
	@echo "deleting build products"
	@$(RM) *.o
	@$(RM) $(EXES)
	@$(RM) $(LIBS)
	@$(RM) version_str.h
	@$(RM) build_date.h
	@$(RM) package_date.h

veryclean: clean
	@echo "returning directory to pristine state"
	@$(RM) BUILD_DATE
	@$(RM) PACKAGE_DATE

libs: $(OBJS) bld-date
	@echo "building libeval $(VER).$(REV).$(BLD) on $$(date)"
	@$(MKOBJ) $(SRCS)
	@$(MKLIB) $(LIBNAME).a $(OBJS)
	@$(MKDLL) $(DLLNAMEVRB) $(OBJS)

test: $(OBJS)
	@echo "building test harness"
	@$(MKEXE) $(TEST) -DEVAL_TEST $(SRCS)

eval.o: eval.c eval.h build_date.h package_date.h ver-str
	@echo "building eval.o"
	@$(MKOBJ) eval.c

func.o: func.c func.h
	@echo "building standard functions"
	@$(MKOBJ) func.c

hashtable.o: hashtable.c hashtable.h
	@echo "building hashtable"
	@$(MKOBJ) hashtable.c

dist: clean pkg-date
	@echo "making distribution of libeval-$(VER).$(REV).$(BLD)"
	@D=$$PWD;cd ..;tar czvf libeval-$(VER).$(REV).$(BLD).tgz $$D

backup: clean pkg-date
	@echo "making backup on $$(date +%Y/%m/%d) at $$(date +%H:%M)"
	@D=$$PWD;cd ..;tar czf libeval-$$(date +%Y%m%d-%H%M).tgz $$D

install: libs
	@echo "installing libeval into $(INSTALLDIR)"
	$(INSTALL_SRC) eval.h $(INSTALLDIR)/include
	$(INSTALL_BIN) $(LIBNAME).a $(INSTALLDIR)/lib
	$(INSTALL_BIN) $(DLLNAMEVRB) $(INSTALLDIR)/lib
	$(RM) $(INSTALLDIR)/lib/$(DLLNAMEVR)
	$(RM) $(INSTALLDIR)/lib/$(DLLNAMEV)
	$(RM) $(INSTALLDIR)/lib/$(DLLNAME)
	$(LN) $(INSTALLDIR)/lib/$(DLLNAMEVRB) $(INSTALLDIR)/lib/$(DLLNAMEVR)
	$(LN) $(INSTALLDIR)/lib/$(DLLNAMEVRB) $(INSTALLDIR)/lib/$(DLLNAMEV)
	$(LN) $(INSTALLDIR)/lib/$(DLLNAMEVRB) $(INSTALLDIR)/lib/$(DLLNAME)

remove:
	@echo "removing libeval v$(VER).$(REV).$(BLD)"
	@$(RM) $(INSTALLDIR)/include/eval.h
	@$(RM) $(INSTALLDIR)/lib/$(LIBNAME).a
	@$(RM) $(INSTALLDIR)/lib/$(DLLNAME)
	@$(RM) $(INSTALLDIR)/lib/$(DLLNAMEV)
	@$(RM) $(INSTALLDIR)/lib/$(DLLNAMEVR)
	@$(RM) $(INSTALLDIR)/lib/$(DLLNAMEVRB)

remove-all: remove
	@echo "removing all revisions of libeval v$(VER)"
	@$(RM) $(INSTALLDIR)/lib/$(LIBNAME).$(VER).*.so

pkg-date: package_date.h
package_date.h:
	@echo "Libeval version $(VER).$(REV).$(BLD)" > PACKAGE_DATE
	@echo "Copyright (C) 2006, 2007 Jeffrey S. Dutky" >> PACKAGE_DATE
	@echo "Packaged on $$(date +%Y/%m/%d) at $$(date +%H:%M)" >> PACKAGE_DATE
	@echo "static char *G_pkg_date = \"libeval pkg date: $$(date +%Y/%m/%d) at $$(date +%H:%M)\";" > package_date.h

bld-date: build_date.h
build_date.h:
	@echo "Libeval version $(VER).$(REV).$(BLD)" > BUILD_DATE
	@echo "Copyright (C) 2006, 2007 Jeffrey S. Dutky" >> BUILD_DATE
	@echo "Built on $$(date +%Y/%m/%d) at $$(date +%H:%M)" >> BUILD_DATE
	@echo "static char *G_bld_date = \"libeval bld date: $$(date +%Y/%m/%d) at $$(date +%H:%M)\";" > build_date.h

ver-str: version_str.h
version_str.h:
	@echo "static char *G_vrb_str = \"libeval version: $(VER).$(REV).$(BLD)\";" > version_str.h
