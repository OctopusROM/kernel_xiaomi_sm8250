/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _DSI_EXPOSURE_ADJUSTMENT_H_
#define _DSI_EXPOSURE_ADJUSTMENT_H_

#include <linux/types.h>

#define EA_ELVSS_OFF_THRESHOLD 1024
#define EA_PCC_MIN 580
#define EA_PCC_MAX 32768

struct dsi_panel;
struct drm_crtc;

bool ea_panel_is_enabled(struct dsi_panel *panel);
int ea_panel_mode_ctrl(struct dsi_panel *panel, bool enable);
u32 ea_panel_calc_backlight(struct dsi_panel *panel, u32 level);
u32 ea_panel_calc_backlight_for_fod_exit(struct dsi_panel *panel, u32 level);
u32 ea_panel_get_coefficient(struct drm_crtc *crtc);

#endif
