EXEEXT = .efi

if X86_64
efibin_PROGRAMS = bootx64
endif

bootx64_CFLAGS = \
	-ffreestanding \
	-fno-stack-check \
	-fno-stack-protector \
	-fpic \
	-fshort-wchar \
	-mno-red-zone \
	-DGNU_EFI_USE_MS_ABI \
	-I/usr/include/efi \
	-I/usr/include/efi/$(efiarch)
bootx64_LDADD = \
	-lefi -lgnuefi
bootx64_LINK = ./link.sh \
	$(CCLD) \
	$(OBJCOPY) \
	$@ \
	$(efiarch)
bootx64_SOURCES = \
	main.c

clean-local:
	rm -f *.so
