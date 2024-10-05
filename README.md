# TCG Boot

This is an EFI application to secure loading Linux with TPM. The project not yet ready for
end-users.

## How it works?

1. The system firmware will measure TCG Loader.
2. The TCG Loader will measure it's all configurations.
3. The TCG Loader will check Linux's signature and all related files.
4. If signature passed TCG Loader will execute Linux.

Then Linux need to unseal the secret keys and do something with it after this to establish trusted
boot. If the unseal operation is failed that mean the boot processes cannot be trusted anymore.

With this way the Linux and it related files are freely to update without breaking TPM's
measurement.

## Build from source

### Prerequisites

- Rust on the latest stable channel

### Add Rust target

```sh
rustup target add TARGET
```

Replace `TARGET` with your target machine (e.g. `x86_64-unknown-uefi`). Use `rustup target list` to
see the available targets.

### Build

```sh
cargo install --target TARGET --path . --root .
```

It will output `bin/tcg-boot.efi`. You can copy this binary to your EFI system partition and boot
from it.

## Development

The following guide only work on x86-64 machine. First, install the following tools:

- GNU Parted
- Mtools
- OVMF
- QEMU

### Create a Linux VM

Prepare an installation media for any distros you like. The only requirements is it must supports
UEFI.

#### Disable Copy-on-write on BTRFS

If you are using BTRFS and Copy-on-write is enabled you should disable it on `vm` directory with the
following command:

```sh
chattr +C vm
```

#### Create a disk image

```sh
dd if=/dev/zero of=vm/disk1.img count=SECTORS status=progress
```

Replaces `SECTORS` with the number you want. 1 sector is equals to 512 bytes. The size of the image
depend on your distro.

#### Install distro

Copy `OVMF_VARS.fd` from your current system to `vm/bios`. Then start an installation with the
following command:

```sh
TCGBOOT_OVMF_CODE=PATH_TO_OVMF_CODE ./start-vm.sh -cdrom MEDIA
```

Replaces `PATH_TO_OVMF_CODE` with a full path of `OVMF_CODE.fd` and `MEDIA` with a full path of the
installation media. Things to be careful during installation:

- The kernel and its initial ramdisk must be installed to EFI system partition.
- The current utility scripts does not supports more than one `fat32` partitions so don't create
  multiple of it.
- You don't need to install boot loader.

Before shutdown the VM:

- Create an empty directory `EFI/boot` in the EFI system partition.
- Note the kernel file name and its initial ramdisk.
- Note the required information for constructing a kernel command line.

### Install TCG Loader into VM

Prepare a configuration that matched with the VM by duplicating `src/default.conf` to `vm/tcg.conf`
and edit it. Please note that root of the path in the config refer to the root of EFI system
partition. Then run the following command:

```sh
./build-vm.sh
```

Now you can start the VM to test TCG Loader:

```sh
TCGBOOT_OVMF_CODE=PATH_TO_OVMF_CODE ./start-vm.sh
```

You may need to change bios settings in order to boot TCG Loader. You can append `-nographic`
options to `start-vm.sh` to output the QEMU console directly to your terminal. To exit the QEMU,
press <kbd>Ctrl</kbd> + <kbd>A</kbd> then <kbd>X</kbd>.
