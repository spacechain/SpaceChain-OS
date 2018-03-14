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
** 文   件   名: miiDev.h
**
** 创   建   人: XuGuizhou
**
** 文件创建日期: 2010 年 08 月 13 日
**
** 描        述: MII 总线库
*********************************************************************************************************/

#ifndef __MII_DEV_H
#define __MII_DEV_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_NET_EN > 0)

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************
  Macro Definition
*********************************************************************************************************/
#define MII_OK                      ERROR_NONE
#define MII_ERROR                   PX_ERROR

#define MII_DEV_ERRNO               (ERRMAX)

#define MII_PHY_LINK_DOWN           (MII_DEV_ERRNO + 1)
#define MII_PHY_NULL                (MII_DEV_ERRNO + 2)
#define MII_PHY_NO_ABILITY          (MII_DEV_ERRNO + 3)
#define MII_PHY_AN_FAIL             (MII_DEV_ERRNO + 4)

#define MII_MAX_PHY_NUM             32
/*********************************************************************************************************
  MII Reg Address
*********************************************************************************************************/
#define MII_CTRL_REG                0x0                                 /* Control Register             */
#define MII_STAT_REG                0x1                                 /* Status Register              */
#define MII_PHY_ID1_REG             0x2                                 /* PHY identifier 1 Register    */
#define MII_PHY_ID2_REG             0x3                                 /* PHY identifier 2 Register    */
#define MII_AN_ADS_REG              0x4                                 /* Auto-Negotiation             */
                                                                        /* Advertisement Register       */
#define MII_AN_PRTN_REG             0x5                                 /* Auto-Negotiation             */
                                                                        /* partner ability Register     */
#define MII_AN_EXP_REG              0x6                                 /* Auto-Negotiation             */
                                                                        /* Expansion Register           */
#define MII_AN_NEXT_REG             0x7                                 /* Auto-Negotiation             */
                                                                        /* next-page transmit Register  */
#define MII_AN_PRTN_NEXT_REG        0x8                                 /* Link partner recv next page  */
#define MII_MASSLA_CTRL_REG         0x9                                 /* MATER-SLAVE control register */
#define MII_MASSLA_STAT_REG         0xa                                 /* MATER-SLAVE status register  */
#define MII_EXT_STAT_REG            0xf                                 /* Extented status register     */
/*********************************************************************************************************
  MII control register bit
*********************************************************************************************************/
#define MII_CTRL_1000               0x0040                              /* 1 = 1000mb when MII_CR_100   */
                                                                        /* is also 1                    */
#define MII_CTRL_COLL_TEST          0x0080                              /* collision test               */
#define MII_CTRL_FDX                0x0100                              /* FDX =1, half duplex =0       */
#define MII_CTRL_RESTART            0x0200                              /* restart auto negotiation     */
#define MII_CTRL_ISOLATE            0x0400                              /* isolate PHY from MII         */
#define MII_CTRL_POWER_DOWN         0x0800                              /* power down                   */
#define MII_CTRL_AUTO_EN            0x1000                              /* auto-negotiation enable      */
#define MII_CTRL_100                0x2000                              /* 0 = 10mb, 1 = 100mb          */
#define MII_CTRL_LOOPBACK           0x4000                              /* 0 = normal, 1 = loopback     */
#define MII_CTRL_RESET              0x8000                              /* 0 = normal, 1 = PHY reset    */
#define MII_CTRL_NORM_EN            0x0000                              /* just enable the PHY          */
#define MII_CTRL_DEF_0_MASK         0xca7f                              /* they must return zero        */
#define MII_CTRL_RES_MASK           0x003f                              /* reserved bits,return zero    */
/*********************************************************************************************************
  MII Status register bit definitions
*********************************************************************************************************/
#define MII_SR_LINK_STATUS          0x0004                              /* link Status -- 1 = link      */
#define MII_SR_AUTO_SEL             0x0008                              /* auto speed select capable    */
#define MII_SR_REMOTE_FAULT         0x0010                              /* Remote fault detect          */
#define MII_SR_AUTO_NEG             0x0020                              /* auto negotiation complete    */
#define MII_SR_EXT_STS              0x0100                              /* extended sts in reg 15       */
#define MII_SR_T2_HALF_DPX          0x0200                              /* 100baseT2 HD capable         */
#define MII_SR_T2_FULL_DPX          0x0400                              /* 100baseT2 FD capable         */
#define MII_SR_10T_HALF_DPX         0x0800                              /* 10BaseT HD capable           */
#define MII_SR_10T_FULL_DPX         0x1000                              /* 10BaseT FD capable           */
#define MII_SR_TX_HALF_DPX          0x2000                              /* TX HD capable                */
#define MII_SR_TX_FULL_DPX          0x4000                              /* TX FD capable                */
#define MII_SR_T4                   0x8000                              /* T4 capable                   */
#define MII_SR_ABIL_MASK            0xff80                              /* abilities mask               */
#define MII_SR_EXT_CAP              0x0001                              /* extended capabilities        */
#define MII_SR_SPEED_SEL_MASK       0xf800                              /* Mask to extract just speed   */
                                                                        /* capabilities  from status    */
                                                                        /* register.                    */
/*********************************************************************************************************
  PHY device definitions
*********************************************************************************************************/
#define MII_PHY_PRE_INIT            0x0001                              /* PHY info pre-initialized     */
#define MII_PHY_NWAIT_STAT          0x0002                              /* PHY not loop wait status     */
#define MII_PHY_AUTO                0x0010                              /* auto-negotiation allowed     */
#define MII_PHY_TBL                 0x0020                              /* use negotiation table        */
#define MII_PHY_100                 0x0040                              /* PHY may use 100Mbit speed    */
#define MII_PHY_10                  0x0080                              /* PHY may use 10Mbit speed     */
#define MII_PHY_FD                  0x0100                              /* PHY may use full duplex      */
#define MII_PHY_HD                  0x0200                              /* PHY may use half duplex      */
#define MII_PHY_ISO                 0x0400                              /* isolate all PHYs             */
#define MII_PHY_PWR_DOWN            0x0800                              /* power down mode              */
#define MII_PHY_DEF_SET             0x1000                              /* set a default mode           */
#define MII_ALL_BUS_SCAN            0x2000                              /* scan the all bus             */
#define MII_PHY_MONITOR             0x4000                              /* monitor the PHY's status     */
#define MII_PHY_INIT                0x8000                              /* PHY info initialized         */
#define MII_PHY_1000T_FD            0x10000                             /* PHY may use 1000-T full dup  */
#define MII_PHY_1000T_HD            0x20000                             /* PHY may use 1000-T half dup  */
#define MII_PHY_TX_FLOW_CTRL        0x40000                             /* Transmit flow control        */
#define MII_PHY_RX_FLOW_CTRL        0x80000                             /* Receive flow control         */
#define MII_PHY_GMII_TYPE           0x100000                            /* GMII = 1, MII = 0            */
#define MII_PHY_ISO_UNAVAIL         0x200000                            /* ctrl reg isolate func not    */
                                                                        /* available                    */
/*********************************************************************************************************
  MII Extented Status register bit definition
*********************************************************************************************************/
#define MII_EXT_STAT_1000T_HD       0x1000
#define MII_EXT_STAT_1000T_FD       0x2000
#define MII_EXT_STAT_1000X_HD       0x4000
#define MII_EXT_STAT_1000X_FD       0x8000
/*********************************************************************************************************
  MII Link Method
*********************************************************************************************************/
#define MII_PHY_LINK_UNKNOWN        0x0                                 /* link method-Unknown          */
#define MII_PHY_LINK_AUTO           0x1                                 /* link method-Auto-Negotiation */
#define MII_PHY_LINK_FORCE          0x2                                 /* link method-Force link       */
/*********************************************************************************************************
  MII Next Page bit definitions
*********************************************************************************************************/
#define MII_NP_TOGGLE               0x0800                              /* toggle bit                   */
#define MII_NP_ACK2                 0x1000                              /* acknowledge two              */
#define MII_NP_MSG                  0x2000                              /* message page                 */
#define MII_NP_ACK1                 0x4000                              /* acknowledge one              */
#define MII_NP_NP                   0x8000                              /* nexp page will follow        */
/*********************************************************************************************************
  MII AN advertisement Register bit definition
*********************************************************************************************************/
#define MII_ANAR_10TX_HD            0x0020
#define MII_ANAR_10TX_FD            0x0040
#define MII_ANAR_100TX_HD           0x0080
#define MII_ANAR_100TX_FD           0x0100
#define MII_ANAR_100T_4             0x0200
#define MII_ANAR_PAUSE              0x0400
#define MII_ANAR_ASM_PAUSE          0x0800
#define MII_ANAR_REMORT_FAULT       0x2000
#define MII_ANAR_NEXT_PAGE          0x8000
#define MII_ANAR_PAUSE_MASK         0x0c00
/*********************************************************************************************************
  MII Master-Slave Control register bit definition
*********************************************************************************************************/
#define MII_MASSLA_CTRL_1000T_HD    0x100
#define MII_MASSLA_CTRL_1000T_FD    0x200
#define MII_MASSLA_CTRL_PORT_TYPE   0x400
#define MII_MASSLA_CTRL_CONFIG_VAL  0x800
#define MII_MASSLA_CTRL_CONFIG_EN   0x1000
/*********************************************************************************************************
  MII Master-Slave Status register bit definition
*********************************************************************************************************/
#define MII_MASSLA_STAT_LP1000T_HD  0x400
#define MII_MASSLA_STAT_LP1000T_FD  0x800
#define MII_MASSLA_STAT_REMOTE_RCV  0x1000
#define MII_MASSLA_STAT_LOCAL_RCV   0x2000
#define MII_MASSLA_STAT_CONF_RES    0x4000
#define MII_MASSLA_STAT_CONF_FAULT  0x8000
/*********************************************************************************************************
  technology ability field bit definitions
*********************************************************************************************************/
#define MII_TECH_10BASE_T           0x0020                              /* 10Base-T                     */
#define MII_TECH_10BASE_FD          0x0040                              /* 10Base-T Full Duplex         */
#define MII_TECH_100BASE_TX         0x0080                              /* 100Base-TX */
#define MII_TECH_100BASE_TX_FD      0x0100                              /* 100Base-TX Full Duplex       */
#define MII_TECH_100BASE_T4         0x0200                              /* 100Base-T4 */

#define MII_TECH_PAUSE              0x0400                              /* PAUSE                        */
#define MII_TECH_ASM_PAUSE          0x0800                              /* Asym pause                   */
#define MII_TECH_PAUSE_MASK         0x0c00

#define MII_ADS_TECH_MASK           0x1fe0                              /* technology abilities mask    */
#define MII_TECH_MASK               MII_ADS_TECH_MASK
#define MII_ADS_SEL_MASK            0x001f                              /* selector field mask          */

#define MII_AN_FAIL                 0x10                                /* auto-negotiation fail        */
#define MII_STAT_FAIL               0x20                                /* errors in the status register*/
#define MII_PHY_NO_ABLE             0x40                                /* the PHY lacks some abilities */
/*********************************************************************************************************
  MII Link Code word  bit definitions
*********************************************************************************************************/
#define MII_BP_FAULT                0x2000                              /* remote fault                 */
#define MII_BP_ACK                  0x4000                              /* acknowledge                  */
#define MII_BP_NP                   0x8000                              /* nexp page is supported       */
/*********************************************************************************************************
  MII Expansion Register bit definitions
*********************************************************************************************************/
#define MII_EXP_FAULT               0x0010                              /* parallel detection fault     */
#define MII_EXP_PRTN_NP             0x0008                              /* link partner next-page able  */
#define MII_EXP_LOC_NP              0x0004                              /* local PHY next-page able     */
#define MII_EXP_PR                  0x0002                              /* full page received           */
#define MII_EXP_PRT_AN              0x0001                              /* link partner auto negotiation*/
/*********************************************************************************************************
  MII Link Mode String
*********************************************************************************************************/
#define MII_FDX_STR                 "full duplex"                       /* full duplex mode             */
#define MII_FDX_LEN                 sizeof(MII_FDX_STR)                 /* full duplex mode             */
#define MII_HDX_STR                 "half duplex"                       /* half duplex mode             */
#define MII_HDX_LEN                 sizeof(MII_HDX_STR)                 /* full duplex mode             */
/*********************************************************************************************************
  PHY Speed
*********************************************************************************************************/
#define MII_1000MBS                 1000000000                          /* 1Gbps                        */
#define MII_100MBS                  100000000                           /* 100Mbps                      */
#define MII_10MBS                   10000000                            /* 10Mbps                       */
/*********************************************************************************************************
  Flag Operations
*********************************************************************************************************/
#define MII_PHY_FLAGS_GET()                     (pPhyDev->PHY_uiPhyFlags)
#define MII_PHY_FLAGS_SET(iSetBits)             (pPhyDev->PHY_uiPhyFlags |= (iSetBits))
#define MII_PHY_FLAGS_CLEAR(iClearBits)         (pPhyDev->PHY_uiPhyFlags &= ~(iClearBits))
#define MII_PHY_FLAGS_JUDGE(iSetBits)           (pPhyDev->PHY_uiPhyFlags & (iSetBits))

#define MII_PHY_ABILITY_FLAGS_SET(iSetBits)     (pPhyDev->PHY_uiPhyAbilityFlags |= (iSetBits))
#define MII_PHY_ABILITY_FLAGS_CLEAR(iClearBits) (pPhyDev->PHY_uiPhyAbilityFlags &= ~(iClearBits))
#define MII_PHY_ABILITY_FLAGS_JUDGE(iSetBits)   (pPhyDev->PHY_uiPhyAbilityFlags & (iSetBits))
/*********************************************************************************************************
  PHY Operations
*********************************************************************************************************/
#define MII_READ(phyDev, iRegAddr, pvRegVal)            \
        ((*(phyDev->PHY_pPhyDrvFunc->PHYF_pfuncRead))   \
            ((phyDev), (iRegAddr), (pvRegVal), phyDev->PHY_pPhyDrvFunc))
        
#define MII_WRITE(phyDev, iRegAddr, iRegVal)            \
        ((*(phyDev->PHY_pPhyDrvFunc->PHYF_pfuncWrite))  \
            ((phyDev), (iRegAddr), (iRegVal), phyDev->PHY_pPhyDrvFunc))
/*********************************************************************************************************
  Link check period
*********************************************************************************************************/
#define MII_LINK_CHK_DELAY          2
/*********************************************************************************************************
  Data Structure Definition
*********************************************************************************************************/

typedef struct phy_drv_func {
    FUNCPTR             PHYF_pfuncRead;                                 /* phy read function            */
    FUNCPTR             PHYF_pfuncWrite;                                /* phy write function           */
    
    FUNCPTR             PHYF_pfuncLinkDown;                             /* phy status down function     */
    FUNCPTR             PHYF_pfuncLinkSetHook;                          /* mii phy link set hook func   */
} PHY_DRV_FUNC;

typedef struct phy_dev {
    LW_LIST_LINE        PHY_node;                                       /*  Device Header               */
    PHY_DRV_FUNC       *PHY_pPhyDrvFunc;
    VOID               *PHY_pvMacDrv;                                   /*  Mother Mac Driver Control   */
    
    UINT32              PHY_uiPhyFlags;                                 /*  PHY flag bits               */
    UINT32              PHY_uiPhyAbilityFlags;                          /*  PHY flag bits               */
    UINT32              PHY_uiPhyANFlags;                               /*  Auto Negotiation flags      */
    UINT32              PHY_uiPhyLinkMethod;                            /*  Whether to force link mode  */
    UINT32              PHY_uiLinkDelay;                                /*  Delay time to wait for Link */
    UINT32              PHY_uiSpinDelay;                                /*  Delay time of Spinning Reg  */
    UINT32              PHY_uiTryMax;                                   /*  Max Try times               */
    UINT16              PHY_usPhyStatus;                                /*  Record Status of PHY        */
    UINT8               PHY_ucPhyAddr;                                  /*  Address of a PHY            */
    UINT32              PHY_uiPhyID;                                    /*  Phy ID                      */
    UINT32              PHY_uiPhyIDMask;                                /*  Phy ID MASK                 */
    UINT32              PHY_uiPhySpeed;
    CHAR                PHY_pcPhyMode[16];                              /*  Link Mode description       */
} PHY_DEV;

/*********************************************************************************************************
  External API
*********************************************************************************************************/

LW_API INT API_MiiPhyInit(PHY_DEV *pPhyDev);
LW_API INT API_MiiPhyLinkSet(PHY_DEV *pPhyDev);
LW_API INT API_MiiPhyModeSet(PHY_DEV *pPhyDev);
LW_API INT API_MiiPhyProbe(PHY_DEV *pPhyDev);
LW_API INT API_MiiPhyDiagnostic(PHY_DEV *pPhyDev);
LW_API INT API_MiiPhyScan(PHY_DEV *pPhyDev);
LW_API INT API_MiiPhyAdd(PHY_DEV *pPhyDev);
LW_API INT API_MiiPhyDel(PHY_DEV *pPhyDev);
LW_API INT API_MiiPhyLinkStatGet(PHY_DEV *pPhyDev);
LW_API INT API_MiiPhyIsolate(PHY_DEV *pPhyDev);

LW_API INT API_MiiLibInit(VOID);
LW_API INT API_MiiPhyMonitorStop(VOID);
LW_API INT API_MiiPhyMonitorStart(VOID);

#define miiPhyInit                  API_MiiPhyInit
#define miiPhyLinkSet               API_MiiPhyLinkSet
#define miiPhyModeSet               API_MiiPhyModeSet
#define miiPhyProbe                 API_MiiPhyProbe
#define miiPhyDiagnostic            API_MiiPhyDiagnostic
#define miiPhyScan                  API_MiiPhyScan
#define miiPhyScanOnly              API_MiiPhyScanOnly
#define miiPhyAdd                   API_MiiPhyAdd
#define miiPhyDel                   API_MiiPhyDel
#define miiPhyLinkStatGet           API_MiiPhyLinkStatGet
#define miiPhyIsolate               API_MiiPhyIsolate
#define miiLibInit                  API_MiiLibInit
#define miiPhyMonitorStop           API_MiiPhyMonitorStop
#define miiPhyMonitorStart          API_MiiPhyMonitorStart

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_NET_EN > 0)         */
#endif                                                                  /*  __MII_DEV_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
