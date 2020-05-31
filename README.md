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

### Running TCG Loader in QEMU

First, create a disk image with the following command:

```sh
./src/tcg-loader/mkdisk.sh src/tcg-loader/bootx64.efi PATH_TO_KERNEL disk.img
```

Then, you can start QEMU with the following command:

```sh
qemu-system-ARCH -bios PATH_TO_OVMF.fd -drive file=disk.img -nographic
```

To terminate QEMU, press `CTRL + A` then `X`.
