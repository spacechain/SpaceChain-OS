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
** 文   件   名: x86CpuId.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 08 月 03 日
**
** 描        述: x86 体系构架处理器 ID 探测.
*********************************************************************************************************/

#ifndef __ARCH_X86CPUID_H
#define __ARCH_X86CPUID_H

/*********************************************************************************************************
  Byte Offset in X86_CPUID structure
*********************************************************************************************************/

#define X86_CPUID_HIGHVALUE         0                       /*  Offset to highestValue                  */
#define X86_CPUID_VENDORID          4                       /*  Offset to vendorId                      */
#define X86_CPUID_SIGNATURE         16                      /*  Offset to signature                     */
#define X86_CPUID_FEATURES_EBX      20                      /*  Offset to featuresEbx                   */
#define X86_CPUID_FEATURES_ECX      24                      /*  Offset to featuresEcx                   */
#define X86_CPUID_FEATURES_EDX      28                      /*  Offset to featuresEdx                   */
#define X86_CPUID_CACHE_EAX         32                      /*  Offset to cacheEax                      */
#define X86_CPUID_CACHE_EBX         36                      /*  Offset to cacheEbx                      */
#define X86_CPUID_CACHE_ECX         40                      /*  Offset to cacheEcx                      */
#define X86_CPUID_CACHE_EDX         44                      /*  Offset to cacheEdx                      */
#define X86_CPUID_SERIALNO          48                      /*  Offset to serialNo64                    */
#define X86_CPUID_BRAND_STR         56                      /*  Offset to brandString[0]                */
#define X86_CPUID_HIGHVALUE_EXT     104                     /*  Offset to highestValue Extended         */
#define X86_CPUID_EXT_FEATURES_EDX  108                     /*  Offset to featuresEdx Extended          */
#define X86_CPUID_EXT_FEATURES_ECX  112                     /*  Offset to featuresEcx Extended          */
#define X86_CPUID_CACHE_COUNT       116                     /*  Offset to cache count                   */
#define X86_CPUID_CACHE_PARAMS      120                     /*  Offset to cache parameters              */
#define X86_CPUID_MONITOR_EAX       216                     /*  Offset to montor/mwait parameters       */
#define X86_CPUID_MONITOR_EBX       220                     /*  Offset to montor/mwait parameters       */
#define X86_CPUID_MONITOR_ECX       224                     /*  Offset to montor/mwait parameters       */
#define X86_CPUID_MONITOR_EDX       228                     /*  Offset to montor/mwait parameters       */
#define X86_CPUID_DTSPM_EAX         232                     /*  Offset to DTSPM parameters              */
#define X86_CPUID_DTSPM_EBX         236                     /*  Offset to DTSPM parameters              */
#define X86_CPUID_DTSPM_ECX         240                     /*  Offset to DTSPM parameters              */
#define X86_CPUID_DTSPM_EDX         244                     /*  Offset to DTSPM parameters              */
#define X86_CPUID_DCA_EAX           248                     /*  Offset to DCA parameters                */
#define X86_CPUID_PMON_EAX          252                     /*  Offset to PMON parameters               */
#define X86_CPUID_PMON_EBX          256                     /*  Offset to PMON parameters               */
#define X86_CPUID_PMON_ECX          260                     /*  Offset to PMON parameters               */
#define X86_CPUID_PMON_EDX          264                     /*  Offset to PMON parameters               */
#define X86_CPUID_PTOP_COUNT        268                     /*  Offset to PTOP count                    */
#define X86_CPUID_PTOP_PARAMS       272                     /*  Offset to PTOP parameters               */
#define X86_CPUID_XSAVE_EAX         784                     /*  Offset to XSAVE parameters              */
#define X86_CPUID_XSAVE_EBX         788                     /*  Offset to XSAVE parameters              */
#define X86_CPUID_XSAVE_ECX         792                     /*  Offset to XSAVE parameters              */
#define X86_CPUID_XSAVE_EDX         796                     /*  Offset to XSAVE parameters              */
#define X86_CPUID_L2CACHE_EAX       800                     /*  Offset to L2 Cache parameters           */
#define X86_CPUID_L2CACHE_EBX       804                     /*  Offset to L2 Cache parameters           */
#define X86_CPUID_L2CACHE_ECX       808                     /*  Offset to L2 Cache parameters           */
#define X86_CPUID_L2CACHE_EDX       812                     /*  Offset to L2 Cache parameters           */
#define X86_CPUID_APM_EAX           816                     /*  Offset to Advanced Power Mgt parameters */
#define X86_CPUID_APM_EBX           820                     /*  Offset to Advanced Power Mgt parameters */
#define X86_CPUID_APM_ECX           824                     /*  Offset to Advanced Power Mgt parameters */
#define X86_CPUID_APM_EDX           828                     /*  Offset to Advanced Power Mgt parameters */
#define X86_CPUID_VPADRS_EAX        832                     /*  Offset to Virt/Phys Adr Size parameters */
#define X86_CPUID_VPADRS_EBX        836                     /*  Offset to Virt/Phys Adr Size parameters */
#define X86_CPUID_VPADRS_ECX        840                     /*  Offset to Virt/Phys Adr Size parameters */
#define X86_CPUID_VPADRS_EDX        844                     /*  Offset to Virt/Phys Adr Size parameters */

/*********************************************************************************************************
  Bit mask in X86_CPUID_VERSION structure
*********************************************************************************************************/

#define X86_CPUID_BRAND             0x000000ff
#define X86_CPUID_FLUSH_SIZE        0x0000ff00
#define X86_CPUID_NPROC             0x00ff0000
#define X86_CPUID_APIC_ID           0xff000000

/*********************************************************************************************************
  Bit mask in X86_CPUID_FEATURES structure
*********************************************************************************************************/

#define X86_CPUID_FPU               0x00000001
#define X86_CPUID_VME               0x00000002
#define X86_CPUID_DEBUG             0x00000004
#define X86_CPUID_PSE               0x00000008
#define X86_CPUID_TSC               0x00000010
#define X86_CPUID_MSR               0x00000020
#define X86_CPUID_PAE               0x00000040
#define X86_CPUID_MCE               0x00000080
#define X86_CPUID_CX8               0x00000100
#define X86_CPUID_APIC              0x00000200
#define X86_CPUID_SEP               0x00000800
#define X86_CPUID_MTTR              0x00001000
#define X86_CPUID_PGE               0x00002000
#define X86_CPUID_MCA               0x00004000
#define X86_CPUID_CMOV              0x00008000
#define X86_CPUID_PAT               0x00010000
#define X86_CPUID_PSE36             0x00020000
#define X86_CPUID_PSNUM             0x00040000
#define X86_CPUID_CLFLUSH           0x00080000
#define X86_CPUID_DTS               0x00200000
#define X86_CPUID_ACPI              0x00400000
#define X86_CPUID_MMX               0x00800000
#define X86_CPUID_FXSR              0x01000000
#define X86_CPUID_SSE               0x02000000
#define X86_CPUID_SSE2              0x04000000
#define X86_CPUID_SS                0x08000000
#define X86_CPUID_HTT               0x10000000
#define X86_CPUID_TM                0x20000000
#define X86_CPUID_PBE               0x80000000

/*********************************************************************************************************
  Bit mask in X86_CPUID_FEATURES_EXT structure
*********************************************************************************************************/

#define X86_CPUID_SSE3              0x00000001
#define X86_CPUID_PCLMULDQ          0x00000002
#define X86_CPUID_DTES64            0x00000004
#define X86_CPUID_MON               0x00000008
#define X86_CPUID_DS_CPL            0x00000010
#define X86_CPUID_VMX               0x00000020
#define X86_CPUID_SMX               0x00000040
#define X86_CPUID_EST               0x00000080
#define X86_CPUID_TM2               0x00000100
#define X86_CPUID_SSSE3             0x00000200
#define X86_CPUID_CID               0x00000400
#define X86_CPUID_CX16              0x00002000
#define X86_CPUID_XTPR              0x00004000
#define X86_CPUID_PDCM              0x00008000
#define X86_CPUID_DCA               0x00040000
#define X86_CPUID_SSE41             0x00080000
#define X86_CPUID_SSE42             0x00100000
#define X86_CPUID_X2APIC            0x00200000
#define X86_CPUID_MOVBE             0x00400000
#define X86_CPUID_POPCNT            0x00800000
#define X86_CPUID_AES               0x02000000
#define X86_CPUID_XSAVE             0x04000000
#define X86_CPUID_OSXSAVE           0x08000000
#define X86_CPUID_AX86              0x10000000

/*********************************************************************************************************
  Bit mask in X86_CPUID_FEATURES_EXTD structure
*********************************************************************************************************/

#define X86_CPUID_SCALL_SRET        0x00001000
#define X86_CPUID_EXCDIS            0x00200000
#define X86_CPUID_EM64T             0x20000000

/*********************************************************************************************************
  Bit mask in X86_CPUID_FEATURES_EXTC structure
*********************************************************************************************************/

#define X86_CPUID_LAHF              0x00000001

/*********************************************************************************************************
  Bit mask in deterministic cache parameters
*********************************************************************************************************/

#define X86_CPUID_CACHE_TYPE        0x0000001f
#define X86_CPUID_CACHE_LEVEL       0x000000e0

/*********************************************************************************************************
  Bit mask in X86_CPUID_INFO structure
*********************************************************************************************************/

#define X86_CPUID_STEPID            0x0000000f              /*  Processor stepping id mask              */
#define X86_CPUID_MODEL             0x000000f0              /*  Processor model mask                    */
#define X86_CPUID_FAMILY            0x00000f00              /*  Processor family mask                   */
#define X86_CPUID_TYPE              0x00003000              /*  Processor type mask                     */
#define X86_CPUID_EXT_MODEL         0x000f0000              /*  Processor extended model mask           */
#define X86_CPUID_EXT_FAMILY        0x0ff00000              /*  Processor extended family mask          */

/*********************************************************************************************************
  extended model definitions
*********************************************************************************************************/

#define X86_CPUID_EXT_MODEL_PENTIUM     0x00000000
#define X86_CPUID_EXT_MODEL_CORE        0x00010000
#define X86_CPUID_EXT_MODEL_SANDYBRIDGE 0x00020000

/*********************************************************************************************************
  CPU TYPE
*********************************************************************************************************/

#define X86_CPUID_ORIG              0x00000000              /*  Type: original OEM                      */
#define X86_CPUID_OVERD             0x00001000              /*  Type: overdrive                         */
#define X86_CPUID_DUAL              0x00002000              /*  Type: dual                              */

/*********************************************************************************************************
  CPUID definitions
*********************************************************************************************************/

#define X86_CPUID_UNSUPPORTED       0xffffffff              /*  Family: not supported                   */

/*********************************************************************************************************
  Pentium microarchitecture CPUIDs
*********************************************************************************************************/

#define X86_CPUID_PENTIUM           0x00000500              /*  Family: Pentium                         */
#define X86_CPUID_MINUTEIA          0x00000590              /*  Family: MinuteIA/Quark                  */
#define X86_CPUID_PENTIUM4          0x00000000              /*  Extended family: PENTIUM4               */

/*********************************************************************************************************
  Core microarchitecture CPUIDs
*********************************************************************************************************/

#define X86_CPUID_CORE              0x000006e0              /*  Core Solo/Duo                           */
#define X86_CPUID_CORE2             0x000006f0              /*  Core2                                   */
#define X86_CPUID_CORE2_DUO         0x00010670              /*  Core2 Duo                               */
#define X86_CPUID_XEON_5400         0x00010670              /*  Xeon 52xx/54xx                          */
#define X86_CPUID_XEON_7400         0x000106d0              /*  Xeon 74xx Core2                         */

/*********************************************************************************************************
  Nehalem microarchitecture CPUIDs
*********************************************************************************************************/

#define X86_CPUID_XEON_5500         0x000106a0              /*  Xeon 55xx                               */
#define X86_CPUID_XEON_C5500        0x000106e0              /*  Xeon C35xx/C55xx                        */
#define X86_CPUID_XEON_5600         0x000206c0              /*  Xeon 56xx                               */
#define X86_CPUID_XEON_7500         0x000206e0              /*  Xeon 65xx/75xx                          */
#define X86_CPUID_XEON_32NM         0x000206f0              /*  Xeon 65xx/75xx 32 NM                    */
#define X86_CPUID_COREI5_I7M        0x00020650              /*  Arrandale i3 or i5/i7 Mobile 6xx/5xx    */

/*********************************************************************************************************
  Atom microarchitecture CPUIDs
*********************************************************************************************************/

#define X86_CPUID_ATOM              0x000106c0              /*  Atom                                    */
#define X86_CPUID_CEDARVIEW         0x00030660              /*  Atom CedarView CPU N2800                */
#define X86_CPUID_SILVERMONT        0x00030672              /*  Atom Silvermont CPU                     */

/*********************************************************************************************************
  Sandy Bridge microarchitecture CPUIDs
*********************************************************************************************************/

#define X86_CPUID_SANDYBRIDGE       0x000206a0              /*  SandyBridge                             */

/*********************************************************************************************************
  Haswell microarchitecture CPUIDs
*********************************************************************************************************/

#define X86_CPUID_HASWELL_CLIENT    0x000306c0              /*  Haswell Client                          */
#define X86_CPUID_HASWELL_SERVER    0x000306f0              /*  Haswell Server                          */
#define X86_CPUID_HASWELL_ULT       0x00040650              /*  Haswell ULT                             */
#define X86_CPUID_CRYSTAL_WELL      0x00040660              /*  Crystal Well                            */

/*********************************************************************************************************
  Dummy entry
*********************************************************************************************************/

#define X86_CPUID_DUMMY             0                       /*  Dummy CPUID entry                       */

/*********************************************************************************************************
  CPU FAMILY
*********************************************************************************************************/

#define X86_FAMILY_UNSUPPORTED      0                       /*  CPU FAMILY: Not supported               */
#define X86_FAMILY_PENTIUM          2                       /*  CPU FAMILY: Pentium/P5                  */
#define X86_FAMILY_PENTIUM4         5                       /*  CPU FAMILY: Pentium4/P7                 */
#define X86_FAMILY_CORE             6                       /*  CPU FAMILY: Core/Core2                  */
#define X86_FAMILY_ATOM             7                       /*  CPU FAMILY: Atom                        */
#define X86_FAMILY_NEHALEM          8                       /*  CPU FAMILY: Nehalem                     */
#define X86_FAMILY_SANDYBRIDGE      9                       /*  CPU FAMILY: Sandy Bridge                */
#define X86_FAMILY_HASWELL          10                      /*  CPU FAMILY: Haswell                     */
#define X86_FAMILY_MINUTEIA         11                      /*  CPU FAMILY: MinuteIA/Quark              */

/*********************************************************************************************************
  CPU feature override flags
*********************************************************************************************************/

#define X86_CPUID_FEAT_ENABLE       1                       /*  CPUID feature enable                    */
#define X86_CPUID_FEAT_DISABLE      0                       /*  CPUID feature disable                   */

/*********************************************************************************************************
  Support definitions and constants to aid in detecting multi-core processor topology on IA platforms.
*********************************************************************************************************/

#define X86_CPUID_KEY0              0x00000000
#define X86_CPUID_KEY1              0x00000001

#define X86_CPUID_HWMT_BIT          0x10000000              /*  Bit #28                                 */

#define X86_CPUID_LOGICAL_BITS      0x00ff0000              /*  Bits 16 thru 23                         */
#define X86_CPUID_LOGICAL_SHIFT     0x00000010              /*  Shift 16                                */

#define X86_CPUID_CORE_BITS         0xfc000000              /*  Bits 26 thru 31                         */
#define X86_CPUID_CORE_SHIFT        0x0000001a              /*  Shift 26                                */

#define X86_CPUID_LEAF_4            0x04

#define X86_CPUID_APIC_ID_BITS      0xff000000              /*  Bits 24 thru 31                         */
#define X86_CPUID_APIC_ID_SHIFT     0x00000018              /*  Shift 24                                */

#define X86_CPUID_MAX_NUM_CPUS      256                     /*  Max num CPUs                            */

/*********************************************************************************************************
  汇编文件不包含以下内容
*********************************************************************************************************/

#ifndef ASSEMBLY

/*********************************************************************************************************
  CPUID override structure
*********************************************************************************************************/

typedef struct {
    UINT32      type;
    UINT32      feat;
    UINT32      state;
} X86_CPUID_OVERRIDE;

/*********************************************************************************************************
  CPUID signature table entry structure
*********************************************************************************************************/

typedef struct {
    UINT32      signature;
    UINT32      family;
} X86_CPUID_ENTRY;

/*********************************************************************************************************
  CPUID standard features
*********************************************************************************************************/

typedef struct {
    INT         highestValue;                           /*  EAX=0: highest integer value                */
    INT         vendorId[3];                            /*  EAX=0: vendor identification string         */
    INT         signature;                              /*  EAX=1: processor signature                  */
    INT         featuresEbx;                            /*  EAX=1: feature flags EBX                    */
    INT         featuresEcx;                            /*  EAX=1: feature flags ECX                    */
    INT         featuresEdx;                            /*  EAX=1: feature flags EDX                    */
    INT         cacheEax;                               /*  EAX=2: config parameters EAX                */
    INT         cacheEbx;                               /*  EAX=2: config parameters EBX                */
    INT         cacheEcx;                               /*  EAX=2: config parameters ECX                */
    INT         cacheEdx;                               /*  EAX=2: config parameters EDX                */
    INT         serialNo64[2];                          /*  EAX=3: lower 64 of 96 bit serial no         */
    INT         brandString[12];                        /*  EAX=0x8000000[234]: brand strings           */
} X86_CPUID_STD;

/*********************************************************************************************************
  CPUID extended features
*********************************************************************************************************/

typedef struct {
    INT         highestExtValue;                        /*  EAX=0x80000000: highest integer value       */
    INT         featuresExtEdx;                         /*  EAX=0x80000001: Extended feature flags EDX  */
    INT         featuresExtEcx;                         /*  EAX=0x80000001: Extended feature flags ECX  */
    INT         cacheCount;                             /*  Count of cache info blocks                  */
    INT         cacheParams[6][4];                      /*  EAX=0x4: Deterministic cache parameters     */
    INT         monitorParamsEax;                       /*  EAX=0x5: monitor/mwait parameters EAX       */
    INT         monitorParamsEbx;                       /*  EAX=0x5: monitor/mwait parameters EBX       */
    INT         monitorParamsEcx;                       /*  EAX=0x5: monitor/mwait parameters ECX       */
    INT         monitorParamsEdx;                       /*  EAX=0x5: monitor/mwait parameters EDX       */
    INT         dtspmParamsEax;                         /*  EAX=0x6: DTSPM parameters EAX               */
    INT         dtspmParamsEbx;                         /*  EAX=0x6: DTSPM parameters EBX               */
    INT         dtspmParamsEcx;                         /*  EAX=0x6: DTSPM parameters ECX               */
    INT         dtspmParamsEdx;                         /*  EAX=0x6: DTSPM parameters EDX               */
    INT         dcaParamsEax;                           /*  EAX=0x9: DCA parameters EAX                 */
    INT         pMonParamsEax;                          /*  EAX=0xA: PMON parameters EAX                */
    INT         pMonParamsEbx;                          /*  EAX=0xA: PMON parameters EBX                */
    INT         pMonParamsEcx;                          /*  EAX=0xA: PMON parameters ECX                */
    INT         pMonParamsEdx;                          /*  EAX=0xA: PMON parameters EDX                */
    INT         pTopCount;                              /*  Count of processor topology info blocks     */
    INT         pTopParams[32][4];                      /*  EAX=0xB: x2APIC/Processor Topology          */
    INT         xsaveParamsEax;                         /*  EAX=0xD: XSAVE parameters EAX               */
    INT         xsaveParamsEbx;                         /*  EAX=0xD: XSAVE parameters EBX               */
    INT         xsaveParamsEcx;                         /*  EAX=0xD: XSAVE parameters ECX               */
    INT         xsaveParamsEdx;                         /*  EAX=0xD: XSAVE parameters EDX               */
    INT         l2ParamsEax;                            /*  EAX=0x80000006: Extended L2 Cache EAX       */
    INT         l2ParamsEbx;                            /*  EAX=0x80000006: Extended L2 Cache EBX       */
    INT         l2ParamsEcx;                            /*  EAX=0x80000006: Extended L2 Cache ECX       */
    INT         l2ParamsEdx;                            /*  EAX=0x80000006: Extended L2 Cache EDX       */
    INT         apmParamsEax;                           /*  EAX=0x80000007: Advanced Power Mgt EAX      */
    INT         apmParamsEbx;                           /*  EAX=0x80000007: Advanced Power Mgt EBX      */
    INT         apmParamsEcx;                           /*  EAX=0x80000007: Advanced Power Mgt ECX      */
    INT         apmParamsEdx;                           /*  EAX=0x80000007: Advanced Power Mgt EDX      */
    INT         vpAdrSizesEax;                          /*  EAX=0x80000008: Virt/Phys Addr Sizes EAX    */
    INT         vpAdrSizesEbx;                          /*  EAX=0x80000008: Virt/Phys Addr Sizes EBX    */
    INT         vpAdrSizesEcx;                          /*  EAX=0x80000008: Virt/Phys Addr Sizes ECX    */
    INT         vpAdrSizesEdx;                          /*  EAX=0x80000008: Virt/Phys Addr Sizes EDX    */
} X86_CPUID_EXT;

/*********************************************************************************************************
  CPUID standard and extended features
*********************************************************************************************************/

typedef struct {
    X86_CPUID_STD   std;                                /*  Standard CPUID features                     */
    X86_CPUID_EXT   ext;                                /*  Extended CPUID features                     */
} X86_CPUID;

/*********************************************************************************************************
  CPUID Fields in the EAX register when EAX=1
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  stepid:4;                               /*  Processor stepping id mask                  */
        UINT32  model:4;                                /*  Processor model mask                        */
        UINT32  family:4;                               /*  Processor family mask                       */
        UINT32  type:2;                                 /*  Processor type mask                         */
        UINT32  reserved1:2;
        UINT32  modelExt:4;                             /*  Processor extended model mask               */
        UINT32  familyExt:8;                            /*  Processor extended family mask              */
        UINT32  reserved2:4;
    } field;
    UINT32      value;
} X86_CPUID_VERSION;

/*********************************************************************************************************
  CPUID Fields in the EBX register when EAX=1
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  brand:8;                                /*  Brand index                                 */
        UINT32  flushSize:8;                            /*  CLFLUSH line size                           */
        UINT32  nproc:8;                                /*  Number of local processors                  */
        UINT32  apicId:8;                               /*  Local APIC id                               */
    } field;
    UINT32      value;
} X86_CPUID_INFO;

/*********************************************************************************************************
  CPUID: feature bit definitions
  CPUID Fields in the EDX register when EAX=1
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  fpu:1;                                  /*  FPU on chip                                 */
        UINT32  vme:1;                                  /*  Virtual 8086 mode enhancement               */
        UINT32  de:1;                                   /*  Debugging extensions                        */
        UINT32  pse:1;                                  /*  Page size extension                         */
        UINT32  tsc:1;                                  /*  Time stamp counter                          */
        UINT32  msr:1;                                  /*  RDMSR and WRMSR support                     */
        UINT32  pae:1;                                  /*  Physical address extensions                 */
        UINT32  mce:1;                                  /*  Machine check exception                     */
        UINT32  cx8:1;                                  /*  CMPXCHG8 inst                               */
        UINT32  apic:1;                                 /*  APIC on chip                                */
        UINT32  reserved1:1;
        UINT32  sep:1;                                  /*  SEP, Fast System Call                       */
        UINT32  mtrr:1;                                 /*  MTRR                                        */
        UINT32  pge:1;                                  /*  PTE global bit                              */
        UINT32  mca:1;                                  /*  Machine check arch.                         */
        UINT32  cmov:1;                                 /*  Cond. move/cmp. inst                        */
        UINT32  pat:1;                                  /*  Page attribute table                        */
        UINT32  pse36:1;                                /*  36 bit page size extension                  */
        UINT32  psnum:1;                                /*  Processor serial number                     */
        UINT32  clflush:1;                              /*  CLFLUSH inst supported                      */
        UINT32  reserved2:1;
        UINT32  dts:1;                                  /*  Debug Store                                 */
        UINT32  acpi:1;                                 /*  TM and SCC supported                        */
        UINT32  mmx:1;                                  /*  MMX technology supported                    */
        UINT32  fxsr:1;                                 /*  Fast FP save and restore                    */
        UINT32  sse:1;                                  /*  SSE supported                               */
        UINT32  sse2:1;                                 /*  SSE2 supported                              */
        UINT32  ss:1;                                   /*  Self Snoop supported                        */
        UINT32  htt:1;                                  /*  Hyper Threading Technology                  */
        UINT32  tm:1;                                   /*  Thermal Monitor supported                   */
        UINT32  reserved3:1;
        UINT32  pbe:1;                                  /*  Pend break enable                           */
    } field;
    UINT32      value;
} X86_CPUID_EDX_FEATURES;

/*********************************************************************************************************
  CPUID Fields in the ECX register when EAX=1
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  sse3:1;                                 /*  SSE3 Extensions                             */
        UINT32  pclmuldq:1;                             /*  PCLMULDQ instruction                        */
        UINT32  dtes64:1;                               /*  64-bit debug store                          */
        UINT32  mon:1;                                  /*  Monitor/wait                                */
        UINT32  ds_cpl:1;                               /*  CPL qualified Debug Store                   */
        UINT32  vmx:1;                                  /*  Virtual Machine Technology                  */
        UINT32  smx:1;                                  /*  Safer mode extensions                       */
        UINT32  est:1;                                  /*  Enhanced Speedstep Technology               */
        UINT32  tm2:1;                                  /*  Thermal Monitor 2 supported                 */
        UINT32  ssse3:1;                                /*  SSSE3 Extensions                            */
        UINT32  cid:1;                                  /*  L1 context ID                               */
        UINT32  debugmsr:1;                             /*  IA32_DEBUG_INTERFACE_MSR support            */
        UINT32  fma:1;                                  /*  FMA Extensions Using XMM state              */
        UINT32  cx16:1;                                 /*  CMPXCHG16B instruction                      */
        UINT32  xtpr:1;                                 /*  Update control                              */
        UINT32  pdcm:1;                                 /*  Perfmon/Debug capability                    */
        UINT32  reserved3:1;
        UINT32  asid_pcid:1;                            /*  ASID-PCID support                           */
        UINT32  dca:1;                                  /*  Direct cache access                         */
        UINT32  sse41:1;                                /*  SSE4.1 Extensions                           */
        UINT32  sse42:1;                                /*  SSE4.2 Extensions                           */
        UINT32  x2apic:1;                               /*  x2APIC feature                              */
        UINT32  movbe:1;                                /*  MOVBE instruction                           */
        UINT32  popcnt:1;                               /*  POPCNT instruction                          */
        UINT32  tsc_dline:1;                            /*  TSC-deadline Timer                          */
        UINT32  aes:1;                                  /*  AES instruction                             */
        UINT32  xsave:1;                                /*  XSAVE/XRSTOR States                         */
        UINT32  osxsave:1;                              /*  OS-Enabled Extended State management        */
        UINT32  avx:1;                                  /*  OS-Enabled Extended State management        */
        UINT32  f16c:1;                                 /*  Float16 instructions support                */
        UINT32  rdrand:1;                               /*  Read Random Number instructions support     */
        UINT32  reserved4:1;
    } field;
    UINT32      value;
} X86_CPUID_ECX_FEATURES;

/*********************************************************************************************************
  CPUID Fields in the EDX register when EAX=0x80000001
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  reserved1:11;                           /*  0-10 reserved                               */
        UINT32  scall_sret:1;                           /*  Syscall/Sysret available in 64 bit mode     */
        UINT32  reserved2:8;                            /*  12-19 reserved                              */
        UINT32  excdis:1;                               /*  Execute Disable Bit available               */
        UINT32  reserved3:5;                            /*  21-25 reserved                              */
        UINT32  gbpss:1;                                /*  1 GB Page Size Support                      */
        UINT32  rdtscp:1;                               /*  RDTSCP instruction and IA32_TSC_AUX_MSR     */
        UINT32  reserved4:1;                            /*  28 reserved                                 */
        UINT32  em64t:1;                                /*  EM64T available                             */
        UINT32  reserved5:2;                            /*  30-31 reserved                              */
    } field;
    UINT32      value;
} X86_CPUID_EDX_FEATURES_EXT;

/*********************************************************************************************************
  CPUID Fields in the ECX register when EAX=0x80000001
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  lahf:1;                                 /*  LAHF and SAHF instructions                  */
        UINT32  reserved1:4;                            /*  1-4 reserved                                */
        UINT32  abm:1;                                  /*  Advanced Bit Manipulation instructions      */
        UINT32  reserved2:26;                           /*  6-31 reserved                               */
    } field;
    UINT32      value;
} X86_CPUID_ECX_FEATURES_EXT;

/*********************************************************************************************************
  Deterministic cache parameter descriptions
  Fields in the EAX register when EAX=0x4
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  type:5;                                 /*  Cache type                                  */
        UINT32  level:3;                                /*  Cache level                                 */
        UINT32  self_init:1;                            /*  Self-initializing cache level               */
        UINT32  assoc:1;                                /*  Fully associative cache level               */
        UINT32  reserved1:4;                            /*  10-13 reserved                              */
        UINT32  threads:12;                             /*  Max threads sharing cache - add 1           */
        UINT32  apics:6;                                /*  Num reserved APIC IDs - add 1               */
    } field;
    UINT32      value;
} X86_CPUID_EAX_CACHE_PARAMS;

/*********************************************************************************************************
  Fields in the EBX register when EAX=0x4
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  line_size:12;                           /*  System coherency line size - add 1          */
        UINT32  partitions:10;                          /*  Physical line partitions - add 1            */
        UINT32  assoc:10;                               /*  Ways of Associativity - add 1               */
    } field;
    UINT32      value;
} X86_CPUID_EBX_CACHE_PARAMS;

/*********************************************************************************************************
  Fields in the ECX register when EAX=0x4
*********************************************************************************************************/

typedef struct {
    UINT32      sets;                                   /*  Number of Sets - add 1                      */
} X86_CPUID_ECX_CACHE_PARAMS;

/*********************************************************************************************************
  Fields in the EDX register when EAX=0x4
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  wbindv:1;                               /*  WBINDV/INVD behavior on lower levels        */
        UINT32  inclusive:1;                            /*  Cache is inclusize to lower levels          */
        UINT32  complex_addr:1;                         /*  Cache uses complex function to index        */
        UINT32  reserved1:29;                           /*  3-31 reserved                               */
    } field;
    UINT32      value;
} X86_CPUID_EDX_CACHE_PARAMS;

/*********************************************************************************************************
  MONITOR/MWAIT parameters
  Fields in the EAX register when EAX=0x5
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  smline:16;                              /*  Smallest monitor line size in bytes         */
        UINT32  reserved1:16;                           /*  15-31 reserved                              */
    } field;
    UINT32      value;
} X86_CPUID_EAX_MONITOR_PARAMS;

/*********************************************************************************************************
  Fields in the EBX register when EAX=0x5
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  lgline:16;                              /*  Largest monitor line size in bytes          */
        UINT32  reserved1:16;                           /*  15-31 reserved                              */
    } field;
    UINT32      value;
} X86_CPUID_EBX_MONITOR_PARAMS;

/*********************************************************************************************************
  Fields in the ECX register when EAX=0x5
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  mwait:1;                                /*  MONITOR/MWAIT extensions supported          */
        UINT32  intr_break:1;                           /*  Break-events for MWAIT                      */
        UINT32  reserved1:30;                           /*  2-31 reserved                               */
    } field;
    UINT32      value;
} X86_CPUID_ECX_MONITOR_PARAMS;

/*********************************************************************************************************
  Fields in the EDX register when EAX=0x5
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  c0:4;                                   /*  C0 sub-states                               */
        UINT32  c1:4;                                   /*  C1 sub-states                               */
        UINT32  c2:4;                                   /*  C2 sub-states                               */
        UINT32  c3_6:4;                                 /*  C3/6 sub-states                             */
        UINT32  c4_7:4;                                 /*  C4/7 sub-states                             */
        UINT32  reserved1:12;                           /*  20-31 reserved                              */
    } field;
    UINT32      value;
} X86_CPUID_EDX_MONITOR_PARAMS;

/*********************************************************************************************************
  Digital Thermal Sensor and Power Management parameters
  Fields in the EAX register when EAX=0x6
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  dts:1;                                  /*  Digital thermal sensor capable              */
        UINT32  turbo:1;                                /*  Turbo Boost technology                      */
        UINT32  invar_apic:1;                           /*  Invariant Apic Timer                        */
        UINT32  reserved1:1;                            /*  3 reserved                                  */
        UINT32  pln_at_core:1;                          /*  Power Limit Notification at Core Level      */
        UINT32  fgcm:1;                                 /*  Fine Grained Clock Modulation               */
        UINT32  thermint_stat:1;                        /*  Package Thermal Interrupt and Status support*/
        UINT32  reserved2:25;                           /*  7-31 reserved                               */
    } field;
    UINT32      value;
} X86_CPUID_EAX_DTSPM_PARAMS;

typedef union {
    struct {
        UINT32  intr:4;                                 /*  Number of interrupt thresholds              */
        UINT32  reserved1:28;                           /*  4-31 reserved                               */
    } field;
    UINT32      value;
} X86_CPUID_EBX_DTSPM_PARAMS;

typedef union {
    struct {
        UINT32  hcfc:1;                                 /*  Hardware Coordination Feedback Capability   */
        UINT32  acnt2:1;                                /*  ACNT2 Capability                            */
        UINT32  reserved1:1;                            /*  2 reserved2 reserved                        */
        UINT32  efps:1;                                 /*  Energy Efficient Policy support             */
        UINT32  reserved2:28;                           /*  4-31 reserved                               */
    } field;
    UINT32      value;
} X86_CPUID_ECX_DTSPM_PARAMS;

typedef union {
    struct {
        UINT32  reserved1:32;                           /*  0-31 reserved                               */
    } field;
    UINT32      value;
} X86_CPUID_EDX_DTSPM_PARAMS;

/*********************************************************************************************************
  Direct Cache Access (DCA) parameters
  Fields in the EAX register when EAX=0x9
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  dca_cap:32;                             /*  PLATFORM_DCA_CAP MSR bits                   */
    } field;
    UINT32      value;
} X86_CPUID_EAX_DCA_PARAMS;

/*********************************************************************************************************
  Performance Monitor Features
  Fields in the EAX register when EAX=0xA
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  ver:8;                                  /*  Architecture PerfMon version                */
        UINT32  ncounters:8;                            /*  Number of counters per logical processor    */
        UINT32  nbits:8;                                /*  Number of bits per programmable counter     */
        UINT32  nevts:8;                                /*  Number of events per logical processor      */
    } field;
    UINT32      value;
} X86_CPUID_EAX_PMON_PARAMS;

/*********************************************************************************************************
  Fields in the EBX register when EAX=0xA, 0 = supported
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  core_cycles:1;                          /*  Core Cycles                                 */
        UINT32  instr_ret:1;                            /*  Instructions retired                        */
        UINT32  ref_cycles:1;                           /*  Reference cycles                            */
        UINT32  cache_ref:1;                            /*  Last level cache references                 */
        UINT32  cache_miss:1;                           /*  Last level cache misses                     */
        UINT32  br_instr:1;                             /*  Branch instructions retired                 */
        UINT32  br_mispr:1;                             /*  Branch mispredicts retired                  */
        UINT32  reserved:25;                            /*  7-31 reserved                               */
    } field;
    UINT32      value;
} X86_CPUID_EBX_PMON_PARAMS;

/*********************************************************************************************************
  Fields in the ECX register when EAX=0xA
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  reserved:32;                            /*  0:31 reserved                               */
    } field;
    UINT32      value;
} X86_CPUID_ECX_PMON_PARAMS;

/*********************************************************************************************************
  Fields in the EDX register when EAX=0xA
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  nfix_cntrs:5;                           /*  Number of fixed counters                    */
        UINT32  nbit_cntrs:8;                           /*  Number of bits in the fixed counters        */
        UINT32  reserved:19;                            /*  13:31 reserved                              */
    } field;
    UINT32      value;
} X86_CPUID_EDX_PMON_PARAMS;

/*********************************************************************************************************
  x2APIC Features / Core Topology
  Fields in the EAX register when EAX=0xB
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  nbits:5;                                /*  Number of bits to shift right APIC ID       */
        UINT32  reserved1:27;                           /*  5:31 reserved                               */
    } field;
    UINT32      value;
} X86_CPUID_EAX_PTOP_PARAMS;

/*********************************************************************************************************
  Fields in the EBX register when EAX=0xB
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  nproc:16;                               /*  Number of logical processors at this level  */
        UINT32  reserved1:16;                           /*  16:31 reserved                              */
    } field;
    UINT32      value;
} X86_CPUID_EBX_PTOP_PARAMS;

/*********************************************************************************************************
  Fields in the ECX register when EAX=0xB
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  lvl_num:8;                              /*  Level number                                */
        UINT32  lvl_type:8;                             /*  Level type                                  */
        UINT32  reserved1:16;                           /*  16:31 reserved                              */
    } field;
    UINT32      value;
} X86_CPUID_ECX_PTOP_PARAMS;

/*********************************************************************************************************
  Fields in the EDX register when EAX=0xB
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  xapic_id:32;                            /*  Extended apic ID                            */
    } field;
    UINT32      value;
} X86_CPUID_EDX_PTOP_PARAMS;

/*********************************************************************************************************
  Extended L2 Cache Features
  Fields in the ECX register when EAX=0x80000006
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  cache_line:8;                           /*  L2 Cache line size ibn bytes                */
        UINT32  reserved1:4;                            /*  8:11 reserved                               */
        UINT32  assoc:4;                                /*  L2 Cache Associativity                      */
        UINT32  cache_size:16;                          /*  L2 Cache size in 1024 byte units            */
    } field;
    UINT32      value;
} X86_CPUID_ECX_L2_PARAMS;

/*********************************************************************************************************
  Advanced Power Management
  Fields in the EDX register when EAX=0x80000007
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  reserved1:8;                            /*  0:7 reserved                                */
        UINT32  tsc_inv:1;                              /*  TSC invariance                              */
        UINT32  reserved2:23;                           /*  9:31 reserved                               */
    } field;
    UINT32      value;
} X86_CPUID_EDX_APM_PARAMS;

/*********************************************************************************************************
  Virtual and Physical Address Sizes
  Fields in the EAX register when EAX=0x80000008
*********************************************************************************************************/

typedef union {
    struct {
        UINT32  phys:8;                                 /*  Physical address size in bits               */
        UINT32  virt:8;                                 /*  Virtual address size  in bits               */
        UINT32  reserved1:16;                           /*  16:31 reserved                              */
    } field;
    UINT32      value;
} X86_CPUID_EAX_VPADRSIZES_PARAMS;

/*********************************************************************************************************
  CPU 特性
*********************************************************************************************************/

typedef struct {
    size_t      CPUF_stCacheFlushBytes;                                 /*  CLFLUSH 字节数              */
    INT         CPUF_iICacheWaySize;                                    /*  I-Cache way size            */
    INT         CPUF_iDCacheWaySize;                                    /*  I-Cache way size            */
    BOOL        CPUF_bHasCLFlush;                                       /*  Has CLFLUSH inst?           */
    BOOL        CPUF_bHasAPIC;                                          /*  Has APIC on chip?           */
    UINT        CPUF_uiProcessorFamily;                                 /*  Processor Family            */
    BOOL        CPUF_bHasX87FPU;                                        /*  Has X87 FPU?                */
    BOOL        CPUF_bHasSSE;                                           /*  Has SSE?                    */
    BOOL        CPUF_bHasSSE2;                                          /*  Has SSE?                    */
    BOOL        CPUF_bHasFXSR;                                          /*  Has FXSR?                   */
    BOOL        CPUF_bHasXSAVE;                                         /*  Has XSAVE?                  */
    BOOL        CPUF_bHasAVX;                                           /*  Has AVX?                    */
    BOOL        CPUF_bHasMMX;                                           /*  Has MMX?                    */
    size_t      CPUF_stXSaveCtxSize;                                    /*  XSAVE context size          */
    BOOL        CPUF_bHasMTRR;                                          /*  Has MTRR?                   */
#define INFO_STR_LEN    256
    CHAR        CPUF_pcCpuInfo[INFO_STR_LEN];                           /*  CPU info                    */
    CHAR        CPUF_pcCacheInfo[INFO_STR_LEN];                         /*  Cache info                  */
} X86_CPU_FEATURE;

extern X86_CPU_FEATURE      _G_x86CpuFeature;

#define X86_FEATURE_CACHE_FLUSH_BYTES  _G_x86CpuFeature.CPUF_stCacheFlushBytes
#define X86_FEATURE_ICACHE_WAY_SIZE    _G_x86CpuFeature.CPUF_iICacheWaySize
#define X86_FEATURE_DCACHE_WAY_SIZE    _G_x86CpuFeature.CPUF_iDCacheWaySize
#define X86_FEATURE_HAS_CLFLUSH        _G_x86CpuFeature.CPUF_bHasCLFlush
#define X86_FEATURE_HAS_APIC           _G_x86CpuFeature.CPUF_bHasAPIC
#define X86_FEATURE_PROCESSOR_FAMILY   _G_x86CpuFeature.CPUF_uiProcessorFamily
#define X86_FEATURE_HAS_X87FPU         _G_x86CpuFeature.CPUF_bHasX87FPU
#define X86_FEATURE_HAS_SSE            _G_x86CpuFeature.CPUF_bHasSSE
#define X86_FEATURE_HAS_SSE2           _G_x86CpuFeature.CPUF_bHasSSE2
#define X86_FEATURE_HAS_FXSR           _G_x86CpuFeature.CPUF_bHasFXSR
#define X86_FEATURE_HAS_XSAVE          _G_x86CpuFeature.CPUF_bHasXSAVE
#define X86_FEATURE_HAS_AVX            _G_x86CpuFeature.CPUF_bHasAVX
#define X86_FEATURE_HAS_MMX            _G_x86CpuFeature.CPUF_bHasMMX
#define X86_FEATURE_XSAVE_CTX_SIZE     _G_x86CpuFeature.CPUF_stXSaveCtxSize
#define X86_FEATURE_HAS_MTRR           _G_x86CpuFeature.CPUF_bHasMTRR
#define X86_FEATURE_CPU_INFO           _G_x86CpuFeature.CPUF_pcCpuInfo
#define X86_FEATURE_CACHE_INFO         _G_x86CpuFeature.CPUF_pcCacheInfo

/*********************************************************************************************************
  函数声明
*********************************************************************************************************/

extern X86_CPUID  *x86CpuIdGet(VOID);
extern VOID        x86CpuIdProbe(VOID);
extern VOID        x86CpuIdShow(VOID);

extern INT         x86CpuIdAdd(X86_CPUID_ENTRY  *pentry);
extern INT         x86CpuIdOverride(X86_CPUID_OVERRIDE  *pentries, INT  iCount);
extern UINT8       x86CpuIdBitField(UINT8  ucFullId, UINT8  ucMaxSubIdValue, UINT8  ucShiftCount);

extern UINT        x86CpuIdMaxNumLProcsPerCore(VOID);

extern UINT64      x86CpuIdGetFreq(VOID);
extern INT         x86CpuIdSetFreq(UINT64  ui64Freq);
extern UINT64      x86CpuIdCalcFreq(VOID);

/*********************************************************************************************************
  汇编函数声明
*********************************************************************************************************/

extern BOOL        x86CpuIdHWMTSupported(VOID);
extern UINT        x86CpuIdMaxNumLProcsPerPkg(VOID);
extern UINT        x86CpuIdMaxNumCoresPerPkg(VOID);
extern UINT        x86CpuIdBitFieldWidth(UINT  iItemCount);
extern UINT8       x86CpuIdInitialApicId(VOID);
extern VOID        x86CpuIdProbeHw(X86_CPUID  *pcpuid);

#endif                                                                  /*  ASSEMBLY                    */

#endif                                                                  /*  __ARCH_X86CPUID_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
