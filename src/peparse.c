// https://coffi.readthedocs.io/en/latest/peering_inside_pe.pdf
// https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#export-directory-table
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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

typedef struct {
    char name[9]; // ASCIIZ-string
    uint32_t virtual_size; // memory
    uint32_t virtual_addr; // rva
    uint32_t physical_size; // file
    uint32_t physical_addr; // file
} PE32SectionInfo;


typedef struct {
    int nexports;
    void **addrs;
    char **names;
} MemoryImageExports;


typedef struct {
    uint32_t imagebase;
    uint32_t entrypoint_rva;
    uint32_t imagesize;
    uint32_t headersize;

    uint32_t export_dir;
    uint32_t import_dir;
    uint32_t reloc_dir;

    int nsections;
    PE32SectionInfo *sections;
} PE32BasicInfo;

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


uint32_t PE32BasicInfo_load(PE32BasicInfo *info, uint8_t *buf, FILE *fp)
{
    for (int i = 0; i < info->nsections; i++) {
        PE32SectionInfo *sect = &info->sections[i];
        fseek(fp, sect->physical_addr, SEEK_SET);
        fread(buf + sect->virtual_addr, sect->physical_size, 1, fp);
    }
    // Export table
    int nexports = *( (uint32_t *) (buf + info->export_dir + 24) );
    uint32_t func_addrs_array_rva = *( (uint32_t *) (buf + info->export_dir + 28) );
    uint32_t func_names_array_rva = *( (uint32_t *) (buf + info->export_dir + 32) );
    uint32_t *func_names_rva = (uint32_t *) &buf[func_names_array_rva];
    uint32_t *func_addrs_rva = (uint32_t *) &buf[func_addrs_array_rva];
    printf("nexports: %d\n", nexports);
    printf("addrs rva: %X\n", func_addrs_array_rva);
    printf("names rva: %X\n", func_names_array_rva);


    uint64_t mwcstate = 0x1;

    for (int i = 0; i < nexports; i++) {
        const char *func_name = &buf[func_names_rva[i]];
        printf("  %s %X\n", func_name, func_addrs_rva[i]);
        if (!strcmp(func_name, "get_bits")) {
            printf("HERE!\n");

            uint64_t (*get_bits)(void *) = (void *) ((size_t) func_addrs_rva[i] + (size_t) buf);
            printf("%X\n", get_bits(&mwcstate));
            printf("%X\n", get_bits(&mwcstate));
            printf("%X\n", get_bits(&mwcstate));
            printf("%X\n", get_bits(&mwcstate));
            printf("%X\n", get_bits(&mwcstate));
            
        }
    }

    // Relocations
    

    

/*
20 4 Address Table Entries The number of entries in the export address table.
24 4 Number of Name Pointers The number of entries in the name pointer table. This is also the number of entries in the ordinal table.
28 4 Export Address Table RVA The address of the export address table, relative to the image base.
32 4 Name Pointer RVA The address of the export name pointer table, relative to the image base. The table size is given by the Number of Name Pointers field.
*/
    // Relocations
    
}


int main()
{
    static const char *filename = "libmwc64x.dll";
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open the file\n");
        return 1;
    }

    uint32_t pe_offset = get_pe386_offset(fp); 

    if (pe_offset == 0) {
        fprintf(stderr, "Invalid PE file\n");
        return 1;
    }

    PE32BasicInfo peinfo;
    PE32BasicInfo_init(&peinfo, fp, pe_offset);
    PE32BasicInfo_print(&peinfo);
    size_t bufsize = PE32BasicInfo_get_membuf_size(&peinfo);
    printf("Buffer size: %X\n", (unsigned int) bufsize);
    uint8_t *buf = execbuffer_alloc(bufsize);

    PE32BasicInfo_load(&peinfo, buf, fp);
    fclose(fp);




    fp = fopen("dump.bin", "wb");
    if (fp == NULL) {
        fprintf(stderr, "Cannot dump the file\n");
        return 1;
    }
    fwrite(buf, bufsize, 1, fp);
    fclose(fp);


    execbuffer_free(buf);
    PE32BasicInfo_free(&peinfo);

    
    
    


    return 0;
}