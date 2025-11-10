#include "smokerand/pe32loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#define ERRMSG_MAXLEN 256
static char errmsg[ERRMSG_MAXLEN] = "";


#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64) || defined(__MINGW32__) || defined(__MINGW64__)
#include <windows.h>
void *execbuffer_alloc(size_t len)
{
    void *buf = VirtualAlloc(NULL, len, MEM_COMMIT, PAGE_READWRITE);
    DWORD dummy;
    if (buf != NULL) {
        VirtualProtect(buf, len, PAGE_EXECUTE_READWRITE, &dummy);
    }
    return buf;
}

void execbuffer_free(void *buf)
{
    VirtualFree(buf, 0, MEM_RELEASE);
}
#else
void *execbuffer_alloc(size_t len)
{
    return calloc(len, 1);
}

void execbuffer_free(void *buf)
{
    free(buf);
}
#endif

static inline uint32_t read_u32(FILE *fp, uint32_t offset)
{
    uint32_t tmp;
    fseek(fp, (long) offset, SEEK_SET);
    if (fread(&tmp, sizeof(tmp), 1, fp) != 1) {
        return 0;
    }
    return tmp;
}

static inline uint16_t read_u16(FILE *fp, uint32_t offset)
{
    uint16_t tmp;
    fseek(fp, (long) offset, SEEK_SET);
    if (fread(&tmp, sizeof(tmp), 1, fp) != 1) {
        return 0;
    }
    return tmp;
}

static inline uint32_t get_u32_le_from_u8(const uint8_t *buf)
{
    return (((uint32_t) buf[0]) << 0)  |
           (((uint32_t) buf[1]) << 8)  |
           (((uint32_t) buf[2]) << 16) |
           (((uint32_t) buf[3]) << 24);
}


static inline void set_u32_le_to_u8(uint8_t *buf, uint32_t value)
{
    buf[0] = (uint8_t) (value & 0xFF);
    buf[1] = (uint8_t) ((value >> 8) & 0xFF);
    buf[2] = (uint8_t) ((value >> 16) & 0xFF);
    buf[3] = (uint8_t) ((value >> 24) & 0xFF);
}


static inline uint16_t get_u16_le_from_u8(const uint8_t *buf)
{
    return (uint16_t) ( ((uint16_t) buf[0]) | (((uint16_t) buf[1]) << 8) );
}


/**
 * @brief Checks some "magic" signatures of PE32 format and returns
 * an offset of PE header.
 * @return PE header offset or 0 in the case of failure.
 */
uint32_t get_pe386_offset(FILE *fp)
{
    uint32_t pe_offset;
    if (read_u16(fp, 0) != 0x5A4D) { // MZ
        return 0;
    }
    pe_offset = read_u32(fp, 0x3C);
    // Check all "magic" signatures
    if (read_u32(fp, pe_offset) != 0x4550 || // PE
        read_u16(fp, pe_offset + 0x04) != 0x14C || // i386
        read_u16(fp, pe_offset + 0x18) != 0x10B || // Magic
        read_u32(fp, pe_offset + 0x74) != 0x10) { // Number of directories
        return 0;
    }
    return pe_offset;
}

/**
 * @brief Read some information from PE32 executable headers. Mainly
 * sections addresses and sizes, export/import tables, relocations. Also
 * checks some "magic constants".
 * @param peinfo    Pointer to the output buffer.
 * @param fp        Opened file descriptior.
 * @param pe_offset Offset of the PE header.
 * @return 0/1 - failure/success.
 * @memberof PE32BasicInfo
 */
int PE32BasicInfo_init(PE32BasicInfo *peinfo, FILE *fp, uint32_t pe_offset)
{
    peinfo->nsections = read_u16(fp, pe_offset + 0x06);
    peinfo->entrypoint_rva = read_u32(fp, pe_offset + 0x28);
    peinfo->imagebase = read_u32(fp, pe_offset + 0x34);
    peinfo->export_dir = read_u32(fp, pe_offset + 0x78);
    peinfo->import_dir = read_u32(fp, pe_offset + 0x80);
    peinfo->reloc_dir  = read_u32(fp, pe_offset + 0xA0);
    peinfo->sections = calloc(peinfo->nsections, sizeof(PE32SectionInfo));
    if (peinfo->sections == NULL) {
        snprintf(errmsg, ERRMSG_MAXLEN, "PE32BasicInfo_init: not enough memory");
        peinfo->nsections = 0;
        return 0;
    }
    uint32_t offset = pe_offset + 0xF8;
    for (unsigned int i = 0; i < peinfo->nsections; i++) {
        PE32SectionInfo *sect = &peinfo->sections[i];
        fseek(fp, (long) offset, SEEK_SET);
        if (fread(sect->name, 8, 1, fp) != 1) {
            return 0;
        }
        sect->virtual_size = read_u32(fp, offset + 0x08);
        sect->virtual_addr = read_u32(fp, offset + 0x0C);
        sect->physical_size = read_u32(fp, offset + 0x10);
        sect->physical_addr = read_u32(fp, offset + 0x14);
        offset += 0x28; // To the next section
    }
    return 1;
}


void PE32BasicInfo_print(const PE32BasicInfo *peinfo)
{
    printf("nsections:  %u\n", peinfo->nsections);
    printf("ep rva:     %" PRIX32 "\n", peinfo->entrypoint_rva);
    printf("imagebase:  %" PRIX32 "\n", peinfo->imagebase);
    printf("export_dir: %" PRIX32 "\n", peinfo->export_dir);
    printf("import_dir: %" PRIX32 "\n", peinfo->import_dir);
    printf("reloc_dir:  %" PRIX32 "\n", peinfo->reloc_dir);
    printf("%12s  %8s %8s %8s %8s\n",
        "Name", "virtsize", "virtaddr", "physsize", "physaddr");
    for (unsigned int i = 0; i < peinfo->nsections; i++) {
        PE32SectionInfo *sect = &peinfo->sections[i];
        printf("%12s: %8.8" PRIX32" %8.8" PRIX32 " %8.8" PRIX32 " %8.8" PRIX32 "\n",
            sect->name,
            sect->virtual_size, sect->virtual_addr,
            sect->physical_size, sect->physical_addr);
    }
}

void PE32BasicInfo_destruct(PE32BasicInfo *peinfo)
{
    free(peinfo->sections);
}

/**
 * @brief Return the size of memory buffer required for the file loading.
 */
uint32_t PE32BasicInfo_get_membuf_size(PE32BasicInfo *info)
{
    PE32SectionInfo *lastsect = &info->sections[info->nsections - 1];
    uint32_t bufsize = lastsect->virtual_addr;
    if (lastsect->physical_size > lastsect->virtual_size) {
        bufsize += lastsect->physical_size;
    } else {
        bufsize += lastsect->virtual_size;
    }
    return bufsize;
}

////////////////////////////////////////////////
///// PE32MemoryImage class implementation /////
////////////////////////////////////////////////

/**
 * @brief Get unsigned 32-bit integer from the given RVA.
 * @param obj  Preloaded PE32 file image.
 * @param rva  Value RVA (Relative Virtual Address).
 * @return Unsigned 32-bit integer.
 * @memberof PE32MemoryImage
 */
static inline uint32_t PE32MemoryImage_get_u32(const PE32MemoryImage *obj, uint32_t rva)
{
    return get_u32_le_from_u8(obj->img + rva);
}


static inline uint16_t PE32MemoryImage_get_u16(const PE32MemoryImage *obj, uint32_t rva)
{
    return get_u16_le_from_u8(obj->img + rva);
}


static inline void PE32MemoryImage_put_u32(PE32MemoryImage *obj, uint32_t rva, uint32_t value)
{
    set_u32_le_to_u8(obj->img + rva, value);
}


/**
 * @brief Return the function address from the PE32 executable file export table.
 * @param obj       Preloaded PE32 file image.
 * @param func_name Function name as written in the export table (ASCIIZ string).
 * @return Function address (not RVA, can be converted to the pointer).
 * @memberof PE32MemoryImage
 */
void *PE32MemoryImage_get_func_addr(const PE32MemoryImage *obj, const char *func_name)
{
    for (unsigned int i = 0; i < obj->nexports; i++) {
        int ord = obj->exports_ords[i];
        if (!strcmp(func_name, obj->exports_names[i])) {
            return obj->exports_addrs[ord];
        }
    }
    return NULL;
}

/**
 * @brief Deallocates all internal buffers.
 * @memberof PE32MemoryImage
 */
void PE32MemoryImage_destruct(PE32MemoryImage *obj)
{
    execbuffer_free(obj->img);
}

/**
 * @brief Applies relocations in the preloaded PE32 executable file image.
 * @param img   Preloaded image with relocations.
 * @param info  Information about sections and directories.
 * @return 0/1 - failure/success.
 * @memberof PE32MemoryImage
 */
int PE32MemoryImage_apply_relocs(PE32MemoryImage *img, PE32BasicInfo *info)
{
    uint8_t *buf = img->img;
    const uint32_t imagebase_real = (uint32_t) ((size_t) buf);
    const uint32_t offset = imagebase_real - info->imagebase;
    uint32_t rva;
    for (uint32_t r = info->reloc_dir;
        (rva = PE32MemoryImage_get_u32(img, r)) != 0; ) {
        const uint32_t nbytes = PE32MemoryImage_get_u32(img, r + 4);
        const uint32_t nrelocs = (nbytes - 8) / 2;
        printf("Reloc. chunk: rva=%" PRIX32 ", nbytes=%" PRIX32 ", nrelocs=%" PRIX32 "\n",
            rva, nbytes, nrelocs);
        const uint32_t re = r + 8;
        for (uint32_t i = 0; i < nrelocs; i++) {
            const uint16_t re_val = PE32MemoryImage_get_u16(img, re + 2*i);
            if (re_val >> 12 == 3) {
                uint32_t reloc_rva = rva + (re_val & 0x0FFF);
                uint32_t reloc_val = PE32MemoryImage_get_u32(img, reloc_rva);
                printf("  Reloc: rva=%" PRIX32 ", before=%" PRIX32 ", ",
                    reloc_rva, reloc_val);
                reloc_val += offset;
                PE32MemoryImage_put_u32(img, reloc_rva, reloc_val);
                printf("after: %" PRIX32 "\n", reloc_val);
            }
        }
        r += nbytes;
    }
    return 1;
}

/**
 * @brief Fill export table in the PE32 preloaded image. Converts RVAs to real
 * addresses.
 * @memberof PE32MemoryImage
 */
int PE32MemoryImage_apply_exports(PE32MemoryImage *img, PE32BasicInfo *info)
{
    img->nexports = PE32MemoryImage_get_u32(img, info->export_dir + 24);
    uint32_t func_addrs_array_rva = PE32MemoryImage_get_u32(img, info->export_dir + 28);
    uint32_t func_names_array_rva = PE32MemoryImage_get_u32(img, info->export_dir + 32);
    uint32_t ord_array_rva = PE32MemoryImage_get_u32(img, info->export_dir + 36);
    uint32_t *func_names_rva = (uint32_t *) &img->img[func_names_array_rva];
    uint32_t *func_addrs_rva = (uint32_t *) &img->img[func_addrs_array_rva];
    img->exports_ords = (uint16_t *) &img->img[ord_array_rva];
    printf("nexports:  %u\n", img->nexports);
    printf("addrs rva: %" PRIX32 "\n", func_addrs_array_rva);
    printf("names rva: %" PRIX32 "\n", func_names_array_rva);
    printf("ord rva:   %" PRIX32 "\n", ord_array_rva);
    // Patch RVAs
    uint32_t imagebase_real = (uint32_t) ((size_t) img->img);
    for (unsigned int i = 0; i < img->nexports; i++) {
        func_names_rva[i] += imagebase_real;
        func_addrs_rva[i] += imagebase_real;
    }
    img->exports_names = (void *) func_names_rva;
    img->exports_addrs = (void *) func_addrs_rva;

    for (unsigned int i = 0; i < img->nexports; i++) {
        const int ord = img->exports_ords[i];
        const unsigned long long addr = (size_t) img->exports_addrs[ord];
        const unsigned long long base = (size_t) img->img;
        printf("  func=%s, addr=%llX, rva=%llX, ord=%d\n",
            img->exports_names[i], addr, addr - base, ord);
    }
    return 1;
}

/**
 * @brief Checks if the import table is present. Presence of the import table
 * is considered as an error (because it is not supported).
 * @return 0 - failure (imports are present), 1 - succes (imports are not
 * present).
 * @memberof PE32MemoryImage
 */
int PE32MemoryImage_apply_imports(PE32MemoryImage *img, PE32BasicInfo *info)
{
    if (info->import_dir == 0) {
        return 1;
    }
    const uint32_t lookup_rva = PE32MemoryImage_get_u32(img, info->import_dir);
    if (lookup_rva != 0) {
        snprintf(errmsg, ERRMSG_MAXLEN, "DLL imports are not supported");
        return 0;
    }
    return 1;
}

//////////////////////////////////////////////
///// PE32BasicInfo class implementation /////
//////////////////////////////////////////////

/**
 * @brief Load PE32 executable file to RAM using the preloaded PE32BasicInfo
 * structure.
 * @memberof PE32BasicInfo
 */
PE32MemoryImage *PE32BasicInfo_load(PE32BasicInfo *info, FILE *fp)
{
    PE32MemoryImage *img = calloc(sizeof(PE32MemoryImage), 1);
    if (img == NULL) {
        snprintf(errmsg, ERRMSG_MAXLEN, "PE32BasicInfo_load: not enough memory");
        return NULL;
    }
    img->imgsize = PE32BasicInfo_get_membuf_size(info);
    img->img = execbuffer_alloc(img->imgsize);
    if (img->img == NULL) {
        snprintf(errmsg, ERRMSG_MAXLEN, "PE32BasicInfo_load: not enough memory");
        return NULL;
        free(img);
        return NULL;
    }
    uint8_t *buf = img->img;
    for (unsigned int i = 0; i < info->nsections; i++) {
        PE32SectionInfo *sect = &info->sections[i];
        if (fseek(fp, (long) sect->physical_addr, SEEK_SET) ||
            fread(buf + sect->virtual_addr, sect->physical_size, 1, fp) != 1) {
            snprintf(errmsg, ERRMSG_MAXLEN, "Cannot read section %d\n", i + 1);
            free(img->img); free(img);
            return NULL;
        }
    }
    // Export table: get size and RVAs
    if (!PE32MemoryImage_apply_imports(img, info) ||
        !PE32MemoryImage_apply_exports(img, info) ||
        !PE32MemoryImage_apply_relocs(img, info)) {
        free(img->img); free(img);
        return NULL;
    }
    // Write metainformation to the image
    snprintf((char *) buf, 128,
        "Image base from PE: %llX\n"
        "Image base (real):  %llX\n",
        (unsigned long long) info->imagebase,
        (unsigned long long) ( (size_t) buf ) );
    return img;
}

int PE32MemoryImage_dump(const PE32MemoryImage *img, const char *filename)
{
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Cannot dump the file\n");
        return 0;
    }
    int is_ok = fwrite(img->img, img->imgsize, 1, fp) != 0;
    fclose(fp);
    return is_ok;
}

////////////////////////////////
///// The higher-level API /////
////////////////////////////////

/**
 * @brief Loads DLL (dynamic library) in PE32 format to memory without WinAPI.
 * @details The support of PE32 is very limited. This loader processes
 * relocations and export table but doesn't support an import table (fails if
 * it is present). It is made intentionally because it is designed for simple
 * plugins with pseudorandom number generators.
 * @param libname Library file name.
 * @param flag    Reserved.
 * @return Library handle or NULL in the case of failure.
 */
void *dlopen_pe32dos(const char *libname, int flag)
{
    if (sizeof(size_t) != sizeof(uint32_t)) {
        snprintf(errmsg, ERRMSG_MAXLEN,
            "This program can work only in 32-bit mode\n");
        return NULL;
    }
    FILE *fp = fopen(libname, "rb");
    if (fp == NULL) {
        snprintf(errmsg, ERRMSG_MAXLEN,
            "Cannot open the '%s' file\n", libname);
        return NULL;
    }
    uint32_t pe_offset = get_pe386_offset(fp);
    if (pe_offset == 0) {
        snprintf(errmsg, ERRMSG_MAXLEN, "The file '%s' is corrupted\n", libname);
        fclose(fp);
        return NULL;
    }
    PE32BasicInfo peinfo;
    PE32BasicInfo_init(&peinfo, fp, pe_offset);
    PE32BasicInfo_print(&peinfo);
    PE32MemoryImage *img = PE32BasicInfo_load(&peinfo, fp);
    fclose(fp);
    PE32BasicInfo_destruct(&peinfo);
    (void) flag;
    return img;
}

/**
 * @brief Returns a pointer to a function from a loaded dynamic library.
 * @param handle   Library handle.
 * @param symname  Function name.
 * @return Function pointer or NULL in the case of failure.
 */
void *dlsym_pe32dos(void *handle, const char *symname)
{
    void *func = PE32MemoryImage_get_func_addr(handle, symname);
    if (func == NULL) {
        snprintf(errmsg, ERRMSG_MAXLEN, "Function '%s' not found", symname);
    }
    return func;
}

/**
 * @brief Unloads a loaded dynamic library from memory.
 * @param handle  Library handle.
 */
void dlclose_pe32dos(void *handle)
{
    if (handle != NULL) {
        PE32MemoryImage_destruct(handle);
        free(handle);
    }
}

/**
 * @brief Returns a pointer to a last error message.
 */
const char *dlerror_pe32dos(void)
{
    return errmsg;
}
