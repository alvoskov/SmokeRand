/**
 * @file pe32loader.h
 * @brief PE32 (32-bit Portable Executables) files loader for 32-bit platforms.
 * Useful for running SmokeRand under 32-bit DOS extenders without DLL support.
 * @details It is a very simplified loader that supports only DLLs without
 * imports (but with relocations). That is enough for loading PRNG PE plugins
 * on x86 platforms without WinAPI support.
 *
 * 1. Matt Pietrek. Peering Inside the PE: A Tour of the Win32 Portable
 *    Executable File Format. March 1994.
 *    https://coffi.readthedocs.io/en/latest/peering_inside_pe.pdf
 * 2. https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
 * 3. https://ferreirasc.github.io/PE-Export-Address-Table/
 *
 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_PE32LOADER_H
#define __SMOKERAND_PE32LOADER_H
#include <stdio.h>
#include <stdint.h>


void *dlopen_pe32dos(const char *libname, int flag);
void *dlsym_pe32dos(void *handle, const char *symname);
void dlclose_pe32dos(void *handle);
const char *dlerror_pe32dos(void);




typedef struct {
    char name[9]; ///< ASCIIZ-string with section name.
    uint32_t virtual_size; ///< Size in RAM.
    uint32_t virtual_addr; ///< Section RVA.
    uint32_t physical_size; ///< Section size in the file.
    uint32_t physical_addr; ///< Section offset in the file.
} PE32SectionInfo;

typedef struct {
    uint8_t *img; ///< Loaded image raw data.
    size_t imgsize; ///< Loaded image size
    void *entry_point;
    int nexports;
    void **exports_addrs;
    char **exports_names;
    uint16_t *exports_ords;
} PE32MemoryImage;

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


void *PE32MemoryImage_get_func_addr(const PE32MemoryImage *img, const char *func_name);
void PE32MemoryImage_free(PE32MemoryImage *obj);


int get_pe386_offset(FILE *fp);
int PE32BasicInfo_init(PE32BasicInfo *peinfo, FILE *fp, uint32_t pe_offset);
void PE32BasicInfo_free(PE32BasicInfo *peinfo);
void PE32BasicInfo_print(const PE32BasicInfo *peinfo);
uint32_t PE32BasicInfo_get_membuf_size(PE32BasicInfo *info);
PE32MemoryImage *PE32BasicInfo_load(PE32BasicInfo *info, FILE *fp);

#endif
