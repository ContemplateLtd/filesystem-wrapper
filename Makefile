### Configuration section
OCAMLOUTDIR=_build
include Makefile.common

MODULES := Filesystem

all: $(OCAMLOUTDIR)/filesystem.cma \
     $(OCAMLOUTDIR)/filesystem.cmxa \
     $(OCAMLOUTDIR)/$(MODULES:=.cmi)

.PHONY: all clean test

ifeq ($(findstring win,$(target)),win)
Filesystem_IMPL := Filesystem-win32.ml
C_OBJS          := win32unix.o
EXE_EXT         := .exe
else
Filesystem_IMPL := Filesystem-unix.ml
C_OBJS          :=
EXE_EXT         :=
endif

### End of configuration section

# Rules for linking the library, bytecode and native versions

$(OCAMLOUTDIR)/filesystem.cmxa: $(OCAMLOUTDIR)/$(MODULES:=.cmx) $(patsubst %,$(OCAMLOUTDIR)/%,$(C_OBJS))
	$(call COMPILE,LINK,$@,$(OCAMLOPT) -a -o $@ -I $(OCAMLOUTDIR) $(MODULES:=.cmx) $(C_OBJS))

$(OCAMLOUTDIR)/filesystem.cma: $(OCAMLOUTDIR)/$(MODULES:=.cmo) $(patsubst %,$(OCAMLOUTDIR)/%,$(C_OBJS))
	$(call COMPILE,LINK,$@,$(OCAMLC) -a -o $@ -I $(OCAMLOUTDIR) $(MODULES:=.cmo) $(C_OBJS))

# The test program

test: $(OCAMLOUTDIR)/test$(EXE_EXT)

$(OCAMLOUTDIR)/test$(EXE_EXT): $(OCAMLOUTDIR)/filesystem.cmxa $(OCAMLOUTDIR)/test.cmx
	$(call COMPILE,LINK,$@,$(OCAMLOPT) -o $@ -I _build unix.cmxa $^)

$(OCAMLOUTDIR)/test.cmx: $(OCAMLOUTDIR)/Filesystem.cmi

# Special rules for the target-dependent Filesystem module

$(OCAMLOUTDIR)/Filesystem.ml: $(Filesystem_IMPL) | $(OCAMLOUTDIR)
	$(call COMPILE,COPY,$<,cp $< $@)

$(OCAMLOUTDIR)/Filesystem.cmx: $(OCAMLOUTDIR)/Filesystem.ml | $(OCAMLOUTDIR)
	$(call COMPILE,OCAMLOPT,$<,$(CMD))

$(OCAMLOUTDIR)/Filesystem.cmo: $(OCAMLOUTDIR)/Filesystem.ml | $(OCAMLOUTDIR)
	$(call COMPILE,OCAMLC,$<,$(CMD) $(ARGS))

# Rule for building the C stubs

$(OCAMLOUTDIR)/%.o: %.c
	$(call COMPILE,OCAMLOPT,$<,$(OCAMLOPT) -ccopt -o -ccopt $@ -c -ccopt -g -ccopt -Wall -ccopt -Wextra $<)

clean:
	rm -rf _build
