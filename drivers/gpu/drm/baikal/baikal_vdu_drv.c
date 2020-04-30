/*
 * Copyright (C) 2019-2020 Baikal Electronics JSC
 *
 * Author: Pavel Parkhomenko <Pavel.Parkhomenko@baikalelectronics.ru>
 * All bugs by Alexey Sheplyakov <asheplyakov@altlinux.org>
 *
 * This driver is based on ARM PL111 DRM driver
 *
 * Parts of this file were based on sources as follows:
 *
 * Copyright (c) 2006-2008 Intel Corporation
 * Copyright (c) 2007 Dave Airlie <airlied@linux.ie>
 * Copyright (C) 2011 Texas Instruments
 * (C) COPYRIGHT 2012-2013 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms of
 * such GNU licence.
 *
 */

#include <linux/arm-smccc.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_aperture.h>
#include <drm/drm_bridge.h>
#include <drm/drm_connector.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_of.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_vblank.h>

#include "baikal_vdu_drm.h"
#include "baikal_vdu_regs.h"

#define DRIVER_NAME                 "baikal-vdu"
#define DRIVER_DESC                 "DRM module for Baikal VDU"
#define DRIVER_DATE                 "20200131"

#define BAIKAL_SMC_SCP_LOG_DISABLE  0x82000200

int mode_fixup = 0;

static struct drm_mode_config_funcs mode_config_funcs = {
	.fb_create = drm_gem_fb_create,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};

static const struct drm_encoder_funcs baikal_vdu_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

DEFINE_DRM_GEM_CMA_FOPS(drm_fops);

static struct drm_driver vdu_drm_driver = {
	.driver_features = DRIVER_GEM | DRIVER_MODESET | DRIVER_ATOMIC,
	.ioctls = NULL,
	.fops = &drm_fops,
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = 1,
	.minor = 0,
	.patchlevel = 0,
	DRM_GEM_CMA_DRIVER_OPS,
#if defined(CONFIG_DEBUG_FS)
	.debugfs_init = baikal_vdu_debugfs_init,
#endif
};

static int vdu_modeset_init(struct drm_device *dev)
{
	struct drm_mode_config *mode_config;
	struct baikal_vdu_private *priv = dev->dev_private;
	struct arm_smccc_res res;
	int ret = 0, ep_count = 0;

	if (priv == NULL)
		return -EINVAL;

	drm_mode_config_init(dev);
	mode_config = &dev->mode_config;
	mode_config->funcs = &mode_config_funcs;
	mode_config->min_width = 1;
	mode_config->max_width = 4095;
	mode_config->min_height = 1;
	mode_config->max_height = 4095;

	ret = baikal_vdu_primary_plane_init(dev);
	if (ret != 0) {
		dev_err(dev->dev, "Failed to init primary plane\n");
		goto out_config;
	}

	ret = baikal_vdu_crtc_create(dev);
	if (ret) {
		dev_err(dev->dev, "Failed to create crtc\n");
		goto out_config;
	}

	ret = drm_of_find_panel_or_bridge(dev->dev->of_node, -1, -1,
					  &priv->panel,
					  &priv->bridge);
	if (ret == -EPROBE_DEFER) {
		dev_info(dev->dev, "Bridge probe deferred\n");
		goto out_config;
	}
	ep_count = of_graph_get_endpoint_count(dev->dev->of_node);
	if (ep_count <= 0) {
		dev_err(dev->dev, "no endpoints connected to panel/bridge\n");
		goto out_config;
	}
	priv->ep_count = ep_count;
	dev_dbg(dev->dev, "panel/bridge has %d endpoints\n", priv->ep_count);

	if (priv->bridge) {
		struct drm_encoder *encoder = &priv->encoder;
		ret = drm_encoder_init(dev, encoder, &baikal_vdu_encoder_funcs,
				       DRM_MODE_ENCODER_NONE, NULL);
		if (ret) {
			dev_err(dev->dev, "failed to create DRM encoder\n");
			goto out_config;
		}
		encoder->crtc = &priv->crtc;
		encoder->possible_crtcs = drm_crtc_mask(encoder->crtc);
		priv->bridge->encoder = &priv->encoder;
		ret = drm_bridge_attach(&priv->encoder, priv->bridge, NULL, 0);
		if (ret) {
			dev_err(dev->dev, "Failed to attach DRM bridge %d\n", ret);
			goto out_config;
		}
	} else if (priv->panel) {
		dev_dbg(dev->dev, "panel has %d endpoints\n", priv->ep_count);
		ret = baikal_vdu_lvds_connector_create(dev);
		if (ret) {
			dev_err(dev->dev, "Failed to create DRM connector\n");
			goto out_config;
		}
	} else
		ret = -EINVAL;

	if (ret) {
		dev_err(dev->dev, "No bridge or panel attached!\n");
		goto out_config;
	}

	priv->clk = clk_get(dev->dev, "pclk");
	if (IS_ERR(priv->clk)) {
		dev_err(dev->dev, "fatal: unable to get pclk, err %ld\n", PTR_ERR(priv->clk));
		ret = PTR_ERR(priv->clk);
		goto out_config;
	}

	priv->mode_fixup = mode_fixup;

	drm_aperture_remove_framebuffers(false, &vdu_drm_driver);

	ret = drm_vblank_init(dev, 1);
	if (ret != 0) {
		dev_err(dev->dev, "Failed to init vblank\n");
		goto out_clk;
	}

	arm_smccc_smc(BAIKAL_SMC_SCP_LOG_DISABLE, 0, 0, 0, 0, 0, 0, 0, &res);

	drm_mode_config_reset(dev);

	drm_kms_helper_poll_init(dev);

	ret = drm_dev_register(dev, 0);
	if (ret)
		goto out_clk;

	drm_fbdev_generic_setup(dev, 32);
	goto finish;

out_clk:
	clk_put(priv->clk);
out_config:
	drm_mode_config_cleanup(dev);
finish:
	return ret;
}


static int baikal_vdu_irq_install(struct baikal_vdu_private *priv, int irq)
{
	int ret;
	ret= request_irq(irq, baikal_vdu_irq, 0, DRIVER_NAME, priv->drm);
	if (ret < 0)
		return ret;
	priv->irq_enabled = true;
	return 0;
}

static void baikal_vdu_irq_uninstall(struct baikal_vdu_private *priv)
{
	if (priv->irq_enabled) {
		priv->irq_enabled = false;
		disable_irq(priv->irq);
		free_irq(priv->irq, priv->drm);
	}
}

static int vdu_maybe_enable_lvds(struct baikal_vdu_private *vdu)
{
	int err = 0;
	struct device *dev;
	if (!vdu->drm) {
		pr_err("%s: vdu->drm is NULL\n", __func__);
		return -EINVAL;
	}
	dev = vdu->drm->dev;

	vdu->enable_gpio = devm_gpiod_get_optional(dev, "enable", GPIOD_OUT_LOW);
	if (IS_ERR(vdu->enable_gpio)) {
		err = (int)PTR_ERR(vdu->enable_gpio);
		dev_err(dev, "failed to get enable-gpios, error %d\n", err);
		vdu->enable_gpio = NULL;
		return err;
	}
	if (vdu->enable_gpio) {
		dev_dbg(dev, "%s: setting enable-gpio\n", __func__);
		gpiod_set_value_cansleep(vdu->enable_gpio, 1);
	} else {
		dev_dbg(dev, "%s: no enable-gpios, assuming it's handled by panel-lvds\n", __func__);
	}
	return 0;
}

static int baikal_vdu_drm_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct baikal_vdu_private *priv;
	struct drm_device *drm;
	struct resource *mem;
	int irq;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	drm = drm_dev_alloc(&vdu_drm_driver, dev);
	if (IS_ERR(drm))
		return PTR_ERR(drm);
	platform_set_drvdata(pdev, drm);
	priv->drm = drm;
	drm->dev_private = priv;

	if (!(mem = platform_get_resource(pdev, IORESOURCE_MEM, 0))) {
		dev_err(dev, "%s no MMIO resource specified\n", __func__);
		return -EINVAL;
	}

	priv->regs = devm_ioremap_resource(dev, mem);
	if (IS_ERR(priv->regs)) {
		dev_err(dev, "%s MMIO allocation failed\n", __func__);
		return PTR_ERR(priv->regs);
	}

	/* turn off interrupts before requesting the irq */
	writel(0, priv->regs + IMR);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(dev, "%s no IRQ resource specified\n", __func__);
		return -EINVAL;
	}
	priv->irq = irq;

	ret = baikal_vdu_irq_install(priv, irq);
	if (ret != 0) {
		dev_err(dev, "%s IRQ %d allocation failed\n", __func__, irq);
		return ret;
	}

	if (pdev->dev.of_node && of_property_read_bool(pdev->dev.of_node, "lvds-out")) {
		priv->type = VDU_TYPE_LVDS;
		if (of_property_read_u32(pdev->dev.of_node, "num-lanes", &priv->ep_count))
			priv->ep_count = 1;
	}
	else
		priv->type = VDU_TYPE_HDMI;

	ret = vdu_modeset_init(drm);
	if (ret != 0) {
		dev_err(dev, "Failed to init modeset\n");
		goto dev_unref;
	}

	ret = vdu_maybe_enable_lvds(priv);
	if (ret != 0) {
		dev_err(dev, "failed to enable LVDS\n");
	}

	return 0;

dev_unref:
	writel(0, priv->regs + IMR);
	writel(0x3ffff, priv->regs + ISR);
	baikal_vdu_irq_uninstall(priv);
	drm->dev_private = NULL;
	drm_dev_put(drm);
	return ret;
}

static int baikal_vdu_drm_remove(struct platform_device *pdev)
{
	struct drm_device *drm;
	struct baikal_vdu_private *priv;

	drm = platform_get_drvdata(pdev);
	if (!drm) {
		return -1;
	}
	priv = drm->dev_private;

	drm_dev_unregister(drm);
	drm_mode_config_cleanup(drm);
	baikal_vdu_irq_uninstall(priv);
	drm->dev_private = NULL;
	drm_dev_put(drm);

	return 0;
}

static const struct of_device_id baikal_vdu_of_match[] = {
    { .compatible = "baikal,vdu" },
    { },
};
MODULE_DEVICE_TABLE(of, baikal_vdu_of_match);

static struct platform_driver baikal_vdu_platform_driver = {
    .probe  = baikal_vdu_drm_probe,
    .remove = baikal_vdu_drm_remove,
    .driver = {
        .name   = DRIVER_NAME,
        .of_match_table = baikal_vdu_of_match,
    },
};

module_param(mode_fixup, int, 0644);

module_platform_driver(baikal_vdu_platform_driver);

MODULE_AUTHOR("Pavel Parkhomenko <Pavel.Parkhomenko@baikalelectronics.ru>");
MODULE_DESCRIPTION("Baikal Electronics BE-M1000 Video Display Unit (VDU) DRM Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRIVER_NAME);
MODULE_SOFTDEP("pre: baikal_hdmi");
