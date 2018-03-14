/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: elf_type.h
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2010 年 04 月 17 日
**
** 描        述: 与体系结构相关的 elf 文件
*********************************************************************************************************/

#ifndef __ELF_TYPE_H
#define __ELF_TYPE_H

/*********************************************************************************************************
  体系结构相关
*********************************************************************************************************/

#include "../SylixOS/config/cpu/cpu_cfg.h"

#if defined(LW_CFG_CPU_ARCH_ARM)
#include "arch/arm/elf/armElf.h"

#elif defined(LW_CFG_CPU_ARCH_X86)
#include "arch/x86/elf/x86Elf.h"

#elif defined(LW_CFG_CPU_ARCH_MIPS)
#include "arch/mips/elf/mipsElf.h"

#elif defined(LW_CFG_CPU_ARCH_PPC)
#include "arch/ppc/elf/ppcElf.h"

#elif defined(LW_CFG_CPU_ARCH_C6X)
#include "arch/c6x/elf/c6xElf.h"

#elif defined(LW_CFG_CPU_ARCH_SPARC)
#include "arch/sparc/elf/sparcElf.h"
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */

/*********************************************************************************************************
  Attention! Platform depended definitions. 
*********************************************************************************************************/
/*********************************************************************************************************
  32-bit ELF base types. 
*********************************************************************************************************/
typedef addr_t   Elf32_Addr;
typedef UINT16   Elf32_Half;
typedef UINT32   Elf32_Off;
typedef SINT32   Elf32_Sword;
typedef UINT32   Elf32_Word;

/*********************************************************************************************************
  64-bit ELF base types. 
*********************************************************************************************************/
typedef addr_t   Elf64_Addr;
typedef UINT16   Elf64_Half;
typedef SINT16   Elf64_SHalf;
typedef UINT64   Elf64_Off;
typedef SINT32   Elf64_Sword;
typedef UINT32   Elf64_Word;
typedef UINT64   Elf64_Xword;
typedef SINT64   Elf64_Sxword;

/*********************************************************************************************************
  These constants are for the segment types stored in the image headers  
*********************************************************************************************************/
#define PT_NULL         0
#define PT_LOAD         1
#define PT_DYNAMIC      2
#define PT_INTERP       3
#define PT_NOTE         4
#define PT_SHLIB        5
#define PT_PHDR         6
#define PT_TLS          7                                               /* Thread local storage segment */
#define PT_LOOS         0x60000000                                      /* OS-specific                  */
#define PT_HIOS         0x6fffffff                                      /* OS-specific                  */
#define PT_LOPROC       0x70000000
#define PT_HIPROC       0x7fffffff
#define PT_GNU_EH_FRAME 0x6474e550

#define PT_GNU_STACK    (PT_LOOS + 0x474e551)

/*********************************************************************************************************
  These constants define the different elf file types 
*********************************************************************************************************/
#define ET_NONE         0
#define ET_REL          1
#define ET_EXEC         2
#define ET_DYN          3
#define ET_CORE         4
#define ET_LOPROC       0xff00
#define ET_HIPROC       0xffff

/*********************************************************************************************************
  This is the info that is needed to parse the dynamic section of the file  
*********************************************************************************************************/
#define DT_NULL                 0
#define DT_NEEDED               1
#define DT_PLTRELSZ             2
#define DT_PLTGOT               3
#define DT_HASH                 4
#define DT_STRTAB               5
#define DT_SYMTAB               6
#define DT_RELA                 7
#define DT_RELASZ               8
#define DT_RELAENT              9
#define DT_STRSZ                10
#define DT_SYMENT               11
#define DT_INIT                 12
#define DT_FINI                 13
#define DT_SONAME               14
#define DT_RPATH                15
#define DT_SYMBOLIC             16
#define DT_REL                  17
#define DT_RELSZ                18
#define DT_RELENT               19
#define DT_PLTREL               20
#define DT_DEBUG                21
#define DT_TEXTREL              22
#define DT_JMPREL               23
#define DT_BIND_NOW             24
#define DT_INIT_ARRAY           25
#define DT_FINI_ARRAY           26
#define DT_INIT_ARRAYSZ         27
#define DT_FINI_ARRAYSZ         28
#define DT_RUNPATH              29
#define DT_FLAGS                30
#define DT_ENCODING             32
#define DT_PREINIT_ARRAY        32
#define DT_PREINIT_ARRAYSZ      33
#define OLD_DT_LOOS             0x60000000
#define DT_LOOS                 0x6000000d
#define DT_HIOS                 0x6ffff000
#define DT_VALRNGLO             0x6ffffd00
#define DT_VALRNGHI             0x6ffffdff
#define DT_ADDRRNGLO            0x6ffffe00
#define DT_ADDRRNGHI            0x6ffffeff
#define DT_VERSYM               0x6ffffff0
#define DT_RELACOUNT            0x6ffffff9
#define DT_RELCOUNT             0x6ffffffa
#define DT_FLAGS_1              0x6ffffffb
#define DT_VERDEF               0x6ffffffc
#define DT_VERDEFNUM            0x6ffffffd
#define DT_VERNEED              0x6ffffffe
#define DT_VERNEEDNUM           0x6fffffff
#define OLD_DT_HIOS             0x6fffffff
#define DT_LOPROC               0x70000000
#define DT_HIPROC               0x7fffffff

/*********************************************************************************************************
  MIPS 
*********************************************************************************************************/
#ifdef  LW_CFG_CPU_ARCH_MIPS
#define DT_MIPS_GOTSYM          0x70000013                              /* First GOT entry in DYNSYM    */
#define DT_MIPS_LOCAL_GOTNO     0x7000000a                              /* Number of local GOT entries  */
#define DT_MIPS_SYMTABNO        0x70000011                              /* Number of DYNSYM entries     */
#define DT_MIPS_RLD_MAP         0x70000016                              /* Addr of run time loader map. */

/*********************************************************************************************************
  The address of .got.plt in an executable using the new non-PIC ABI. 
*********************************************************************************************************/
#define DT_MIPS_PLTGOT          0x70000032
#endif                                                                  /* LW_CFG_CPU_ARCH_MIPS         */

/*********************************************************************************************************
  C6X
*********************************************************************************************************/
#if defined(LW_CFG_CPU_ARCH_C6X)
#define DT_C6000_DSBT_BASE      (DT_LOPROC + 0)
#define DT_C6000_DSBT_SIZE      (DT_LOPROC + 1)
#define DT_C6000_PREEMPTMAP     (DT_LOPROC + 2)
#define DT_C6000_DSBT_INDEX     (DT_LOPROC + 3)
#define DT_C6000_NUM            4
#endif                                                                  /* LW_CFG_CPU_ARCH_C6X          */

/*********************************************************************************************************
  e_machine 
*********************************************************************************************************/
#define EM_NONE         0                                               /* e_machine                    */
#define EM_M32          1                                               /* AT&T WE 32100                */
#define EM_SPARC        2                                               /* Sun SPARC                    */
#define EM_386          3                                               /* Intel 80386                  */
#define EM_68K          4                                               /* Motorola 68000               */
#define EM_88K          5                                               /* Motorola 88000               */
#define EM_486          6                                               /* Intel 80486                  */
#define EM_860          7                                               /* Intel i860                   */
#define EM_MIPS         8                                               /* MIPS family                  */
#define EM_PARISC       15                                              /* HPPA                         */
#define EM_SPARC32PLUS  18                                              /* Sun's "v8plus"               */
#define EM_PPC          20                                              /* PowerPC family               */
#define EM_PPC64        21                                              /* PowerPC64                    */
#define EM_S390         22                                              /* IBM S/390                    */
#define EM_SPU          23                                              /* Cell BE SPU                  */
#define EM_ARM          40                                              /* ARM/Thumb family             */
#define EM_SH           42                                              /* SuperH                       */
#define EM_SPARCV9      43                                              /* SPARC v9 64-bit              */
#define EM_H8_300       46                                              /* Renesas H8/300,300H,H8S      */
#define EM_IA_64        50                                              /* HP/Intel IA-64               */
#define EM_X86_64       62                                              /* AMD x86-64                   */
#define EM_CRIS         76                                              /* Axis 32-bit processor        */
#define EM_V850         87                                              /* NEC v850                     */
#define EM_M32R         88                                              /* Renesas M32R                 */
#define EM_MN10300      89                                              /* Panasonic/MEI MN10300, AM33  */
#define EM_BLACKFIN     106                                             /* ADI Blackfin Processor       */
#define EM_ALTERA_NIOS2 113                                             /* ALTERA NIOS2                 */
#define EM_TI_C6000     140                                             /* TI DSP C6x                   */
#define EM_AARCH64      183                                             /* ARM 64 bit                   */
#define EM_FRV          0x5441                                          /* Fujitsu FR-V                 */
#define EM_AVR32        0x18ad                                          /* Atmel AVR32                  */
#define EM_ALPHA        0x9026                                          /* Alpha                        */

/*********************************************************************************************************
  This info is needed when parsing the symbol table 
*********************************************************************************************************/
#define STB_LOCAL       0
#define STB_GLOBAL      1
#define STB_WEAK        2

#define STT_NOTYPE      0
#define STT_OBJECT      1
#define STT_FUNC        2
#define STT_SECTION     3
#define STT_FILE        4
#define STT_COMMON      5
#define STT_TLS         6

#define ELF_ST_BIND(x)      ((x) >> 4)
#define ELF_ST_TYPE(x)      (((unsigned int) x) & 0xf)
#define ELF32_ST_BIND(x)    ELF_ST_BIND(x)
#define ELF32_ST_TYPE(x)    ELF_ST_TYPE(x)
#define ELF64_ST_BIND(x)    ELF_ST_BIND(x)
#define ELF64_ST_TYPE(x)    ELF_ST_TYPE(x)

/*********************************************************************************************************
  dynamic segment
*********************************************************************************************************/
typedef struct dynamic {
    Elf32_Sword d_tag;
    union{
        Elf32_Word      d_val;
        Elf32_Addr      d_ptr;
    } d_un;
} Elf32_Dyn;

typedef struct {
    Elf64_Sxword        d_tag;                                          /* entry tag value              */
    union {
        Elf64_Xword     d_val;
        Elf64_Addr      d_ptr;
    } d_un;
} Elf64_Dyn;

/*********************************************************************************************************
  The following are used with relocations
*********************************************************************************************************/
#define ELF32_R_SYM(x)          ((x) >> 8)
#define ELF32_R_TYPE(x)         ((x) & 0xff)

#define ELF64_R_SYM(i)          ((i) >> 32)
#define ELF64_R_TYPE(i)         ((i) & 0xffffffff)

/*********************************************************************************************************
  sym st_other Symbol Visibility (add by Han.Hui)
*********************************************************************************************************/
#define STV_DEFAULT             0
#define STV_INTERNAL            1
#define STV_HIDDEN              2
#define STV_PROTECTED           3

typedef struct elf32_rel {
    Elf32_Addr      r_offset;
    Elf32_Word      r_info;
} Elf32_Rel;

typedef struct elf64_rel {
    Elf64_Addr      r_offset;                               /* Location at which to apply the action    */
    Elf64_Xword     r_info;                                 /* index and type of relocation             */
} Elf64_Rel;

typedef struct elf32_rela {
    Elf32_Addr      r_offset;
    Elf32_Word      r_info;
    Elf32_Sword     r_addend;
} Elf32_Rela;

typedef struct elf64_rela {
    Elf64_Addr      r_offset;                               /* Location at which to apply the action    */
    Elf64_Xword     r_info;                                 /* index and type of relocation             */
    Elf64_Sxword    r_addend;                               /* Constant addend used to compute value    */
} Elf64_Rela;

typedef struct elf32_sym {
    Elf32_Word      st_name;
    Elf32_Addr      st_value;
    Elf32_Word      st_size;
    unsigned char   st_info;
    unsigned char   st_other;
    Elf32_Half      st_shndx;
} Elf32_Sym;

typedef struct elf64_sym {
    Elf64_Word      st_name;                                /* Symbol name, index in string tbl         */
    unsigned char   st_info;                                /* Type and binding attributes              */
    unsigned char   st_other;                               /* No defined meaning, 0                    */
    Elf64_Half      st_shndx;                               /* Associated section index                 */
    Elf64_Addr      st_value;                               /* Value of the symbol                      */
    Elf64_Xword     st_size;                                /* Associated symbol size                   */
} Elf64_Sym;

/*********************************************************************************************************
  elf header
*********************************************************************************************************/
#define EI_NIDENT   16

typedef struct elf32_hdr {
    unsigned char   e_ident[EI_NIDENT];
    Elf32_Half      e_type;
    Elf32_Half      e_machine;
    Elf32_Word      e_version;
    Elf32_Addr      e_entry;                                /* Entry point                              */
    Elf32_Off       e_phoff;
    Elf32_Off       e_shoff;
    Elf32_Word      e_flags;
    Elf32_Half      e_ehsize;
    Elf32_Half      e_phentsize;
    Elf32_Half      e_phnum;
    Elf32_Half      e_shentsize;
    Elf32_Half      e_shnum;
    Elf32_Half      e_shstrndx;
} Elf32_Ehdr;

typedef struct elf64_hdr {
    unsigned char   e_ident[EI_NIDENT];                     /* ELF "magic number"                       */
    Elf64_Half      e_type;
    Elf64_Half      e_machine;
    Elf64_Word      e_version;
    Elf64_Addr      e_entry;                                /* Entry point virtual address              */
    Elf64_Off       e_phoff;                                /* Program header table file offset         */
    Elf64_Off       e_shoff;                                /* Section header table file offset         */
    Elf64_Word      e_flags;
    Elf64_Half      e_ehsize;
    Elf64_Half      e_phentsize;
    Elf64_Half      e_phnum;
    Elf64_Half      e_shentsize;
    Elf64_Half      e_shnum;
    Elf64_Half      e_shstrndx;
} Elf64_Ehdr;

/*********************************************************************************************************
  These constants define the permissions on sections in the program
  header, p_flags. 
*********************************************************************************************************/
#define PF_R        0x4
#define PF_W        0x2
#define PF_X        0x1

typedef struct elf32_phdr {
    Elf32_Word      p_type;
    Elf32_Off       p_offset;
    Elf32_Addr      p_vaddr;
    Elf32_Addr      p_paddr;
    Elf32_Word      p_filesz;
    Elf32_Word      p_memsz;
    Elf32_Word      p_flags;
    Elf32_Word      p_align;
} Elf32_Phdr;

typedef struct elf64_phdr {
    Elf64_Word      p_type;
    Elf64_Word      p_flags;
    Elf64_Off       p_offset;                               /* Segment file offset                      */
    Elf64_Addr      p_vaddr;                                /* Segment virtual address                  */
    Elf64_Addr      p_paddr;                                /* Segment physical address                 */
    Elf64_Xword     p_filesz;                               /* Segment size in file                     */
    Elf64_Xword     p_memsz;                                /* Segment size in memory                   */
    Elf64_Xword     p_align;                                /* Segment alignment, file & memory         */
} Elf64_Phdr;

/*********************************************************************************************************
  sh_type
*********************************************************************************************************/
#define SHT_NULL        0
#define SHT_PROGBITS    1
#define SHT_SYMTAB      2
#define SHT_STRTAB      3
#define SHT_RELA        4
#define SHT_HASH        5
#define SHT_DYNAMIC     6
#define SHT_NOTE        7
#define SHT_NOBITS      8
#define SHT_REL         9
#define SHT_SHLIB       10
#define SHT_DYNSYM      11
#define SHT_NUM         12
#define SHT_LOPROC      0x70000000
#define SHT_HIPROC      0x7fffffff
#define SHT_LOUSER      0x80000000
#define SHT_HIUSER      0xffffffff

/*********************************************************************************************************
  sh_flags
*********************************************************************************************************/
#define SHF_WRITE       0x1
#define SHF_ALLOC       0x2
#define SHF_EXECINSTR   0x4
#define SHF_MASKPROC    0xf0000000

/*********************************************************************************************************
  special section indexes
*********************************************************************************************************/
#define SHN_UNDEF       0
#define SHN_LORESERVE   0xff00
#define SHN_LOPROC      0xff00
#define SHN_HIPROC      0xff1f
#define SHN_ABS         0xfff1
#define SHN_COMMON      0xfff2
#define SHN_HIRESERVE   0xffff

typedef struct {
    Elf32_Word      sh_name;
    Elf32_Word      sh_type;
    Elf32_Word      sh_flags;
    Elf32_Addr      sh_addr;
    Elf32_Off       sh_offset;
    Elf32_Word      sh_size;
    Elf32_Word      sh_link;
    Elf32_Word      sh_info;
    Elf32_Word      sh_addralign;
    Elf32_Word      sh_entsize;
} Elf32_Shdr;

typedef struct elf64_shdr {
    Elf64_Word      sh_name;                                /* Section name, index in string tbl        */
    Elf64_Word      sh_type;                                /* Type of section                          */
    Elf64_Xword     sh_flags;                               /* Miscellaneous section attributes         */
    Elf64_Addr      sh_addr;                                /* Section virtual addr at execution        */
    Elf64_Off       sh_offset;                              /* Section file offset                      */
    Elf64_Xword     sh_size;                                /* Size of section in bytes                 */
    Elf64_Word      sh_link;                                /* Index of another section                 */
    Elf64_Word      sh_info;                                /* Additional section information           */
    Elf64_Xword     sh_addralign;                           /* Section alignment                        */
    Elf64_Xword     sh_entsize;                             /* Entry size if section holds table        */
} Elf64_Shdr;

#define EI_MAG0         0                                   /* e_ident[] indexes                        */
#define EI_MAG1         1
#define EI_MAG2         2
#define EI_MAG3         3
#define EI_CLASS        4
#define EI_DATA         5
#define EI_VERSION      6
#define EI_OSABI        7
#define EI_PAD          8

#define ELFMAG0         0x7f                                /* EI_MAG                                   */
#define ELFMAG1         'E'
#define ELFMAG2         'L'
#define ELFMAG3         'F'
#define ELFMAG          "\177ELF"
#define SELFMAG         4

#define ELFCLASSNONE    0                                   /* EI_CLASS                                 */
#define ELFCLASS32      1
#define ELFCLASS64      2
#define ELFCLASSNUM     3

#define ELFDATANONE     0                                   /* e_ident[EI_DATA]                         */
#define ELFDATA2LSB     1
#define ELFDATA2MSB     2

#define EV_NONE         0                                   /* e_version, EI_VERSION                    */
#define EV_CURRENT      1
#define EV_NUM          2

#define ELFOSABI_NONE   0
#define ELFOSABI_LINUX  3

#ifndef ELF_OSABI
#define ELF_OSABI       ELFOSABI_NONE
#endif

/*********************************************************************************************************
  Notes used in ET_CORE
*********************************************************************************************************/
#define NT_PRSTATUS     1
#define NT_PRFPREG      2
#define NT_PRPSINFO     3
#define NT_TASKSTRUCT   4
#define NT_AUXV         6
#define NT_PRXFPREG     0x46e62b7f                          /* copied from gdb5.1/include/elf/common.h  */
#define NT_PPC_VMX      0x100                               /* PowerPC Altivec/VMX registers            */
#define NT_PPC_SPE      0x101                               /* PowerPC SPE/EVR registers                */
#define NT_PPC_VSX      0x102                               /* PowerPC VSX registers                    */
#define NT_386_TLS      0x200                               /* i386 TLS slots (struct user_desc)        */

/*********************************************************************************************************
  Note header in a PT_NOTE section
*********************************************************************************************************/
typedef struct elf32_note {
    Elf32_Word      n_namesz;                               /* Name size                                */
    Elf32_Word      n_descsz;                               /* Content size                             */
    Elf32_Word      n_type;                                 /* Content type                             */
} Elf32_Nhdr;

/*********************************************************************************************************
  Note header in a PT_NOTE section
*********************************************************************************************************/
typedef struct elf64_note {
    Elf64_Word      n_namesz;                               /* Name size                                */
    Elf64_Word      n_descsz;                               /* Content size                             */
    Elf64_Word      n_type;                                 /* Content type                             */
} Elf64_Nhdr;

typedef struct elf32_hash {
    Elf32_Word      nbucket;
    Elf32_Word      nchain;
}Elf32_Hash;

typedef struct elf64_hash {
    Elf64_Word      nbucket;
    Elf64_Word      nchain;
}Elf64_Hash;

typedef struct elf32_auxv_t {
    Elf32_Word      a_type;
    union {
        Elf32_Word  a_val;
    } a_un;
} Elf32_auxv_t;

typedef struct elf64_auxv_t {
    Elf64_Word      a_type;
    union {
        Elf64_Word  a_val;
    } a_un;
} Elf64_auxv_t;

/*********************************************************************************************************
  32bit elf
*********************************************************************************************************/
#if ELF_CLASS == ELFCLASS32

#define ELF_R_SYM       ELF32_R_SYM
#define ELF_R_TYPE      ELF32_R_TYPE

typedef Elf32_Half      Elf_Half;
typedef Elf32_Off       Elf_Off;
typedef Elf32_Sword     Elf_Sword;
typedef Elf32_Word      Elf_Word;
typedef Elf32_Addr      Elf_Addr;
typedef Elf_Addr        Elf_Val;

typedef Elf32_Dyn       Elf_Dyn;
typedef Elf32_Rel       Elf_Rel;
typedef Elf32_Rela      Elf_Rela;
typedef Elf32_Sym       Elf_Sym;
typedef Elf32_Ehdr      Elf_Ehdr;
typedef Elf32_Phdr      Elf_Phdr;
typedef Elf32_Shdr      Elf_Shdr;
typedef Elf32_Hash      Elf_Hash;

typedef Elf32_auxv_t    Elf_auxv_t;

/*********************************************************************************************************
  64bit elf
*********************************************************************************************************/
#else                                                       /* ELF_CLASS == ELFCLASS32                  */

#define ELF_R_SYM       ELF64_R_SYM
#define ELF_R_TYPE      ELF64_R_TYPE

typedef Elf64_Half      Elf_Half;
typedef Elf64_Off       Elf_Off;
typedef Elf64_Sword     Elf_Sword;
typedef Elf64_Word      Elf_Word;
typedef Elf64_Addr      Elf_Addr;
typedef Elf_Addr        Elf_Val;

typedef Elf64_Dyn       Elf_Dyn;
typedef Elf64_Rel       Elf_Rel;
typedef Elf64_Rela      Elf_Rela;
typedef Elf64_Sym       Elf_Sym;
typedef Elf64_Ehdr      Elf_Ehdr;
typedef Elf64_Phdr      Elf_Phdr;
typedef Elf64_Shdr      Elf_Shdr;
typedef Elf64_Hash      Elf_Hash;

typedef Elf64_auxv_t    Elf_auxv_t;

#endif                                                      /* ELF_CLASS == ELFCLASS32                  */
#endif                                                      /* __ELF_TYPE_H                             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
