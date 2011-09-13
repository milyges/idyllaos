
include $(TOPDIR)/scripts/common.mak

all: $(APPNAME)
	@true

install:
	@mkdir -p $(DESTDIR)/$(APPDIR)
	@echo " INSTALL $(APPDIR)/$(APPNAME)"
	@cp $(APPNAME) $(DESTDIR)/$(APPDIR)/$(APPNAME)

clean:
	@echo " CLEAN   $(APPNAME) $(OBJECTS)"
	@rm -f $(APPNAME) $(OBJECTS)

$(APPNAME): $(OBJECTS)
	@echo " LD      $(APPNAME)"
	@$(LD) $(LDFLAGS) -o $(APPNAME) $(OBJECTS) $(LDADD)
	@echo " STRIP   $(APPNAME)"
	@$(STRIP) --strip-debug $(APPNAME)
