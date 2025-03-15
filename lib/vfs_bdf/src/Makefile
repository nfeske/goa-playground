override CPPFLAGS += -I.

vfs_bdf.lib.so: bdf_font.o vfs_bdf.o
	$(CC) -o $@ -Wl,-soname=$@ -shared $(subst ",,$(MAKE_SHARED_LINKER_FLAGS)) \
	      -Wl,-z noexecstack -Wl,--hash-style=sysv $^
