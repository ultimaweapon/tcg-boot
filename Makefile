.PHONY: all clean install

# common configurations
SOURCES = \
	src/config.c \
	src/image.c \
	src/linux.c \
	src/main.c \
	src/page.c \
	src/path.c

# build options
CFLAGS = \
	-ffreestanding \
	-fshort-wchar \
	-fno-stack-protector \
	-fpic \
	-O3 \
	-Wall \
	-Wextra

LDFLAGS = \
	-nostdlib \
	-shared \
	-Wl,--no-undefined \
	-Wl,-Bsymbolic

LDLIBS = \
	-lefi \
	-lgnuefi

CPFLAGS = \
	--strip-unneeded \
	-R .gnu.hash \
	-R .eh_frame \
	-R .reloc \
	-R '.note.*' \
	-R .dynsym \
	-R .dynstr \
	-R .eh_frame_hdr \
	-R .comment \
	-R .symtab \
	-R .strtab \
	-R .shstrtab

# platform-specific configurations
ifeq ($(shell uname -m),x86_64)
SOURCES += src/arch/x86/linux.c
CFLAGS += -DGNU_EFI_USE_MS_ABI -mno-red-zone
LDFLAGS += -T /usr/lib/elf_x86_64_efi.lds
OUTPUT = bootx64.efi
FORMAT = efi-app-x86_64
STARTUP = /usr/lib/crt0-efi-x86_64.o
else
$(error the architecture is not supported yet)
endif

# build rules
all: $(OUTPUT)

clean:
	$(RM) $(OUTPUT) $(OUTPUT).so $(SOURCES:.c=.o)

install:

$(OUTPUT): $(OUTPUT).so
	objcopy $(CPFLAGS) --target $(FORMAT) $< $@

$(OUTPUT).so: $(SOURCES:.c=.o)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(STARTUP) $+ $(LDLIBS)
