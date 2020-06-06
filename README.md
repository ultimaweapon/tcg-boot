# TCG Boot

This is EFI application to secure loading Linux with TPM.

## How it works?

1. The system firmware will measure TCG Loader.
2. The TCG Loader will measure it's all configurations.
3. The TCG Loader will check Linux's signature and all related files.
4. If signature passed TCG Loader will execute Linux.

Then Linux need to unseal the secret keys and do something with it after this to
establish trusted boot. If the unseal operation is failed that mean the boot
processes cannot be trusted anymore.

With this way the Linux and it related files are freely to update without
breaking TPM's measurement.

## Dependencies

- GNU-EFI
- Kernel User-space API headers (UAPI)

## Build

If you are building from distribution tarball the steps to build will be the
same as other software:

```sh
./configure
make
make install
```

Otherwise; you need GNU Autotools in addition to C toolchain to run the
following command:

```sh
./autogen.sh
```

Then do the same steps as building from distribution tarball.

## Development

First, install the following tools:

- GNU Parted
- Mtools
- OVMF
- QEMU

### Create a Linux VM

Prepare an installation media for any distros you like. The only requirements is
it must supports UEFI. Then create a directory for keeping files that related to
the VM. Copy `OVMF_VARS.fd` from the current system to that directory. Then
switch to that directory and create a disk image with the following command:

```sh
dd if=/dev/zero of=disk1.img count=SECTORS status=progress
```

Replaces `SECTORS` with the number you want. 1 sector is equals to 512 bytes.
When finished you can start a VM with the following command:

```sh
qemu-system-x86_64 -enable-kvm -cpu host -m 1G -machine q35 -drive if=pflash,format=raw,readonly,file=OVMF_CODE -drive if=pflash,format=raw,file=OVMF_VARS.fd -drive if=virtio,format=raw,file=disk1.img -cdrom MEDIA
```

Replaces `OVMF_CODE` with a full path of `OVMF_CODE.fd` and `MEDIA` with a full
path of the installation media. Things to be careful durring installation:

- The kernel and its initial ramdisk must be installed to EFI system partition.
- The current utility scripts does not supports more than one `fat32` partitions
so don't create multiple of it.
- You don't need to install boot loader.
- You need to create an empty directory `EFI/boot` in the EFI system partition
before shutdown or reboot.

### Install TCG Loader into VM

Prepare a configuration that matched with your VM by duplicating
`src/tcg-loader/default.conf`. Then run the following command:

```sh
./install-img.sh PATH_TO_IMAGE src/tcg-loader/bootx64.efi PATH_TO_CONFIG
```

Now you can start the VM to test TCG Loader. You may need to change bios
settings in order to boot TCG Loader.
