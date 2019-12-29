EXEEXT = .efi

efibin_PROGRAMS = bootx64

bootx64_CFLAGS = \
	-fshort-wchar \
	-I/usr/include/efi \
	-I/usr/include/efi/$(efiarch)
bootx64_LDADD = \
	-lefi
bootx64_LINK = ./link.sh \
	$(CCLD) \
	$@ \
	$(efiarch)
	$(bootx64_CFLAGS) \
	$(CFLAGS) \
	$(AM_LDFLAGS) \
	$(LDFLAGS)
bootx64_SOURCES = \
	main.c

clean-local:
	rm -f *.so
