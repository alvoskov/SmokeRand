#include "pe32loader.h"
#include <stdlib.h>
#include <string.h>


#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64) || defined(__MINGW32__) || defined(__MINGW64__)
#include <windows.h>
void *execbuffer_alloc(size_t len)
{
    void *buf = VirtualAlloc(NULL, len, MEM_COMMIT, PAGE_READWRITE);
    DWORD dummy;
    VirtualProtect(buf, len, PAGE_EXECUTE_READWRITE, &dummy);
    return buf;
}

void execbuffer_free(void *buf)
{
    VirtualFree(buf, 0, MEM_RELEASE);
}
#else
void *execbuffer_alloc(size_t len)
{
    return malloc(len);
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
    fread(&tmp, sizeof(tmp), 1, fp);
    return tmp;
}

static inline uint16_t read_u16(FILE *fp, unsigned int offset)
{
    uint16_t tmp;
    fseek(fp, offset, SEEK_SET);
    fread(&tmp, sizeof(tmp), 1, fp);
    return tmp;
}

void *PE32MemoryImage_get_func_addr(const PE32MemoryImage *img, const char *func_name)
{
    for (int i = 0; i < img->nexports; i++) {
        if (!strcmp(func_name, img->exports_names[i])) {
            return img->exports_addrs[i];
        }
    }
    return NULL;
}

void PE32MemoryImage_free(PE32MemoryImage *obj)
{
    execbuffer_free(obj->img);
}



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


int PE32BasicInfo_init(PE32BasicInfo *peinfo, FILE *fp, uint32_t pe_offset)
{
    peinfo->nsections = read_u16(fp, pe_offset + 0x06);
    peinfo->entrypoint_rva = read_u32(fp, pe_offset + 0x28);
    peinfo->imagebase = read_u32(fp, pe_offset + 0x34);
    peinfo->export_dir = read_u32(fp, pe_offset + 0x78);
    peinfo->import_dir = read_u32(fp, pe_offset + 0x80);
    peinfo->reloc_dir  = read_u32(fp, pe_offset + 0xA0);
    peinfo->sections = calloc(peinfo->nsections, sizeof(PE32SectionInfo));
    unsigned int offset = pe_offset + 0xF8;
    for (int i = 0; i < peinfo->nsections; i++) {
        PE32SectionInfo *sect = &peinfo->sections[i];
        fseek(fp, offset, SEEK_SET);
        fread(sect->name, 8, 1, fp);
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

void PE32BasicInfo_free(PE32BasicInfo *peinfo)
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



PE32MemoryImage PE32BasicInfo_load(PE32BasicInfo *info, FILE *fp)
{
    PE32MemoryImage img;
    img.imgsize = PE32BasicInfo_get_membuf_size(info);
    img.img = execbuffer_alloc(img.imgsize);
    uint8_t *buf = img.img;
    for (int i = 0; i < info->nsections; i++) {
        PE32SectionInfo *sect = &info->sections[i];
        fseek(fp, sect->physical_addr, SEEK_SET);
        fread(buf + sect->virtual_addr, sect->physical_size, 1, fp);
    }
    // Export table: get size and RVAs
    img.nexports = *( (uint32_t *) (buf + info->export_dir + 24) );
    uint32_t func_addrs_array_rva = *( (uint32_t *) (buf + info->export_dir + 28) );
    uint32_t func_names_array_rva = *( (uint32_t *) (buf + info->export_dir + 32) );
    uint32_t *func_names_rva = (uint32_t *) &buf[func_names_array_rva];
    uint32_t *func_addrs_rva = (uint32_t *) &buf[func_addrs_array_rva];
    printf("nexports: %d\n", img.nexports);
    printf("addrs rva: %X\n", func_addrs_array_rva);
    printf("names rva: %X\n", func_names_array_rva);
    // Patch RVAs
    size_t imagebase_real = (size_t) buf;
    for (int i = 0; i < img.nexports; i++) {
        func_names_rva[i] += imagebase_real;
        func_addrs_rva[i] += imagebase_real;
    }
    img.exports_names = (void *) func_names_rva;
    img.exports_addrs = (void *) func_addrs_rva;

    // Relocations
    uint32_t *relocs = (uint32_t *) (buf + info->reloc_dir);
    size_t offset = imagebase_real - info->imagebase; 
    for (uint32_t *r = relocs; r[0] != 0; ) {
        uint32_t rva = r[0];
        uint32_t nbytes = r[1];
        uint32_t nrelocs = (r[1] - 8) / 2;
        printf("RELOC: %X %X %X\n", rva, nbytes, nrelocs);
        uint16_t *re = (uint16_t *) (r + 2);
        for (uint32_t i = 0; i < nrelocs; i++) {
            if (re[i] >> 12 == 3) {
                uint32_t reloc_rva = rva + (re[i] & 0x0FFF);
                printf("RELOC_RVA: %X\n", reloc_rva);
                uint32_t *reloc_place = (uint32_t *) (buf + reloc_rva);
                printf("BEFORE:%X\n", *reloc_place);
                *reloc_place += offset;
                printf("AFTER:%X\n", *reloc_place);
            }
        }

        r += (nbytes) / sizeof(uint32_t);
    }

    // Write metainformation to the image
    snprintf((char *) buf, 128,
        "Image base from PE: %lX\n"
        "Image base (real):  %lX\n",
        (unsigned long) info->imagebase, (unsigned long) buf);

    return img;
}

