/*
 * Copyright (C) 2019-2020 Baikal Electronics JSC
 *
 * Author: Pavel Parkhomenko <Pavel.Parkhomenko@baikalelectronics.ru>
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

/**
 * baikal_vdu_connector.c
 * Implementation of the connector functions for Baikal Electronics BE-M1000 SoC's VDU
 */
#include <linux/version.h>
#include <linux/shmem_fs.h>
#include <linux/dma-buf.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_of.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

#include "baikal_vdu_drm.h"
#include "baikal_vdu_regs.h"

#define to_baikal_vdu_private(x) \
	container_of(x, struct baikal_vdu_private, connector)

static void baikal_vdu_drm_connector_destroy(struct drm_connector *connector)
{
	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
}

static enum drm_connector_status baikal_vdu_drm_connector_detect(
		struct drm_connector *connector, bool force)
{
	struct baikal_vdu_private *priv = to_baikal_vdu_private(connector);

	return (priv->panel ?
		connector_status_connected :
		connector_status_disconnected);
}

static int baikal_vdu_drm_connector_helper_get_modes(
		struct drm_connector *connector)
{
	struct baikal_vdu_private *priv = to_baikal_vdu_private(connector);

	if (!priv->panel)
		return 0;

	return drm_panel_get_modes(priv->panel, connector);
}

const struct drm_connector_funcs connector_funcs = {
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = baikal_vdu_drm_connector_destroy,
	.detect = baikal_vdu_drm_connector_detect,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

const struct drm_connector_helper_funcs connector_helper_funcs = {
	.get_modes = baikal_vdu_drm_connector_helper_get_modes,
};

static const struct drm_encoder_funcs encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

int baikal_vdu_lvds_connector_create(struct drm_device *dev)
{
	struct baikal_vdu_private *priv = dev->dev_private;
	struct drm_connector *connector = &priv->connector;
	struct drm_encoder *encoder = &priv->encoder;
	int ret = 0;

	ret = drm_connector_init(dev, connector, &connector_funcs,
			DRM_MODE_CONNECTOR_LVDS);
	if (ret) {
		dev_err(dev->dev, "drm_connector_init failed: %d\n", ret);
		goto out;
	}
	drm_connector_helper_add(connector, &connector_helper_funcs);
	ret = drm_encoder_init(dev, encoder, &encoder_funcs,
			       DRM_MODE_ENCODER_LVDS, NULL);
	if (ret) {
		dev_err(dev->dev, "drm_encoder_init failed: %d\n", ret);
		goto out;
	}
	encoder->crtc = &priv->crtc;
	encoder->possible_crtcs = drm_crtc_mask(encoder->crtc);
	ret = drm_connector_attach_encoder(connector, encoder);
	if (ret) {
		dev_err(dev->dev, "drm_connector_attach_encoder failed: %d\n", ret);
		goto out;
	}
	ret = drm_connector_register(connector);
	if (ret) {
		dev_err(dev->dev, "drm_connector_register failed: %d\n", ret);
		goto out;
	}
out:
	return ret;
}
