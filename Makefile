# -*- Makefile -*-
# ======================================================================

TARGETS = \
  $(ACMACS_VIRUS_LIB) \
  $(DIST)/virus-name \
  $(DIST)/test-virus-name \
  $(DIST)/test-passage

all: install

include $(ACMACSD_ROOT)/share/Makefile.config

# ----------------------------------------------------------------------

SRC_DIR = $(abspath $(ACMACSD_ROOT)/sources)

ACMACS_VIRUS_SOURCES = \
  virus-name-fields.cc virus-name-v1.cc \
  virus-name-normalize.cc virus-name-parse.cc \
  virus-name.cc \
  passage.cc reassortant.cc

# ----------------------------------------------------------------------

ACMACS_VIRUS_LIB_MAJOR = 1
ACMACS_VIRUS_LIB_MINOR = 0
ACMACS_VIRUS_LIB = $(DIST)/$(call shared_lib_name,libacmacsvirus,$(ACMACS_VIRUS_LIB_MAJOR),$(ACMACS_VIRUS_LIB_MINOR))

# ----------------------------------------------------------------------

lib: $(ACMACS_VIRUS_LIB)

test: install | $(TARGETS)
	test/test
.PHONY: test

# ----------------------------------------------------------------------

install: $(TARGETS)
	$(call install_lib,$(ACMACS_VIRUS_LIB))
	$(call symbolic_link_wildcard,$(DIST)/virus-*,$(AD_BIN))
	if [ ! -d $(AD_INCLUDE)/acmacs-virus ]; then mkdir $(AD_INCLUDE)/acmacs-virus; fi
	ln -sf $(abspath cc)/*.hh $(AD_INCLUDE)/acmacs-virus

.PHONY: install test

LDLIBS = \
  $(AD_LIB)/$(call shared_lib_name,libacmacsbase,1,0) \
  $(AD_LIB)/$(call shared_lib_name,liblocationdb,1,0) \
  $(CXX_LIBS)

# ----------------------------------------------------------------------

RTAGS_TARGET = $(ACMACS_VIRUS_LIB)

# ----------------------------------------------------------------------

$(ACMACS_VIRUS_LIB): $(patsubst %.cc,$(BUILD)/%.o,$(ACMACS_VIRUS_SOURCES)) | $(DIST)
	$(call echo_shared_lib,$@)
	$(call make_shared_lib,libacmacsvirus,$(ACMACS_VIRUS_LIB_MAJOR),$(ACMACS_VIRUS_LIB_MINOR)) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(DIST)/%: $(BUILD)/%.o | $(DIST) $(ACMACS_VIRUS_LIB)
	$(call echo_link_exe,$@)
	$(CXX) $(LDFLAGS) -o $@ $^ $(ACMACS_VIRUS_LIB) $(LDLIBS) $(AD_RPATH)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
