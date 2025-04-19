#include "smokerand/pe32loader.h"
#include <stdio.h>
#include <stdlib.h>
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

static inline uint32_t read_u32(FILE *fp, unsigned int offset)
{
    uint32_t tmp;
    fseek(fp, offset, SEEK_SET);
    if (fread(&tmp, sizeof(tmp), 1, fp) != 1) {
        return 0;
    }
    return tmp;
}

static inline uint16_t read_u16(FILE *fp, unsigned int offset)
{
    uint16_t tmp;
    fseek(fp, offset, SEEK_SET);
    if (fread(&tmp, sizeof(tmp), 1, fp) != 1) {
        return 0;
    }
    return tmp;
}

/**
 * @brief Checks some "magic" signatures of PE32 format and returns
 * an offset of PE header.
 * @return PE header offset or 0 in the case of failure.
 */
int get_pe386_offset(FILE *fp)
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
 * @relates PE32BasicInfo
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
    unsigned int offset = pe_offset + 0xF8;
    for (int i = 0; i < peinfo->nsections; i++) {
        PE32SectionInfo *sect = &peinfo->sections[i];
        fseek(fp, offset, SEEK_SET);
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
    printf("nsections:  %d\n", (int) peinfo->nsections);
    printf("ep rva:     %X\n", (unsigned int) peinfo->entrypoint_rva);
    printf("imagebase:  %X\n", (unsigned int) peinfo->imagebase);
    printf("export_dir: %X\n", (unsigned int) peinfo->export_dir);
    printf("import_dir: %X\n", (unsigned int) peinfo->import_dir);
    printf("reloc_dir:  %X\n", (unsigned int) peinfo->reloc_dir);

    for (int i = 0; i < peinfo->nsections; i++) {
        PE32SectionInfo *sect = &peinfo->sections[i];
        printf("%s: %X %X %X %X\n", sect->name,
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
 * @relates PE32MemoryImage
 */
static inline uint32_t PE32MemoryImage_get_u32(const PE32MemoryImage *obj, uint32_t rva)
{
    return *( (uint32_t *) (obj->img + rva) );
}

/**
 * @brief Return the function address from the PE32 executable file export table.
 * @param obj       Preloaded PE32 file image.
 * @param func_name Function name as written in the export table (ASCIIZ string).
 * @return Function address (not RVA, can be converted to the pointer).
 * @relates PE32MemoryImage
 */
void *PE32MemoryImage_get_func_addr(const PE32MemoryImage *img, const char *func_name)
{
    for (int i = 0; i < img->nexports; i++) {
        int ord = img->exports_ords[i];
        if (!strcmp(func_name, img->exports_names[i])) {
            return img->exports_addrs[ord];
        }
    }
    return NULL;
}

/**
 * @brief Deallocates all internal buffers.
 * @relates PE32MemoryImage
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
 * @relates PE32MemoryImage
 */
int PE32MemoryImage_apply_relocs(PE32MemoryImage *img, PE32BasicInfo *info)
{
    uint8_t *buf = img->img;
    uint32_t imagebase_real = (uint32_t) ((size_t) buf);
    uint32_t *relocs = (uint32_t *) (buf + info->reloc_dir);
    uint32_t offset = imagebase_real - info->imagebase;
    for (uint32_t *r = relocs; r[0] != 0; ) {
        uint32_t rva = r[0];
        uint32_t nbytes = r[1];
        uint32_t nrelocs = (r[1] - 8) / 2;
        printf("Reloc. chunk: rva=%X, nbytes=%X, nrelocs=%X\n",
            rva, nbytes, nrelocs);
        uint16_t *re = (uint16_t *) (r + 2);
        for (uint32_t i = 0; i < nrelocs; i++) {
            if (re[i] >> 12 == 3) {
                uint32_t reloc_rva = rva + (re[i] & 0x0FFF);
                uint32_t *reloc_place = (uint32_t *) (buf + reloc_rva);
                printf("  Reloc: rva=%X, before=%X, ", reloc_rva, *reloc_place);
                *reloc_place += offset;
                printf("after: %X\n", *reloc_place);
            }
        }
        r += (nbytes) / sizeof(uint32_t);
    }
    return 1;
}

/**
 * @brief Fill export table in the PE32 preloaded image. Converts RVAs to real
 * addresses.
 * @relates PE32MemoryImage
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
    printf("nexports: %d\n", img->nexports);
    printf("addrs rva: %X\n", func_addrs_array_rva);
    printf("names rva: %X\n", func_names_array_rva);
    printf("ord rva: %X\n", ord_array_rva);
    // Patch RVAs
    uint32_t imagebase_real = (uint32_t) ((size_t) img->img);
    for (int i = 0; i < img->nexports; i++) {
        func_names_rva[i] += imagebase_real;
        func_addrs_rva[i] += imagebase_real;
    }
    img->exports_names = (void *) func_names_rva;
    img->exports_addrs = (void *) func_addrs_rva;

    for (int i = 0; i < img->nexports; i++) {
        int ord = img->exports_ords[i];
        printf("  func=%s, addr=%llX, rva=%llX, ord=%d\n",
            img->exports_names[i],
            (unsigned long long) ( (size_t) img->exports_addrs[ord] ),
            (unsigned long long) ( (size_t) img->exports_addrs[ord] ) -
                (unsigned long long) ( (size_t) img->img ),
            ord);
    }
    return 1;
}

/**
 * @brief Checks if the import table is present. Presence of the import table
 * is considered as an error (because it is not supported).
 * @return 0 - failure (imports are present), 1 - succes (imports are not
 * present).
 * @relates PE32MemoryImage
 */
int PE32MemoryImage_apply_imports(PE32MemoryImage *img, PE32BasicInfo *info)
{
    if (info->import_dir == 0) {
        return 1;
    }
    uint32_t lookup_rva = PE32MemoryImage_get_u32(img, info->import_dir);
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
 * @relates PE32BasicInfo
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
    for (int i = 0; i < info->nsections; i++) {
        PE32SectionInfo *sect = &info->sections[i];
        if (fseek(fp, sect->physical_addr, SEEK_SET) ||
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
