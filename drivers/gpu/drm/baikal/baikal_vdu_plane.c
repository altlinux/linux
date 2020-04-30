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

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/of_graph.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_plane_helper.h>

#include "baikal_vdu_drm.h"
#include "baikal_vdu_regs.h"

static int baikal_vdu_primary_plane_atomic_check(struct drm_plane *plane,
						 struct drm_atomic_state *atomic_state)
{
	struct drm_device *dev = plane->dev;
	struct baikal_vdu_private *priv = dev->dev_private;
	struct drm_crtc_state *crtc_state;
	struct drm_plane_state *state;
	struct drm_display_mode *mode;
	int rate, ret;
	u32 cntl;

	state = drm_atomic_get_new_plane_state(atomic_state, plane);
	if (!state || !state->crtc)
		return 0;

	crtc_state = drm_atomic_get_crtc_state(state->state, state->crtc);
	if (IS_ERR(crtc_state)) {
		ret = PTR_ERR(crtc_state);
		dev_warn(dev->dev, "failed to get crtc_state: %d\n", ret);
		return ret;
	}
	mode = &crtc_state->adjusted_mode;
	rate = mode->clock * 1000;
	if (rate == clk_get_rate(priv->clk))
		return 0;

	/* hold clock domain reset; disable clocking */
	writel(0, priv->regs + PCTR);

	if (__clk_is_enabled(priv->clk))
		clk_disable_unprepare(priv->clk);
	ret = clk_set_rate(priv->clk, rate);
	DRM_DEV_DEBUG_DRIVER(dev->dev, "Requested pixel clock is %d Hz\n", rate);

	if (ret < 0) {
		DRM_ERROR("Cannot set desired pixel clock (%d Hz)\n",
			  rate);
		ret = -EINVAL;
	} else {
		clk_prepare_enable(priv->clk);
		if (__clk_is_enabled(priv->clk))
			ret = 0;
		else {
			DRM_ERROR("PLL could not lock at desired frequency (%d Hz)\n",
			  rate);
			ret = -EINVAL;
		}
	}

	/* release clock domain reset; enable clocking */
	cntl = readl(priv->regs + PCTR);
	cntl |= PCTR_PCR + PCTR_PCI;
	writel(cntl, priv->regs + PCTR);

	return ret;
}

static void baikal_vdu_primary_plane_atomic_update(struct drm_plane *plane,
						   struct drm_atomic_state *old_state)
{
	struct drm_device *dev = plane->dev;
	struct baikal_vdu_private *priv = dev->dev_private;
	struct drm_plane_state *state = plane->state;
	struct drm_framebuffer *fb = state->fb;
	u32 cntl, addr, end;

	if (!fb)
		return;

	addr = drm_fb_cma_get_gem_addr(fb, state, 0);
	priv->fb_addr = addr & 0xfffffff8;

	cntl = readl(priv->regs + CR1);
	cntl &= ~CR1_BPP_MASK;

	/* Note that the the hardware's format reader takes 'r' from
	 * the low bit, while DRM formats list channels from high bit
	 * to low bit as you read left to right.
	 */
	switch (fb->format->format) {
	case DRM_FORMAT_BGR888:
		cntl |= CR1_BPP24 | CR1_FBP | CR1_BGR;
		break;
	case DRM_FORMAT_RGB888:
		cntl |= CR1_BPP24 | CR1_FBP;
		break;
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_XBGR8888:
		cntl |= CR1_BPP24 | CR1_BGR;
		break;
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XRGB8888:
		cntl |= CR1_BPP24;
		break;
	case DRM_FORMAT_BGR565:
		cntl |= CR1_BPP16_565 | CR1_BGR;
		break;
	case DRM_FORMAT_RGB565:
		cntl |= CR1_BPP16_565;
		break;
	case DRM_FORMAT_ABGR1555:
	case DRM_FORMAT_XBGR1555:
		cntl |= CR1_BPP16_555 | CR1_BGR;
		break;
	case DRM_FORMAT_ARGB1555:
	case DRM_FORMAT_XRGB1555:
		cntl |= CR1_BPP16_555;
		break;
	default:
		WARN_ONCE(true, "Unknown FB format 0x%08x, set XRGB8888 instead\n",
				fb->format->format);
		cntl |= CR1_BPP24;
		break;
	}

	writel(priv->fb_addr, priv->regs + DBAR);
	end = ((priv->fb_addr + fb->height * fb->pitches[0] - 1) & MRR_DEAR_MRR_MASK) | \
		MRR_OUTSTND_RQ(4);

	if (priv->fb_end < end) {
		writel(end, priv->regs + MRR);
		priv->fb_end = end;
	}
	writel(cntl, priv->regs + CR1);
}

static const struct drm_plane_helper_funcs baikal_vdu_primary_plane_helper_funcs = {
	.atomic_check = baikal_vdu_primary_plane_atomic_check,
	.atomic_update = baikal_vdu_primary_plane_atomic_update,
};

static const struct drm_plane_funcs baikal_vdu_primary_plane_funcs = {
	.update_plane = drm_atomic_helper_update_plane,
	.disable_plane = drm_atomic_helper_disable_plane,
	.reset = drm_atomic_helper_plane_reset,
	.destroy = drm_plane_cleanup,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
};

int baikal_vdu_primary_plane_init(struct drm_device *drm)
{
	struct baikal_vdu_private *priv = drm->dev_private;
	struct drm_plane *plane = &priv->primary;
	static const u32 formats[] = {
		DRM_FORMAT_BGR888,
		DRM_FORMAT_RGB888,
		DRM_FORMAT_ABGR8888,
		DRM_FORMAT_XBGR8888,
		DRM_FORMAT_ARGB8888,
		DRM_FORMAT_XRGB8888,
		DRM_FORMAT_BGR565,
		DRM_FORMAT_RGB565,
		DRM_FORMAT_ABGR1555,
		DRM_FORMAT_XBGR1555,
		DRM_FORMAT_ARGB1555,
		DRM_FORMAT_XRGB1555,
	};
	int ret;

	ret = drm_universal_plane_init(drm, plane, 0,
				       &baikal_vdu_primary_plane_funcs,
				       formats,
				       ARRAY_SIZE(formats),
				       NULL,
				       DRM_PLANE_TYPE_PRIMARY,
				       NULL);
	if (ret)
		return ret;

	drm_plane_helper_add(plane, &baikal_vdu_primary_plane_helper_funcs);

	return 0;
}


