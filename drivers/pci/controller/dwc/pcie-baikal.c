/*
 * PCIe root complex driver for Baikal SoCs
 *
 * Copyright (C) 2019-2020 Baikal Electronics, JSC
 * Authors: Pavel Parkhomenko <pavel.parkhomenko@baikalelectronics.ru>
 *          Mikhail Ivanov <michail.ivanov@baikalelectronics.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/irqchip/arm-gic-v3.h>
#include <linux/mfd/baikal/lcru-pcie.h>
#include <linux/mfd/syscon.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/pci.h>
#include <linux/phy/phy.h>

#include "pcie-designware.h"

#ifdef CONFIG_PCI_DEBUG
#define DEBUG
#endif

struct baikal_pcie {
	struct dw_pcie pp;
	unsigned bus_nr;
	struct regmap *lcru;
	struct gpio_desc *reset_gpio;
	unsigned reset_active_low;
	char reset_name[32];
	unsigned retrained;
	unsigned num_lanes;
};

#define to_baikal_pcie(x)	container_of((x), struct baikal_pcie, pp)

#define PCIE_DEVICE_CONTROL_DEVICE_STATUS_REG	0x78
#define PCIE_CAP_CORR_ERR_REPORT_EN		BIT(0)
#define PCIE_CAP_NON_FATAL_ERR_REPORT_EN	BIT(1)
#define PCIE_CAP_FATAL_ERR_REPORT_EN		BIT(2)
#define PCIE_CAP_UNSUPPORT_REQ_REP_EN		BIT(3)

#define PCIE_LINK_CAPABILITIES_REG		0x7c
#define PCIE_CAP_MAX_LINK_WIDTH_MASK		0x3f0
#define PCIE_CAP_MAX_LINK_WIDTH_SHIFT		4

#define PCIE_LINK_CONTROL_LINK_STATUS_REG	0x80
#define PCIE_CAP_LINK_SPEED_MASK		0xf0000
#define PCIE_CAP_LINK_SPEED_SHIFT		16
#define PCIE_CAP_NEGO_LINK_WIDTH_MASK		0x3f00000
#define PCIE_CAP_NEGO_LINK_WIDTH_SHIFT		20
#define PCIE_CAP_LINK_TRAINING			BIT(27)

#define PCIE_ROOT_CONTROL_ROOT_CAPABILITIES_REG	0x8c
#define PCIE_CAP_SYS_ERR_ON_CORR_ERR_EN		BIT(0)
#define PCIE_CAP_SYS_ERR_ON_NON_FATAL_ERR_EN	BIT(1)
#define PCIE_CAP_SYS_ERR_ON_FATAL_ERR_EN	BIT(2)
#define PCIE_CAP_PME_INT_EN			BIT(3)

#define PCIE_LINK_CONTROL2_LINK_STATUS2_REG	0xa0
#define PCIE_CAP_TARGET_LINK_SPEED_MASK		0xf

#define PCIE_UNCORR_ERR_STATUS_REG		0x104
#define PCIE_CORR_ERR_STATUS_REG		0x110

#define PCIE_ROOT_ERR_CMD_REG			0x12c
#define PCIE_CORR_ERR_REPORTING_EN		BIT(0)
#define PCIE_NON_FATAL_ERR_REPORTING_EN		BIT(1)
#define PCIE_FATAL_ERR_REPORTING_EN		BIT(2)

#define PCIE_ROOT_ERR_STATUS_REG		0x130

#define PCIE_GEN2_CTRL_REG			0x80c
#define PCIE_DIRECT_SPEED_CHANGE		BIT(17)

#define PCIE_MSI_CTRL_ADDR_LO_REG		0x820
#define PCIE_MSI_CTRL_ADDR_HI_REG		0x824

#define PCIE_MISC_CONTROL_1_REG			0x8bc
#define PCIE_DBI_RO_RW_EN			BIT(0)

#define PCIE_IATU_VIEWPORT_REG			0x900
#define PCIE_IATU_REGION_INBOUND		BIT(31)
#define PCIE_IATU_REGION_OUTBOUND		0
#define PCIE_IATU_REGION_CTRL_2_REG		0x908

static const struct of_device_id of_baikal_pcie_match[] = {
	{ .compatible = "baikal,pcie-m", },
	{},
};

static unsigned baikal_pcie_link_is_training(struct dw_pcie *pp)
{
	unsigned long reg;
	reg = dw_pcie_readl_dbi(pp, PCIE_LINK_CONTROL_LINK_STATUS_REG);
	return reg & PCIE_CAP_LINK_TRAINING;
}

static bool baikal_wait_pcie_link_training_done(struct dw_pcie *pp)
{
	unsigned long start_jiffies = jiffies;
	while (baikal_pcie_link_is_training(pp)) {
		if (time_after(jiffies, start_jiffies + HZ)) {
			pr_err("%s: link retrained for too long, timeout occured\n", __func__);
			return false;
		}
		udelay(100);
	}
	return true;
}

static void baikal_print_link_status(struct dw_pcie *pp)
{
	unsigned long reg;
	unsigned speed, width;

	reg = dw_pcie_readl_dbi(pp, PCIE_LINK_CONTROL_LINK_STATUS_REG);
	speed = (reg & PCIE_CAP_LINK_SPEED_MASK) >> PCIE_CAP_LINK_SPEED_SHIFT;
	width = (reg & PCIE_CAP_NEGO_LINK_WIDTH_MASK) >>
		PCIE_CAP_NEGO_LINK_WIDTH_SHIFT;

	dev_info(pp->dev, "link status is Gen%u, x%u\n", speed, width);
}

static void baikal_pcie_link_retrain(struct dw_pcie *pp, int target_speed)
{
	unsigned long reg;
	unsigned long start_jiffies;

	dev_info(pp->dev, "retrain link to Gen%u\n", target_speed);

	/* In case link is already training wait for training to complete */
	baikal_wait_pcie_link_training_done(pp);

	/* Set desired speed */
	reg = dw_pcie_readl_dbi(pp, PCIE_LINK_CONTROL2_LINK_STATUS2_REG);
	reg &= ~PCIE_CAP_TARGET_LINK_SPEED_MASK;
	reg |= target_speed;
	dw_pcie_writel_dbi(pp, PCIE_LINK_CONTROL2_LINK_STATUS2_REG, reg);

	/* Set DIRECT_SPEED_CHANGE bit */
	reg = dw_pcie_readl_dbi(pp, PCIE_GEN2_CTRL_REG);
	reg &= ~PCIE_DIRECT_SPEED_CHANGE;
	dw_pcie_writel_dbi(pp, PCIE_GEN2_CTRL_REG, reg);
	reg |= PCIE_DIRECT_SPEED_CHANGE;
	dw_pcie_writel_dbi(pp, PCIE_GEN2_CTRL_REG, reg);

	/* Wait for link training begin */
	start_jiffies = jiffies;
	while (baikal_pcie_link_is_training(pp) == 0) {
		if (time_after(jiffies, start_jiffies + HZ)) {
			pr_err("%s: link retrained for too long, timeout occured\n", __func__);
			break;
		}
		udelay(100);
	}

	/* Wait for link training end */
	baikal_wait_pcie_link_training_done(pp);

	if (dw_pcie_wait_for_link(pp) == 0) {
		baikal_print_link_status(pp);
	}
}

static void baikal_pcie_link_speed_fixup(struct pci_dev *pdev)
{
	struct pcie_port *portp = pdev->bus->sysdata;
	struct dw_pcie *pp = to_dw_pcie_from_pp(portp);
	struct baikal_pcie *pcie = to_baikal_pcie(pp);
	unsigned dev_lnkcap_speed;
	unsigned dev_lnkcap_width;
	unsigned rc_lnkcap_speed;
	unsigned rc_lnksta_speed;
	unsigned rc_target_speed;
	u32 reg;

	/* Skip Root Bridge */
	if (!pdev->bus->self) {
		return;
	}

	/* Skip any devices not directly connected to the RC */
	if (pdev->bus->self->bus->number != portp->bridge->bus->number) {
		return;
	}

	/* Skip if the bus has already been retrained */
	if (pcie->retrained) {
		return;
	}

	reg = dw_pcie_readl_dbi(pp, PCIE_LINK_CAPABILITIES_REG);
	rc_lnkcap_speed = reg & PCI_EXP_LNKCAP_SLS;

	reg = dw_pcie_readl_dbi(pp, PCIE_LINK_CONTROL_LINK_STATUS_REG);
	rc_lnksta_speed = (reg & PCIE_CAP_LINK_SPEED_MASK) >>
			  PCIE_CAP_LINK_SPEED_SHIFT;

	rc_target_speed = rc_lnksta_speed < 3? rc_lnksta_speed + 1 : 3;

	pcie_capability_read_dword(pdev, PCI_EXP_LNKCAP, &reg);
	dev_lnkcap_speed = (reg & PCI_EXP_LNKCAP_SLS);
	dev_lnkcap_width = (reg & PCI_EXP_LNKCAP_MLW) >> PCI_EXP_LNKSTA_NLW_SHIFT;

	baikal_print_link_status(pp);
	dev_info(&pdev->dev, "device link capability is Gen%u, x%u\n",
		 dev_lnkcap_speed, dev_lnkcap_width);

	while (rc_target_speed > rc_lnksta_speed &&
	       rc_target_speed <= rc_lnkcap_speed &&
	       rc_target_speed <= dev_lnkcap_speed) {
		/* Try to change link speed */
		baikal_pcie_link_retrain(pp, rc_target_speed);
		reg = dw_pcie_readl_dbi(pp, PCIE_LINK_CONTROL_LINK_STATUS_REG);
		rc_lnksta_speed = (reg & PCIE_CAP_LINK_SPEED_MASK) >>
				  PCIE_CAP_LINK_SPEED_SHIFT;

		/* Check if the targeted speed has not been reached */
		if (rc_lnksta_speed < rc_target_speed) {
			/* Check if the retraining has led to speed regression */
			if (rc_lnksta_speed < (rc_target_speed - 1)) {
				/* Fall back to the previous speed */
				--rc_target_speed;
				baikal_pcie_link_retrain(pp, rc_target_speed);
			}

			break;
		}

		++rc_target_speed;
	}

	pcie->retrained = 1;
}

static void baikal_pcie_retrain_links(const struct pci_bus *bus)
{
       struct pci_dev *dev;
       struct pci_bus *child;

       list_for_each_entry(dev, &bus->devices, bus_list)
               baikal_pcie_link_speed_fixup(dev);

       list_for_each_entry(dev, &bus->devices, bus_list) {
               child = dev->subordinate;
               if (child)
                       baikal_pcie_retrain_links(child);
       }
}

static int baikal_pcie_link_up(struct dw_pcie *pp)
{
	struct baikal_pcie *pcie = to_baikal_pcie(pp);
	unsigned long reg;

	reg = baikal_pcie_lcru_readl(pcie->lcru,
				     BAIKAL_LCRU_PCIE_GEN_CTL(pcie->bus_nr));

	if ((reg & BAIKAL_PCIE_LTSSM_ENABLE) == 0) {
		return 0;
	}

	reg = baikal_pcie_lcru_readl(pcie->lcru,
				     BAIKAL_LCRU_PCIE_STATUS(pcie->bus_nr));

	return (reg & BAIKAL_PCIE_SMLH_LINKUP) &&
	       (reg & BAIKAL_PCIE_RDLH_LINKUP);
}

static int baikal_pcie_get_msi(struct baikal_pcie *pcie,
			       struct device_node *msi_node,
			       u64 *msi_addr)
{
	struct device *dev = pcie->pp.dev;
	int ret;
	struct resource res;

	/*
	 * Check if 'msi-parent' points to ARM GICv3 ITS, which is the only
	 * supported MSI controller.
	 */
	if (!of_device_is_compatible(msi_node, "arm,gic-v3-its")) {
		dev_err(dev, "unable to find compatible MSI controller\n");
		return -ENODEV;
	}

	/* Derive GITS_TRANSLATER address from GICv3 */
	ret = of_address_to_resource(msi_node, 0, &res);
	if (ret < 0) {
		dev_err(dev, "unable to obtain MSI controller resources\n");
		return ret;
	}

	*msi_addr = res.start + GITS_TRANSLATER;
	return 0;
}

static int baikal_pcie_msi_steer(struct baikal_pcie *pcie,
				 struct device_node *msi_node)
{
	struct dw_pcie *pp = &pcie->pp;
	struct device *dev = pp->dev;
	int ret;
	u64 msi_addr;

	ret = baikal_pcie_get_msi(pcie, msi_node, &msi_addr);
	if (ret < 0) {
		dev_err(dev, "MSI steering failed\n");
		return ret;
	}

	/* Program the msi_data */
	dw_pcie_write(pp->dbi_base + PCIE_MSI_CTRL_ADDR_LO_REG, 4,
			  lower_32_bits(msi_addr));
	dw_pcie_write(pp->dbi_base + PCIE_MSI_CTRL_ADDR_HI_REG, 4,
			  upper_32_bits(msi_addr));
	return 0;
}

static int baikal_pcie_msi_enable(struct baikal_pcie *pcie)
{
	struct device *dev = pcie->pp.dev;
	struct device_node *msi_node;
	int ret;

	/*
	 * The "msi-parent" phandle needs to exist
	 * for us to obtain the MSI node.
	 */
	msi_node = of_parse_phandle(dev->of_node, "msi-parent", 0);
	if (!msi_node) {
		dev_err(dev, "failed to read msi-parent node from FDT\n");
		return -ENODEV;
	}

	ret = baikal_pcie_msi_steer(pcie, msi_node);
	if (ret) {
		goto out_put_node;
	}

out_put_node:
	of_node_put(msi_node);
	return ret;
}

static irqreturn_t baikal_pcie_err_irq_handler(int irq, void *priv)
{
	struct baikal_pcie *pcie = priv;
	struct device *dev = pcie->pp.dev;
	unsigned long corr_err_status;
	unsigned long dev_ctrl_dev_status;
	unsigned long root_err_status;
	unsigned long uncorr_err_status;

	uncorr_err_status   = dw_pcie_readl_dbi(&pcie->pp,
					       PCIE_UNCORR_ERR_STATUS_REG);
	corr_err_status	    = dw_pcie_readl_dbi(&pcie->pp,
					       PCIE_CORR_ERR_STATUS_REG);
	root_err_status     = dw_pcie_readl_dbi(&pcie->pp,
					       PCIE_ROOT_ERR_STATUS_REG);
	dev_ctrl_dev_status = dw_pcie_readl_dbi(&pcie->pp,
					       PCIE_DEVICE_CONTROL_DEVICE_STATUS_REG);
	dev_err(dev,
		"dev_err:0x%lx root_err:0x%lx uncorr_err:0x%lx corr_err:0x%lx\n",
		(dev_ctrl_dev_status & 0xf0000) >> 16,
		root_err_status, uncorr_err_status, corr_err_status);

	dw_pcie_writel_dbi(&pcie->pp,
		PCIE_UNCORR_ERR_STATUS_REG, uncorr_err_status);
	dw_pcie_writel_dbi(&pcie->pp,
		PCIE_CORR_ERR_STATUS_REG, corr_err_status);
	dw_pcie_writel_dbi(&pcie->pp,
		PCIE_ROOT_ERR_STATUS_REG, root_err_status);
	dw_pcie_writel_dbi(&pcie->pp,
		PCIE_DEVICE_CONTROL_DEVICE_STATUS_REG, dev_ctrl_dev_status);

	return IRQ_HANDLED;
}

static int baikal_pcie_host_init(struct pcie_port *port)
{
	struct dw_pcie* pp = to_dw_pcie_from_pp(port);
	struct device *dev = pp->dev;
	struct baikal_pcie *pcie = to_baikal_pcie(pp);
	int err;
	int linkup;
	unsigned idx;
	unsigned long reg;

	/* Disable access to PHY registers and DBI2 mode */
	reg = baikal_pcie_lcru_readl(pcie->lcru,
				     BAIKAL_LCRU_PCIE_GEN_CTL(pcie->bus_nr));

	reg &= ~(BAIKAL_PCIE_PHY_MGMT_ENABLE |
		 BAIKAL_PCIE_DBI2_MODE);

	baikal_pcie_lcru_writel(pcie->lcru,
				BAIKAL_LCRU_PCIE_GEN_CTL(pcie->bus_nr), reg);

	pcie->retrained = 0;
	linkup = baikal_pcie_link_up(pp);

	/* If link is not established yet, reset the RC */
	if (!linkup) {
		/* Disable link training */
		reg = baikal_pcie_lcru_readl(pcie->lcru,
					BAIKAL_LCRU_PCIE_GEN_CTL(pcie->bus_nr));

		reg &= ~BAIKAL_PCIE_LTSSM_ENABLE;
		baikal_pcie_lcru_writel(pcie->lcru,
				   BAIKAL_LCRU_PCIE_GEN_CTL(pcie->bus_nr), reg);

		/* Assert PERST pin */
		if (pcie->reset_gpio != NULL) {
			unsigned long gpio_flags;

			if (pcie->reset_active_low) {
				gpio_flags = GPIOF_ACTIVE_LOW |
					     GPIOF_OUT_INIT_LOW;
			} else {
				gpio_flags = GPIOF_OUT_INIT_HIGH;
			}

			err = devm_gpio_request_one(dev,
						 desc_to_gpio(pcie->reset_gpio),
						 gpio_flags, pcie->reset_name);

			if (err) {
				dev_err(dev, "request GPIO failed (%d)\n", err);
				return -ENODEV;
			}
		}

		/* Reset the RC */
		reg = baikal_pcie_lcru_readl(pcie->lcru,
					  BAIKAL_LCRU_PCIE_RESET(pcie->bus_nr));

		reg |= BAIKAL_PCIE_NONSTICKY_RST |
                       BAIKAL_PCIE_STICKY_RST |
                       BAIKAL_PCIE_PWR_RST |
                       BAIKAL_PCIE_CORE_RST |
                       BAIKAL_PCIE_PHY_RESET;

		/* If the RC is PCIe x8, reset PIPE0 and PIPE1 */
		if (pcie->bus_nr == 2) {
			reg |= BAIKAL_PCIE_PIPE0_RESET |
			       BAIKAL_PCIE_PIPE1_RESET;
		} else {
			reg |= BAIKAL_PCIE_PIPE_RESET;
		}

		baikal_pcie_lcru_writel(pcie->lcru,
				     BAIKAL_LCRU_PCIE_RESET(pcie->bus_nr), reg);

		usleep_range(20000, 30000);

		if (pcie->reset_gpio != NULL) {
			/* Deassert PERST pin */
			gpiod_set_value_cansleep(pcie->reset_gpio, 0);
		}

		/* Deassert PHY reset */
		reg = baikal_pcie_lcru_readl(pcie->lcru,
					  BAIKAL_LCRU_PCIE_RESET(pcie->bus_nr));

		reg &= ~BAIKAL_PCIE_PHY_RESET;
		baikal_pcie_lcru_writel(pcie->lcru,
				     BAIKAL_LCRU_PCIE_RESET(pcie->bus_nr), reg);

		/* Deassert all software controlled resets */
		reg = baikal_pcie_lcru_readl(pcie->lcru,
					  BAIKAL_LCRU_PCIE_RESET(pcie->bus_nr));

		reg &= ~(BAIKAL_PCIE_ADB_PWRDWN |
			 BAIKAL_PCIE_HOT_RESET |
			 BAIKAL_PCIE_NONSTICKY_RST |
			 BAIKAL_PCIE_STICKY_RST |
			 BAIKAL_PCIE_PWR_RST |
			 BAIKAL_PCIE_CORE_RST |
			 BAIKAL_PCIE_PHY_RESET);

		if (pcie->bus_nr == 2) {
			reg &= ~(BAIKAL_PCIE_PIPE0_RESET |
				 BAIKAL_PCIE_PIPE1_RESET);
		} else {
			reg &= ~BAIKAL_PCIE_PIPE_RESET;
		}

		baikal_pcie_lcru_writel(pcie->lcru,
				     BAIKAL_LCRU_PCIE_RESET(pcie->bus_nr), reg);
	}

	/* Deinitialise all iATU regions */
	for (idx = 0; idx < pp->num_ob_windows; ++idx) {
		dw_pcie_writel_dbi(pp, PCIE_IATU_VIEWPORT_REG,
				  PCIE_IATU_REGION_OUTBOUND | idx);
		dw_pcie_writel_dbi(pp, PCIE_IATU_REGION_CTRL_2_REG, 0);
	}

	/*
	 * Enable writing to config regs. This is required as the DW driver
	 * changes the class code. That register needs DBI write enable.
	 */
	reg = dw_pcie_readl_dbi(pp, PCIE_MISC_CONTROL_1_REG);
	reg |= PCIE_DBI_RO_RW_EN;
	dw_pcie_writel_dbi(pp, PCIE_MISC_CONTROL_1_REG, reg);

	dw_pcie_setup_rc(port);

	/* Set prog-if = 1 */
	reg = dw_pcie_readl_dbi(pp, PCI_CLASS_REVISION);
	reg = (1 << 8) | (reg & 0xffff00ff);
	dw_pcie_writel_dbi(pp, PCI_CLASS_REVISION, reg);

	/* Set max link width in accordance with 'num-lanes' value */
	reg = dw_pcie_readl_dbi(pp, PCIE_LINK_CAPABILITIES_REG);
	reg &= ~PCIE_CAP_MAX_LINK_WIDTH_MASK;
	reg |= (pcie->num_lanes) << PCIE_CAP_MAX_LINK_WIDTH_SHIFT;
	dw_pcie_writel_dbi(pp, PCIE_LINK_CAPABILITIES_REG, reg);

	/* Disable writing to config regs */
	reg = dw_pcie_readl_dbi(pp, PCIE_MISC_CONTROL_1_REG);
	reg &= ~PCIE_DBI_RO_RW_EN;
	dw_pcie_writel_dbi(pp, PCIE_MISC_CONTROL_1_REG, reg);

	if (IS_ENABLED(CONFIG_PCI_MSI)) {
		err = baikal_pcie_msi_enable(pcie);
		if (err) {
			dev_err(pp->dev, "failed to initialize MSI\n");
			return -EIO;
		}
	}

	/* Enable error reporting */
	reg = dw_pcie_readl_dbi(pp, PCIE_ROOT_ERR_CMD_REG);
	reg |= PCIE_CORR_ERR_REPORTING_EN |
	       PCIE_NON_FATAL_ERR_REPORTING_EN |
	       PCIE_FATAL_ERR_REPORTING_EN;
	dw_pcie_writel_dbi(pp, PCIE_ROOT_ERR_CMD_REG, reg);

	reg = dw_pcie_readl_dbi(pp, PCIE_DEVICE_CONTROL_DEVICE_STATUS_REG);
	reg |= PCIE_CAP_CORR_ERR_REPORT_EN |
	       PCIE_CAP_NON_FATAL_ERR_REPORT_EN |
	       PCIE_CAP_FATAL_ERR_REPORT_EN |
	       PCIE_CAP_UNSUPPORT_REQ_REP_EN;
	dw_pcie_writel_dbi(pp, PCIE_DEVICE_CONTROL_DEVICE_STATUS_REG, reg);

	reg = dw_pcie_readl_dbi(pp, PCIE_ROOT_CONTROL_ROOT_CAPABILITIES_REG);
	reg |= PCIE_CAP_SYS_ERR_ON_CORR_ERR_EN |
	       PCIE_CAP_SYS_ERR_ON_NON_FATAL_ERR_EN |
	       PCIE_CAP_SYS_ERR_ON_FATAL_ERR_EN |
	       PCIE_CAP_PME_INT_EN;
	dw_pcie_writel_dbi(pp, PCIE_ROOT_CONTROL_ROOT_CAPABILITIES_REG, reg);

	if (linkup) {
		dev_info(dev, "link is already up\n");
		return 0;
	}

	/* Use Gen1 mode for link establishing */
	reg = dw_pcie_readl_dbi(pp, PCIE_LINK_CONTROL2_LINK_STATUS2_REG);
	reg &= ~PCIE_CAP_TARGET_LINK_SPEED_MASK;
	reg |= 1;
	dw_pcie_writel_dbi(pp, PCIE_LINK_CONTROL2_LINK_STATUS2_REG, reg);

	/*
	 * Clear DIRECT_SPEED_CHANGE bit. It has been set by dw_pcie_setup_rc.
	 * This bit causes link retraining. However, link retraining should be
	 * performed later by calling a speed fixup function.
	 */
	reg = dw_pcie_readl_dbi(pp, PCIE_GEN2_CTRL_REG);
	reg &= ~PCIE_DIRECT_SPEED_CHANGE;
	dw_pcie_writel_dbi(pp, PCIE_GEN2_CTRL_REG, reg);

	/* Establish link */
	reg = baikal_pcie_lcru_readl(pcie->lcru,
				     BAIKAL_LCRU_PCIE_GEN_CTL(pcie->bus_nr));

	reg |= BAIKAL_PCIE_LTSSM_ENABLE;
	baikal_pcie_lcru_writel(pcie->lcru,
				BAIKAL_LCRU_PCIE_GEN_CTL(pcie->bus_nr), reg);

	dw_pcie_wait_for_link(pp);
	/* XXX:
	 * - return OK even if the link is down
	 * - on error dw_pcie_wait_for_link prints a warning on its own,
	 *   there's no need to print more messages here
	 */
	return 0;
}

static int baikal_pcie_msi_host_init(struct pcie_port *pp)
{
	struct dw_pcie* pcie = to_dw_pcie_from_pp(pp);
	struct device *dev = pcie->dev;
	struct device_node *np = dev->of_node;
	struct device_node *msi_node;

	/*
	 * The MSI domain is set by the generic of_msi_configure(). This
	 * .msi_host_init() function keeps us from doing the default MSI domain
	 * setup in dw_pcie_host_init() and also enforces the requirement that
	 * "msi-parent" exists.
	 */
	msi_node = of_parse_phandle(np, "msi-parent", 0);
	if (!msi_node) {
		dev_err(dev, "failed to find msi-parent\n");
		return -EINVAL;
	}

	return 0;
}

static const struct dw_pcie_host_ops baikal_pcie_host_ops = {
	.host_init = baikal_pcie_host_init,
	.msi_host_init = baikal_pcie_msi_host_init,
};

static const struct dw_pcie_ops baikal_pcie_ops = {
	.link_up = baikal_pcie_link_up,
};

static int baikal_add_pcie_port(struct baikal_pcie *pcie,
				struct platform_device *pdev)
{
	struct dw_pcie *dw_pci = &pcie->pp;
	struct pcie_port *pp = &dw_pci->pp;
	struct resource *res;
	int irq;
	int ret;

	dw_pci->dev = &pdev->dev;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dbi");
	if (res) {
		devm_request_resource(dw_pci->dev, &iomem_resource, res);
		dw_pci->dbi_base = devm_ioremap_resource(dw_pci->dev, res);
		if (IS_ERR(dw_pci->dbi_base)) {
			dev_err(dw_pci->dev, "error with ioremap\n");
			return -ENOMEM;
		}
	} else {
		dev_err(dw_pci->dev, "missing *dbi* reg space\n");
		return -EINVAL;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(dw_pci->dev, "missing IRQ resource: %d\n", irq);
		return irq;
	}

	ret = devm_request_irq(dw_pci->dev, irq, baikal_pcie_err_irq_handler,
			       IRQF_SHARED | IRQF_NO_THREAD,
			       "baikal-pcie-error-irq", pcie);
	if (ret) {
		dev_err(dw_pci->dev, "failed to request IRQ\n");
		return ret;
	}

	pp->ops = &baikal_pcie_host_ops;
	ret = dw_pcie_host_init(pp);
	if (ret) {
		dev_err(dw_pci->dev, "failed to initialize host\n");
		return ret;
	}
	baikal_pcie_retrain_links(pp->bridge->bus);

	return 0;
}

static bool has_incompat_firmware(void) {
	bool gotcha = false;
	struct device_node *np = NULL;
	np = of_find_node_by_path("/soc");
	if (np) {
		of_node_put(np);
	} else {
		gotcha = true;
	}
	return gotcha;
}

static int baikal_pcie_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct baikal_pcie *pcie;
	int err;
	u32 index[2];
	enum of_gpio_flags gpio_flags;
	int reset_gpio;

	if (has_incompat_firmware()) {
		dev_err(dev, "detected incompatible firmware, bailing out\n");
		return -EINVAL;
	}
	if (!of_match_device(of_baikal_pcie_match, dev)) {
		return -EINVAL;
	}

	pcie = devm_kzalloc(dev, sizeof(*pcie), GFP_KERNEL);
	if (!pcie) {
		return -ENOMEM;
	}
	pcie->pp.dev = dev;
	pcie->pp.ops = &baikal_pcie_ops;

	err = of_property_read_u32(dev->of_node, "num-lanes", &pcie->num_lanes);
	if (err) {
		dev_err(dev, "property num-lanes isn't found\n");
		return -EINVAL;
	}
	pcie->lcru = syscon_regmap_lookup_by_phandle(dev->of_node,
						     "baikal,pcie-lcru");
	if (IS_ERR(pcie->lcru)) {
		dev_err(dev, "No LCRU phandle specified\n");
		pcie->lcru = NULL;
		return -EINVAL;
	}

	if (of_property_read_u32_array(dev->of_node, "baikal,pcie-lcru", index, 2)) {
		pcie->lcru = NULL;
		return -EINVAL;
	}

	pcie->bus_nr = index[1];

	pm_runtime_enable(dev);
	err = pm_runtime_get_sync(dev);
	if (err < 0) {
		dev_err(dev, "pm_runtime_get_sync failed\n");
		goto err_pm_disable;
	}

	reset_gpio = of_get_named_gpio_flags(dev->of_node, "reset-gpios", 0,
					     &gpio_flags);
	if (gpio_is_valid(reset_gpio)) {
		pcie->reset_gpio = gpio_to_desc(reset_gpio);
		pcie->reset_active_low = gpio_flags & OF_GPIO_ACTIVE_LOW;
		snprintf(pcie->reset_name, sizeof pcie->reset_name,
			"pcie%u-reset", pcie->bus_nr);
	} else {
		pcie->reset_gpio = NULL;
	}

	err = baikal_add_pcie_port(pcie, pdev);
	if (err < 0) {
		goto err_pm_put;
	}

	platform_set_drvdata(pdev, pcie);
	return 0;

err_pm_put:
	pm_runtime_put(dev);

err_pm_disable:
	pm_runtime_disable(dev);

	return err;
}

#ifdef CONFIG_PM_SLEEP
static int baikal_pcie_suspend(struct device *dev)
{
	struct baikal_pcie *pcie = dev_get_drvdata(dev);
	struct dw_pcie *pp = &pcie->pp;
	u32 val;

	/* Clear Memory Space Enable (MSE) bit */
	val = dw_pcie_readl_dbi(pp, PCI_COMMAND);
	val &= ~PCI_COMMAND_MEMORY;
	dw_pcie_writel_dbi(pp, PCI_COMMAND, val);
	return 0;
}

static int baikal_pcie_resume(struct device *dev)
{
	struct baikal_pcie *pcie = dev_get_drvdata(dev);
	struct dw_pcie *pp = &pcie->pp;
	u32 val;

	/* Set Memory Space Enable (MSE) bit */
	val = dw_pcie_readl_dbi(pp, PCI_COMMAND);
	val |= PCI_COMMAND_MEMORY;
	dw_pcie_writel_dbi(pp, PCI_COMMAND, val);
	return 0;
}

static int baikal_pcie_suspend_noirq(struct device *dev)
{
	return 0;
}

static int baikal_pcie_resume_noirq(struct device *dev)
{
	return 0;
}
#endif

static const struct dev_pm_ops baikal_pcie_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(baikal_pcie_suspend, baikal_pcie_resume)
	SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(baikal_pcie_suspend_noirq,
				      baikal_pcie_resume_noirq)
};

static struct platform_driver baikal_pcie_driver = {
	.driver = {
		.name	= "baikal-pcie-v2",
		.of_match_table = of_baikal_pcie_match,
		.suppress_bind_attrs = true,
		.pm	= &baikal_pcie_pm_ops,
	},
	.probe = baikal_pcie_probe,
};

MODULE_DEVICE_TABLE(of, of_baikal_pcie_match);
module_platform_driver(baikal_pcie_driver);
MODULE_LICENSE("GPL v2");
