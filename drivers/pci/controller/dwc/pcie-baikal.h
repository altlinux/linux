/*
 * pcie-baikal - PCIe controller driver for Baikal SoCs
 *
 * Copyright (C) 2019 Baikal Electronics JSC
 * Author: Pavel Parkhomenko <Pavel.Parkhomenko@baikalelectronics.ru>
 *
 * Parts of this file were based on sources as follows:
 * Copyright (C) 2013-2014 Texas Instruments Incorporated - http://www.ti.com
 * Author: Kishon Vijay Abraham I <kishon@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define PCI_VENDOR_ID_BAIKAL	0x1d39

#define PCIE_GEN2_CTRL_OFF			(0x80c)	/* Link Width and Speed Change Control Register. */

/* PHY control registers. */
#define PCIE_PHY_DWC_GLBL_PLL_CFG_0		(0x1c000)	/* PLL Global Configuration Register #0 */
#define PCIE_PHY_DWC_GLBL_PLL_CFG_1		(0x1c001)	/* PLL Global Configuration Register #1 */
#define PCIE_PHY_DWC_GLBL_PLL_CFG_2		(0x1c002)	/* PLL Global Configuration Register #2 */
#define PCIE_PHY_DWC_GLBL_PLL_CFG_3		(0x1c003)	/* PLL Global Configuration Register #3 */
#define PCIE_PHY_DWC_GLBL_PLL_CFG_4		(0x1c004)	/* PLL Global Configuration Register #4 */
#define PCIE_PHY_DWC_GLBL_MISC_CONFIG_0		(0x1c005)	/* Global Miscellaneous Configuration #0 */
#define PCIE_PHY_DWC_GLBL_MISC_CONFIG_1		(0x1c006)	/* Global Miscellaneous Configuration #1 */
#define PCIE_PHY_DWC_SLICE_CFG			(0x1c00c)	/* Slice Configuration */
#define PCIE_PHY_DWC_GLBL_REGU_CFG		(0x1c00d)	/* Global Regulator Configuration */
#define PCIE_PHY_DWC_GLBL_TERM_CFG		(0x1c00e)	/* Global Termination Calibration Configuration */
#define PCIE_PHY_DWC_GLBL_CAL_CFG		(0x1c00f)	/* Global PLL Calibration Configuration */
#define PCIE_PHY_DWC_GLBL_RD_SYNC_STATUS	(0x1c010)	/* Global Read Synchronization Status */
#define PCIE_PHY_DWC_RX_PWR_CTRL_P0		(0x1c014)	/* RX Power Controls in Power State P0 */
#define PCIE_PHY_DWC_RX_PWR_CTRL_P0S		(0x1c015)	/* RX Power Controls in Power State P0S */
#define PCIE_PHY_DWC_RX_PWR_CTRL_P1		(0x1c016)	/* RX Power Controls in Power State P1 */
#define PCIE_PHY_DWC_RX_PWR_CTRL_P2		(0x1c017)	/* RX Power Controls in Power State P2 */
#define PCIE_PHY_DWC_TX_PWR_CTRL_P0_P0S		(0x1c018)	/* TX Power Controls in Power States P0 and POS */
#define PCIE_PHY_DWC_TX_PWR_CTRL_P1_P2		(0x1c019)	/* TX Power Controls in Power States P1 and P2 */
#define PCIE_PHY_DWC_GLBL_PWR_CTRL		(0x1c01a)	/* Global Power State Machine Control Override */
#define PCIE_PHY_DWC_RX_TXDIR_CTRL_0		(0x1c01d)	/* Far-end TX Direction Control Register #0 */
#define PCIE_PHY_DWC_RX_TXDIR_CTRL_1		(0x1c01e)	/* Far-end TX Direction Control Register #1 */
#define PCIE_PHY_DWC_RX_TXDIR_CTRL_2		(0x1c01f)	/* Far-end TX Direction Control Register #2 */
#define PCIE_PHY_DWC_GLBL_PLL_MONITOR		(0x1c020)	/* Monitor for SerDes Global to Raw PCS Global Interface */
#define PCIE_PHY_DWC_GLBL_TERM_MON_1		(0x1c022)	/* Monitor for SerDes Global to Raw PCS Global Interface */
#define PCIE_PHY_DWC_GLBL_SDS_PIN_MON_0		(0x1c023)	/* Monitor for Raw PCS Global to SerDes Global to Raw PCS Interface */
#define PCIE_PHY_DWC_GLBL_SDS_PIN_MON_1		(0x1c024)	/* Monitor for Raw PCS Global to SerDes Global to Raw PCS Interface */
#define PCIE_PHY_DWC_GLBL_PWR_MON_0		(0x1c025)	/* Monitor of Global Power State Machine Values */
#define PCIE_PHY_DWC_GLBL_PWR_MON_1		(0x1c026)	/* Monitor of Global Power State Machine Values */
#define PCIE_PHY_DWC_GLBL_PWR_MON_2		(0x1c027)	/* Monitor of Global Power State Machine Values */
#define PCIE_PHY_DWC_GLBL_PLL_SSC_FRAC_BASE	(0x1c060)	/* Global PLL SSC Fractional Base */
#define PCIE_PHY_DWC_GLBL_PLL_SSC_CYCLES	(0x1c061)	/* Global PLL SSC Cycles Configuration */
#define PCIE_PHY_DWC_GLBL_PLL_SSC_FMFREQ	(0x1c062)	/* Global PLL SSC Modulation Frequency */
#define PCIE_PHY_DWC_GLBL_PLL_SSC_FREF		(0x1c063)	/* Global PLL SSC Reference Frequency */
#define PCIE_PHY_DWC_GLBL_PLL_SSC_PPM		(0x1c064)	/* Global PLL SSC PPM */
#define PCIE_PHY_DWC_GLBL_PLL_SSC_CFG		(0x1c065)	/* Global PLL SSC Configuration */
#define PCIE_PHY_DWC_GLBL_PLL_SSC_ALU_CMD	(0x1c067)	/* Global PLL SSC ALU Command */
#define PCIE_PHY_DWC_GLBL_PLL_SSC_MON		(0x1c069)	/* Global PLL SSC Monitor */
#define PCIE_PHY_DWC_GLBL_PLL_SSC_ALU_OUT_0	(0x1c06b)	/* Global PLL SSC ALU Output Register #0 */
#define PCIE_PHY_DWC_GLBL_PLL_SSC_ALU_OUT_1	(0x1c06c)	/* Global PLL SSC ALU Output Register #1 */
#define PCIE_PHY_DWC_GLBL_PLL_SSC_DIV		(0x1c06d)	/* Global PLL SSC Divider */
#define PCIE_PHY_DWC_GLBL_PLL_SSC_FRAC		(0x1c06e)	/* Global PLL SSC Fraction */
#define PCIE_PHY_DWC_GLBL_TAD			(0x1c080)	/* Global Test Analog and Digital Monitor */
#define PCIE_PHY_DWC_GLBL_TM_ADMON		(0x1c081)	/* Global Test Mode Analog/Digital Monitor Enable */
#define PCIE_PHY_DWC_EQ_WAIT_TIME		(0x3c000)	/* TX and RX Equalization Wait Times */
#define PCIE_PHY_DWC_RDET_TIME			(0x3c001)	/* Receiver Detect Wait Times */
#define PCIE_PHY_DWC_PCS_LANE_LINK_CFG		(0x3c002)	/* Link Configuration Override */
#define PCIE_PHY_DWC_PCS_PLL_CTLIFC_0		(0x3c003)	/* PLL Control Interface Override Register #0 */
#define PCIE_PHY_DWC_PCS_PLL_CTLIFC_1		(0x3c004)	/* PLL Control Interface Override Register #1 */
#define PCIE_PHY_DWC_PCS_REG_RD_TIMEOUT		(0x3c005)	/* Register Read Timeout */
#define PCIE_PHY_DWC_PCS_PLL_PCIE1_MODE_0	(0x3c006)	/* PLL Configuration Register #0 for PCIe1 */
#define PCIE_PHY_DWC_PCS_PLL_PCIE1_MODE_1	(0x3c007)	/* PLL Configuration Register #1 for PCIe1 */
#define PCIE_PHY_DWC_PCS_LANE_PCIE1_MODE_0	(0x3c008)	/* Lane Configuration Register #0 for PCIe1 */
#define PCIE_PHY_DWC_PCS_LANE_PCIE1_MODE_1	(0x3c009)	/* Lane Configuration Register #1 for PCIe1 */
#define PCIE_PHY_DWC_PCS_PLL_PCIE2_MODE_0	(0x3c00a)	/* PLL Configuration Register #0 for PCIe2 */
#define PCIE_PHY_DWC_PCS_PLL_PCIE2_MODE_1	(0x3c00b)	/* PLL Configuration Register #1 for PCIe2 */
#define PCIE_PHY_DWC_PCS_LANE_PCIE2_MODE_0	(0x3c00c)	/* Lane Configuration Register #0 for PCIe2 */
#define PCIE_PHY_DWC_PCS_LANE_PCIE2_MODE_1	(0x3c00d)	/* Lane Configuration Register #1 for PCIe2 */
#define PCIE_PHY_DWC_PCS_PLL_PCIE3_MODE_0	(0x3c00e)	/* PLL Configuration Register #0 for PCIe3 */
#define PCIE_PHY_DWC_PCS_PLL_PCIE3_MODE_1	(0x3c00f)	/* PLL Configuration Register #1 for PCIe3 */
#define PCIE_PHY_DWC_PCS_LANE_PCIE3_MODE_0	(0x3c010)	/* Lane Configuration Register #0 for PCIe3 */
#define PCIE_PHY_DWC_PCS_LANE_PCIE3_MODE_1	(0x3c011)	/* Lane Configuration Register #1 for PCIe3 */
#define PCIE_PHY_DWC_PCS_PLL_KX_MODE_1		(0x3c013)	/* PLL Configuration Register #1 for KX */
#define PCIE_PHY_DWC_PCS_LANE_KX_MODE_0		(0x3c014)	/* Lane Configuration Register #0 for KX */
#define PCIE_PHY_DWC_PCS_LANE_KX_MODE_1		(0x3c015)	/* Lane Configuration Register #1 for KX */
#define PCIE_PHY_DWC_PCS_PLL_KX4_MODE_0		(0x3c016)	/* PLL Configuration Register #0 for KX4 */
#define PCIE_PHY_DWC_PCS_PLL_KX4_MODE_1		(0x3c017)	/* PLL Configuration Register #1 for KX4 */
#define PCIE_PHY_DWC_PCS_LANE_KX4_MODE_0	(0x3c018)	/* Lane Configuration Register #0 for KX4 */
#define PCIE_PHY_DWC_PCS_LANE_KX4_MODE_1	(0x3c019)	/* Lane Configuration Register #1 for KX4 */
#define PCIE_PHY_DWC_PCS_PLL_KR_MODE_0		(0x3c01a)	/* PLL Configuration Register #0 for KR */
#define PCIE_PHY_DWC_PCS_PLL_KR_MODE_1		(0x3c01b)	/* PLL Configuration Register #1 for KR */
#define PCIE_PHY_DWC_PCS_LANE_KR_MODE_0		(0x3c01c)	/* Lane Configuration Register #0 for KR */
#define PCIE_PHY_DWC_PCS_LANE_KR_MODE_1		(0x3c01d)	/* Lane Configuration Register #1 for KR */
#define PCIE_PHY_DWC_PCS_PLL_SGMII_MODE_0	(0x3c01e)	/* PLL Configuration Register #0 for SGMII */
#define PCIE_PHY_DWC_PCS_PLL_SGMII_MODE_1	(0x3c01f)	/* PLL Configuration Register #1 for SGMII */
#define PCIE_PHY_DWC_PCS_LANE_SGMII_MODE_0	(0x3c020)	/* Lane Configuration Register #0 for SGMII */
#define PCIE_PHY_DWC_PCS_LANE_SGMII_MODE_1	(0x3c021)	/* Lane Configuration Register #1 for SGMII */
#define PCIE_PHY_DWC_PCS_PLL_QSGMII_MODE_0	(0x3c022)	/* PLL Configuration Register #0 for QSGMII */
#define PCIE_PHY_DWC_PCS_PLL_QSGMII_MODE_1	(0x3c023)	/* PLL Configuration Register #1 for QSGMII */
#define PCIE_PHY_DWC_PCS_LANE_QSGMII_MODE_0	(0x3c024)	/* Lane Configuration Register #0 for QSGMII */
#define PCIE_PHY_DWC_PCS_LANE_QSGMII_MODE_1	(0x3c025)	/* Lane Configuration Register #1 for QSGMII */
#define PCIE_PHY_DWC_PCS_PLL_CEI_MODE_0		(0x3c026)	/* PLL Configuration Register #0 for CEI */
#define PCIE_PHY_DWC_PCS_PLL_CEI_MODE_1		(0x3c027)	/* PLL Configuration Register #1 for CEI */
#define PCIE_PHY_DWC_PCS_LANE_CEI_MODE_0		(0x3c028)	/* Lane Configuration Register #0 for CEI */
#define PCIE_PHY_DWC_PCS_LANE_CEI_MODE_1		(0x3c029)	/* Lane Configuration Register #1 for CEI */
#define PCIE_PHY_DWC_PCS_PLL_PCIE1_125M_MODE_0		(0x3c02a)	/* PLL Configuration Register #0 for PCIe1 with 125MHz refclk */
#define PCIE_PHY_DWC_PCS_PLL_PCIE1_125M_MODE_1		(0x3c02b)	/* PLL Configuration Register #1 for PCIe1 with 125MHz refclk */
#define PCIE_PHY_DWC_PCS_LANE_PCIE1_125M_MODE_0		(0x3c02c)	/* Lane Configuration Register #0 for PCIe1 with 125MHz refclk */
#define PCIE_PHY_DWC_PCS_LANE_PCIE1_125M_MODE_1		(0x3c02d)	/* Lane Configuration Register #1 for PCIe1 with 125MHz refclk */
#define PCIE_PHY_DWC_PCS_PLL_PCIE2_125M_MODE_0		(0x3c02e)	/* PLL Configuration Register #0 for PCIe2 with 125MHz refclk */
#define PCIE_PHY_DWC_PCS_PLL_PCIE2_125M_MODE_1		(0x3c02f)	/* PLL Configuration Register #1 for PCIe2 with 125MHz refclk */
#define PCIE_PHY_DWC_PCS_LANE_PCIE2_125M_MODE_0		(0x3c030)	/* Lane Configuration Register #0 for PCIe2 with 125MHz refclk */
#define PCIE_PHY_DWC_PCS_LANE_PCIE2_125M_MODE_1		(0x3c031)	/* Lane Configuration Register #1 for PCIe2 with 125MHz refclk */
#define PCIE_PHY_DWC_PCS_PLL_PCIE3_125M_MODE_0		(0x3c032)	/* PLL Configuration Register #0 for PCIe3 with 125MHz refclk */
#define PCIE_PHY_DWC_PCS_PLL_PCIE3_125M_MODE_1		(0x3c033)	/* PLL Configuration Register #1 for PCIe3 with 125MHz refclk */
#define PCIE_PHY_DWC_PCS_LANE_PCIE3_125M_MODE_0		(0x3c034)	/* Lane Configuration Register #0 for PCIe3 with 125MHz refclk */
#define PCIE_PHY_DWC_PCS_LANE_PCIE3_125M_MODE_1		(0x3c035)	/* Lane Configuration Register #1 for PCIe3 with 125MHz refclk */
#define PCIE_PHY_DWC_PCS_LANE_VMA_COARSE_CTRL_0		(0x3c036)	/* Lane VMA Coarse Control Register #0 */
#define PCIE_PHY_DWC_PCS_LANE_VMA_COARSE_CTRL_1		(0x3c037)	/* Lane VMA Coarse Control Register #1 */
#define PCIE_PHY_DWC_PCS_LANE_VMA_COARSE_CTRL_2		(0x3c038)	/* Lane VMA Coarse Control Register #2 */
#define PCIE_PHY_DWC_PCS_LANE_VMA_FINE_CTRL_0		(0x3c039)	/* Lane VMA Fine Control Register #0 */
#define PCIE_PHY_DWC_PCS_LANE_VMA_FINE_CTRL_1		(0x3c03a)	/* Lane VMA Fine Control Register #1 */
#define PCIE_PHY_DWC_PCS_LANE_VMA_FINE_CTRL_2		(0x3c03b)	/* Lane VMA Fine Control Register #2 */
#define PCIE_PHY_DWC_PCS_LANE_MODE_OVRD			(0x3c03c)	/* Lane Mode Override in Raw PCS Global and Slice */
#define PCIE_PHY_DWC_PCS_LANE_LINK_MON			(0x3c040)	/* Monitor of MAC to Raw PCS Link Configuration Interface */
#define PCIE_PHY_DWC_PCS_MAC_PLLIFC_MON_2		(0x3c043)	/* Monitor of MAC to Raw PCS PLL_PCS Divider Value */
#define PCIE_PHY_DWC_PCS_MAC_PLLIFC_MON_3		(0x3c044)	/* Monitor of MAC to Raw PCS PLL OP_Range and Divider Values */
#define PCIE_PHY_DWC_SLICE_TRIM			(0x1c040)	/* Slice TX and RX Bias Trim Settings */
#define PCIE_PHY_DWC_RX_LDLL_CTRL		(0x1c043)	/* RX Lane DLL Test Controls */
#define PCIE_PHY_DWC_RX_SDLL_CTRL		(0x1c044)	/* RX Slice DLL test controls */
#define PCIE_PHY_DWC_SLICE_PCIE1_MODE		(0x1c045)	/* Slice Configuration Settings for PCIE1 @ 100MHz */
#define PCIE_PHY_DWC_SLICE_PCIE2_MODE		(0x1c046)	/* Slice Configuration Settings for PCIE2 @ 100Mhz */
#define PCIE_PHY_DWC_SLICE_PCIE3_MODE		(0x1c047)	/* Slice Configuration Settings for PCIE3 @ 100Mhz */
#define PCIE_PHY_DWC_SLICE_KX_MODE		(0x1c048)	/* Slice Configuration Settings for KX */
#define PCIE_PHY_DWC_SLICE_KX4_MODE		(0x1c049)	/* Slice Configuration Settings for KX4 */
#define PCIE_PHY_DWC_SLICE_KR_MODE		(0x1c04a)	/* Slice Configuration Settings for KR */
#define PCIE_PHY_DWC_SLICE_SGMII_MODE		(0x1c04b)	/* Slice Configuration Settings for SGMII */
#define PCIE_PHY_DWC_SLICE_QSGMII_MODE		(0x1c04c)	/* Slice Configuration Settings for QSGMII */
#define PCIE_PHY_DWC_SLICE_CEI_MODE		(0x1c04d)	/* Slice Configuration Settings for CEI */
#define PCIE_PHY_DWC_SLICE_PCIE1_125M_MODE	(0x1c04e)	/* Slice Configuration Settings for PCIE1 @ 125MHz */
#define PCIE_PHY_DWC_SLICE_PCIE2_125M_MODE	(0x1c04f)	/* Slice Configuration Settings for PCIE2 @ 125MHz */
#define PCIE_PHY_DWC_SLICE_PCIE3_125M_MODE	(0x1c050)	/* Slice Configuration Settings for PCIE3 @ 125MHz */
#define PCIE_PHY_DWC_SLICE_OVRD_MODE		(0x1c051)	/* Slice Configuration Settings Override */
#define PCIE_PHY_DWC_RX_CFG_0			(0x18000)	/* Lane RX Configuration Register #0 */
#define PCIE_PHY_DWC_RX_CFG_1			(0x18001)	/* Lane RX Configuration Register #1 */
#define PCIE_PHY_DWC_RX_CFG_2			(0x18002)	/* Lane RX Configuration Register #2 */
#define PCIE_PHY_DWC_RX_CFG_3			(0x18003)	/* Lane RX Configuration Register #3 */
#define PCIE_PHY_DWC_RX_CFG_4			(0x18004)	/* Lane RX Configuration Register #4 */
#define PCIE_PHY_DWC_RX_CFG_5			(0x18005)	/* Lane RX Configuration Register #5 */
#define PCIE_PHY_DWC_RX_CDR_CTRL_0		(0x18006)	/* Lane RX CDR Control Register #0 */
#define PCIE_PHY_DWC_RX_CDR_CTRL_1		(0x18007)	/* Lane RX CDR Control Register #1 */
#define PCIE_PHY_DWC_RX_CDR_CTRL_2		(0x18008)	/* Lane RX CDR Control Register #2 */
#define PCIE_PHY_DWC_RX_LOOP_CTRL		(0x18009)	/* Lane RX Loop Control */
#define PCIE_PHY_DWC_RX_MISC_CTRL		(0x1800a)	/* Lane RX Miscellaneous Control */
#define PCIE_PHY_DWC_RX_CTLE_CTRL		(0x1800b)	/* Lane RX CTLE Control */
#define PCIE_PHY_DWC_RX_PRECORR_CTRL		(0x1800c)	/* Lane RX Pre-Correlation Control */
#define PCIE_PHY_DWC_RX_PHS_ACCM_CTRL		(0x1800d)	/* Lane RX Phase Accumulator Control */
#define PCIE_PHY_DWC_RX_PHS_ACCM_FR_VAL		(0x1800e)	/* Lane RX Phase Accumulator Frequency Portion Control */
#define PCIE_PHY_DWC_RX_PRECORR_VAL		(0x1800f)	/* Lane RX Pre-Correlation Count */
#define PCIE_PHY_DWC_RX_DELTA_PM_0		(0x18010)	/* Lane RX VMA Performance Metric Register #0 */
#define PCIE_PHY_DWC_RX_DELTA_PM_1		(0x18011)	/* Lane RX VMA Performance Metric Register #1 */
#define PCIE_PHY_DWC_TX_CAPT_CTRL		(0x18012)	/* Lane TX Latch Control */
#define PCIE_PHY_DWC_TX_CFG_0			(0x18015)	/* Lane TX Configuration Register #0 */
#define PCIE_PHY_DWC_TX_CFG_1			(0x18016)	/* Lane TX Configuration Register #1 */
#define PCIE_PHY_DWC_TX_CFG_2			(0x18017)	/* Lane TX Configuration Register #2 */
#define PCIE_PHY_DWC_TX_CFG_3			(0x18018)	/* Lane TX Configuration Register #3 */
#define PCIE_PHY_DWC_TX_PREEMPH_0		(0x18019)	/* Lane TX Pre-Emphasis */
#define PCIE_PHY_DWC_PMA_LOOPBACK_CTRL		(0x1801a)	/* Lane PMA Loopback Control */
#define PCIE_PHY_DWC_LANE_PWR_CTRL		(0x1801b)	/* Lane Power Control */
#define PCIE_PHY_DWC_TERM_CTRL			(0x1801c)	/* Lane Termination Control */
#define PCIE_PHY_DWC_RX_MISC_STATUS		(0x18025)	/* RX Miscellaneous Status */
#define PCIE_PHY_DWC_SDS_PIN_MON_0		(0x18026)	/* SerDes Pin Monitor 0 */
#define PCIE_PHY_DWC_SDS_PIN_MON_1		(0x18027)	/* SerDes Pin Monitor 1 */
#define PCIE_PHY_DWC_SDS_PIN_MON_2		(0x18028)	/* SerDes Pin Monitor 2 */
#define PCIE_PHY_DWC_RX_PWR_MON_0		(0x18029)	/* RX Power State Machine Monitor 0 */
#define PCIE_PHY_DWC_RX_PWR_MON_1		(0x1802a)	/* RX Power State Machine Monitor 1 */
#define PCIE_PHY_DWC_RX_PWR_MON_2		(0x1802b)	/* RX Power State Machine Monitor 2 */
#define PCIE_PHY_DWC_TX_PWR_MON_0		(0x1802c)	/* TX Power State Machine Monitor 0 */
#define PCIE_PHY_DWC_TX_PWR_MON_1		(0x1802d)	/* TX Power State Machine Monitor 1 */
#define PCIE_PHY_DWC_TX_PWR_MON_2		(0x1802e)	/* TX Power State Machine Monitor 2 */
#define PCIE_PHY_DWC_RX_VMA_CTRL		(0x18040)	/* Lane RX VMA Control */
#define PCIE_PHY_DWC_RX_CDR_MISC_CTRL_0		(0x18041)	/* Lane RX CDR Miscellaneous Control Register #0 */
#define PCIE_PHY_DWC_RX_CDR_MISC_CTRL_1		(0x18042)	/* Lane RX CDR Miscellaneous Control Register #1 */
#define PCIE_PHY_DWC_RX_PWR_CTRL		(0x18043)	/* Lane RX Power Control */
#define PCIE_PHY_DWC_RX_OS_MVALBBD_0		(0x18045)	/* Lane RX Offset Calibration Manual Control Register #0 */
#define PCIE_PHY_DWC_RX_OS_MVALBBD_1		(0x18046)	/* Lane RX Offset Calibration Manual Control Register #1 */
#define PCIE_PHY_DWC_RX_OS_MVALBBD_2		(0x18047)	/* Lane RX Offset Calibration Manual Control Register #2 */
#define PCIE_PHY_DWC_RX_AEQ_VALBBD_0		(0x18048)	/* Lane RX Adaptive Equalizer Control Register #0 */
#define PCIE_PHY_DWC_RX_AEQ_VALBBD_1		(0x18049)	/* Lane RX Adaptive Equalizer Control Register #1 */
#define PCIE_PHY_DWC_RX_AEQ_VALBBD_2		(0x1804a)	/* Lane RX Adaptive Equalizer Control Register #2 */
#define PCIE_PHY_DWC_RX_MISC_OVRRD		(0x1804b)	/* Lane RX Miscellaneous Override Controls */
#define PCIE_PHY_DWC_RX_OVRRD_PHASE_ACCUM_ADJ	(0x1804c)	/* Lane RX Phase Accumulator Adjust Override */
#define PCIE_PHY_DWC_RX_AEQ_OUT_0		(0x18050)	/* Lane RX Adaptive Equalizer Status Register #0 */
#define PCIE_PHY_DWC_RX_AEQ_OUT_1		(0x18051)	/* Lane RX Adaptive Equalizer Status Register #1 */
#define PCIE_PHY_DWC_RX_AEQ_OUT_2		(0x18052)	/* Lane RX Adaptive Equalizer Status Register #2 */
#define PCIE_PHY_DWC_RX_OS_OUT_0		(0x18053)	/* Lane RX Offset Calibration Status Register #0 */
#define PCIE_PHY_DWC_RX_OS_OUT_1		(0x18054)	/* Lane RX Offset Calibration Status Register #1 */
#define PCIE_PHY_DWC_RX_OS_OUT_2		(0x18055)	/* Lane RX Offset Calibration Status Register #2 */
#define PCIE_PHY_DWC_RX_OS_OUT_3		(0x18056)	/* Lane RX Offset Calibration Status Register #3 */
#define PCIE_PHY_DWC_RX_VMA_STATUS_0		(0x18057)	/* Lane RX CDR Status Register #0 */
#define PCIE_PHY_DWC_RX_VMA_STATUS_1		(0x18058)	/* Lane RX CDR Status Register #1 */
#define PCIE_PHY_DWC_RX_CDR_STATUS_0		(0x18059)	/* Lane RX CDR Status Register #0 */
#define PCIE_PHY_DWC_RX_CDR_STATUS_1		(0x1805a)	/* Lane RX CDR Status Register #1 */
#define PCIE_PHY_DWC_RX_CDR_STATUS_2		(0x1805b)	/* Lane RX CDR Status Register #2 */
#define PCIE_PHY_DWC_PCS_MISC_CFG_0		(0x38000)	/* Lane Miscellaneous Configuration Register #0 */
#define PCIE_PHY_DWC_PCS_MISC_CFG_1		(0x38001)	/* Lane Raw PCS Miscellaneous Configuration Register #1 */
#define PCIE_PHY_DWC_PCS_LBERT_PAT_CFG		(0x38003)	/* LBERT Pattern Configuration */
#define PCIE_PHY_DWC_PCS_LBERT_CFG		(0x38004)	/* LBERT Configuration */
#define PCIE_PHY_DWC_PCS_LBERT_ECNT		(0x38005)	/* LBERT Error Counter */
#define PCIE_PHY_DWC_PCS_RESET_0		(0x38006)	/* Lane Raw PCS Reset Register #0 */
#define PCIE_PHY_DWC_PCS_RESET_1		(0x38007)	/* Lane Raw PCS Reset Register #1 */
#define PCIE_PHY_DWC_PCS_RESET_2		(0x38008)	/* Lane Raw PCS Reset Register #2 */
#define PCIE_PHY_DWC_PCS_RESET_3		(0x38009)	/* Lane Raw PCS Reset Register #3 */
#define PCIE_PHY_DWC_PCS_CTLIFC_CTRL_0		(0x3800c)	/* Lane Raw PCS Control Interface Configuration Register #0 */
#define PCIE_PHY_DWC_PCS_CTLIFC_CTRL_1		(0x3800d)	/* Lane Raw PCS Control Interface Configuration Register #1 */
#define PCIE_PHY_DWC_PCS_CTLIFC_CTRL_2		(0x3800e)	/* Lane Raw PCS Control Interface Configuration Register #2 */
#define PCIE_PHY_DWC_PCS_MACIFC_MON_0		(0x38021)	/* MAC to Raw PCS Interface Monitor Register #0 */
#define PCIE_PHY_DWC_PCS_MACIFC_MON_2		(0x38023)	/* MAC to Raw PCS Interface Monitor Register #1 */
