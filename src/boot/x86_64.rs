use crate::efi::{
    allocate_pages, get_memory_map, page_count, AllocateType, EfiString, File, MemoryDescriptor,
    MemoryType, Owned, Status, SystemTable,
};
use crate::eprintln;
use alloc::borrow::ToOwned;
use alloc::ffi::CString;
use alloc::vec::Vec;
use core::arch::asm;
use core::cmp::min;
use core::mem::{size_of, transmute, zeroed};
use core::ptr::copy_nonoverlapping;

pub fn boot(mut image: Owned<File>, initrds: Vec<Owned<File>>, cmd_line: &str) -> Status {
    // Allocate page for the kernel.
    let info = image.info().unwrap();
    let size = info.file_size().try_into().unwrap();
    let pages = page_count(size);
    let mut kernel = match allocate_pages(AllocateType::AnyPages, MemoryType::LoaderData, pages, 0)
    {
        Ok(v) => v,
        Err(e) => {
            eprintln!("Cannot allocate {pages} pages for the kernel: {e}.");
            return Status::ABORTED;
        }
    };

    // Load the kernel.
    match image.read(&mut kernel) {
        Ok(v) => assert_eq!(v, size),
        Err(e) => {
            eprintln!("Cannot read {}: {}.", info.file_name(), e);
            return Status::ABORTED;
        }
    }

    // Get setup header.
    let mut bp: BootParams = unsafe { zeroed() };
    let size = (0x0202 + (kernel[0x0201] as usize)) - 0x01f1;
    let size = min(size_of::<SetupHeader>(), size);
    let hdr = &kernel[0x01f1..(0x01f1 + size)];

    unsafe { copy_nonoverlapping::<u8>(hdr.as_ptr(), transmute(&mut bp.hdr), size) };

    // Check kernel version.
    if bp.hdr.header != 0x53726448 || bp.hdr.version < 0x206 {
        eprintln!(
            "{} is not a Linux kernel or it is too old.",
            info.file_name()
        );
        return Status::ABORTED;
    }

    // Setup boot params.
    bp.hdr.vid_mode = 0xFFFD; // ask
    bp.hdr.type_of_loader = 0xFF;
    bp.hdr.loadflags &= !(0x20 | 0x80);

    // Set command line.
    let cmd_line = CString::new(cmd_line).unwrap();
    let cmd_line = cmd_line.as_ptr() as u64;

    bp.hdr.cmd_line_ptr = cmd_line as u32;
    bp.ext_cmd_line_ptr = (cmd_line >> 32) as u32;

    // Get total size of initrd.
    let mut total = 0;
    let initrds: Vec<(Owned<File>, EfiString, usize)> = initrds
        .into_iter()
        .map(|initrd| {
            let info = initrd.info().unwrap();
            let size: usize = info.file_size().try_into().unwrap();

            total += size;

            (initrd, info.file_name().to_owned(), size)
        })
        .collect();

    // Allocate page for all initrd.
    let pages = page_count(total);
    let mut initrd = match allocate_pages(AllocateType::AnyPages, MemoryType::LoaderData, pages, 0)
    {
        Ok(v) => v,
        Err(e) => {
            eprintln!("Cannot allocate {pages} pages for initrd: {e}.");
            return Status::ABORTED;
        }
    };

    // Load all initrd.
    let mut off = 0;

    for (mut i, n, s) in initrds {
        match i.read(&mut initrd[off..]) {
            Ok(v) => assert_eq!(v, s),
            Err(e) => {
                eprintln!("Cannot read {n}: {e}.");
                return Status::ABORTED;
            }
        }

        off += s;
    }

    // Set size of initrd.
    bp.hdr.ramdisk_size = initrd.len() as u32;
    bp.ext_ramdisk_size = (initrd.len() >> 32) as u32;

    // Set initrd.
    let initrd = initrd.as_ptr() as u64;

    bp.hdr.ramdisk_image = initrd as u32;
    bp.ext_ramdisk_image = (initrd >> 32) as u32;

    // Get memory map.
    let (map, key) = match get_memory_map() {
        Ok(v) => v,
        Err(e) => {
            eprintln!("Cannot get memory map: {e}.");
            return Status::ABORTED;
        }
    };

    let map_len = map.len();
    let map = map.as_ptr() as u64;

    // Exit boot services.
    let st = SystemTable::current();

    if let Err(e) = unsafe { st.boot_services().exit_boot_services(key) } {
        eprintln!("Cannot terminate EFI boot services: {e}.");
        return Status::ABORTED;
    }

    // Set EFI info.
    let st = st as *const SystemTable as u64;

    bp.efi_info.efi_loader_signature = 0x34364c45; // EL64
    bp.efi_info.efi_memdesc_size = size_of::<MemoryDescriptor>() as u32;
    bp.efi_info.efi_memdesc_version = 1;
    bp.efi_info.efi_memmap = map as u32;
    bp.efi_info.efi_memmap_hi = (map >> 32) as u32;
    bp.efi_info.efi_memmap_size = (map_len * size_of::<MemoryDescriptor>()) as u32;
    bp.efi_info.efi_systab = st as u32;
    bp.efi_info.efi_systab_hi = (st >> 32) as u32;

    // Jump to the kernel.
    let entry = kernel.addr() + ((bp.hdr.setup_sects as usize + 1) * 512) + 0x200;
    let bp = &bp as *const BootParams;

    unsafe {
        asm!(
            "jmp rax",
            in("rax") entry,
            in("rsi") bp,
            options(noreturn)
        );
    };
}

#[repr(C, packed)]
struct BootParams {
    screen_info: ScreenInfo,
    apm_bios_info: ApmBiosInfo,
    pad2: [u8; 4],
    tboot_addr: u64,
    ist_info: IstInfo,
    acpi_rsdp_addr: u64,
    pad3: [u8; 8],
    hd0_info: [u8; 16],
    hd1_info: [u8; 16],
    sys_desc_table: SysDescTable,
    olpc_ofw_header: OlpcOfwHeader,
    ext_ramdisk_image: u32,
    ext_ramdisk_size: u32,
    ext_cmd_line_ptr: u32,
    pad4: [u8; 112],
    cc_blob_address: u32,
    edid_info: EdidInfo,
    efi_info: EfiInfo,
    alt_mem_k: u32,
    scratch: u32,
    e820_entries: u8,
    eddbuf_entries: u8,
    edd_mbr_sig_buf_entries: u8,
    kbd_status: u8,
    secure_boot: u8,
    pad5: [u8; 2],
    sentinel: u8,
    pad6: [u8; 1],
    hdr: SetupHeader,
    pad7: [u8; 36],
    edd_mbr_sig_buffer: [u32; 16],
    e820_table: [BootE820Entry; 128],
    pad8: [u8; 48],
    eddbuf: [u8; 492],
    pad9: [u8; 276],
}

#[repr(C, packed)]
struct ScreenInfo {
    orig_x: u8,
    orig_y: u8,
    ext_mem_k: u16,
    orig_video_page: u16,
    orig_video_mode: u8,
    orig_video_cols: u8,
    flags: u8,
    unused2: u8,
    orig_video_ega_bx: u16,
    unused3: u16,
    orig_video_lines: u8,
    orig_video_is_vga: u8,
    orig_video_points: u16,
    lfb_width: u16,
    lfb_height: u16,
    lfb_depth: u16,
    lfb_base: u32,
    lfb_size: u32,
    cl_magic: u16,
    cl_offset: u16,
    lfb_linelength: u16,
    red_size: u8,
    red_pos: u8,
    green_size: u8,
    green_pos: u8,
    blue_size: u8,
    blue_pos: u8,
    rsvd_size: u8,
    rsvd_pos: u8,
    vesapm_seg: u16,
    vesapm_off: u16,
    pages: u16,
    vesa_attributes: u16,
    capabilities: u32,
    ext_lfb_base: u32,
    reserved: [u8; 2],
}

#[repr(C)]
struct ApmBiosInfo {
    version: u16,
    cseg: u16,
    offset: u32,
    cseg_16: u16,
    dseg: u16,
    flags: u16,
    cseg_len: u16,
    cseg_16_len: u16,
    dseg_len: u16,
}

#[repr(C)]
struct IstInfo {
    signature: u32,
    command: u32,
    event: u32,
    perf_level: u32,
}

#[repr(C)]
struct SysDescTable {
    length: u16,
    table: [u8; 14],
}

#[repr(C, packed)]
struct OlpcOfwHeader {
    ofw_magic: u32,
    ofw_version: u32,
    cif_handler: u32,
    irq_desc_table: u32,
}

#[repr(C)]
struct EdidInfo {
    dummy: [u8; 128],
}

#[repr(C)]
struct EfiInfo {
    efi_loader_signature: u32,
    efi_systab: u32,
    efi_memdesc_size: u32,
    efi_memdesc_version: u32,
    efi_memmap: u32,
    efi_memmap_size: u32,
    efi_systab_hi: u32,
    efi_memmap_hi: u32,
}

#[repr(C, packed)]
struct SetupHeader {
    setup_sects: u8,
    root_flags: u16,
    syssize: u32,
    ram_size: u16,
    vid_mode: u16,
    root_dev: u16,
    boot_flag: u16,
    jump: u16,
    header: u32,
    version: u16,
    realmode_swtch: u32,
    start_sys_seg: u16,
    kernel_version: u16,
    type_of_loader: u8,
    loadflags: u8,
    setup_move_size: u16,
    code32_start: u32,
    ramdisk_image: u32,
    ramdisk_size: u32,
    bootsect_kludge: u32,
    heap_end_ptr: u16,
    ext_loader_ver: u8,
    ext_loader_type: u8,
    cmd_line_ptr: u32,
    initrd_addr_max: u32,
    kernel_alignment: u32,
    relocatable_kernel: u8,
    min_alignment: u8,
    xloadflags: u16,
    cmdline_size: u32,
    hardware_subarch: u32,
    hardware_subarch_data: u64,
    payload_offset: u32,
    payload_length: u32,
    setup_data: u64,
    pref_address: u64,
    init_size: u32,
    handover_offset: u32,
    kernel_info_offset: u32,
}

#[repr(C, packed)]
struct BootE820Entry {
    addr: u64,
    size: u64,
    ty: u32,
}
