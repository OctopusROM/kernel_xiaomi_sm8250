// SPDX-License-Identifier: GPL-2.0-only
#include <linux/math64.h>
#include <drm/drm_connector.h>
#include <drm/drm_modeset_lock.h>

#include "dsi_display.h"
#include "dsi_panel.h"
#include "exposure_adjustment.h"
#include "../sde/sde_color_processing.h"
#include "../sde/sde_crtc.h"

bool ea_panel_is_enabled(struct dsi_panel *panel)
{
	return panel->ea_enabled;
}

u32 ea_panel_get_coefficient(struct drm_crtc *crtc)
{
	struct dsi_display *display = get_main_display();
	struct dsi_panel *panel;
	u32 level;

	if (!display || !display->panel || !display->drm_conn ||
	    !display->drm_conn->state || display->drm_conn->state->crtc != crtc)
		return EA_PCC_MAX;

	panel = display->panel;
	level = READ_ONCE(panel->ea_last_level);
	if (!READ_ONCE(panel->ea_enabled) || !level ||
	    level >= EA_ELVSS_OFF_THRESHOLD ||
	    panel->mi_cfg.dc_enable ||
	    panel->mi_cfg.fod_hbm_enabled || panel->mi_cfg.hbm_enabled)
		return EA_PCC_MAX;

	return EA_PCC_MIN + div_u64((u64)level * (EA_PCC_MAX - EA_PCC_MIN),
				   EA_ELVSS_OFF_THRESHOLD);
}

u32 ea_panel_calc_backlight(struct dsi_panel *panel, u32 level)
{
	WRITE_ONCE(panel->ea_last_level, level);
	if (READ_ONCE(panel->ea_enabled) && level &&
	    level < EA_ELVSS_OFF_THRESHOLD && !panel->mi_cfg.dc_enable &&
	    !panel->mi_cfg.fod_hbm_enabled && !panel->mi_cfg.hbm_enabled)
		return EA_ELVSS_OFF_THRESHOLD;
	return level;
}

int ea_panel_mode_ctrl(struct dsi_panel *panel, bool enable)
{
	struct dsi_display *display = get_main_display();
	struct drm_crtc *crtc;
	bool previous;
	int rc;

	if (!display || !display->drm_dev || !display->drm_conn ||
	    display->panel != panel || !panel->panel_initialized)
		return -ENODEV;
	previous = READ_ONCE(panel->ea_enabled);
	if (previous == enable)
		return 0;

	drm_modeset_lock_all(display->drm_dev);
	if (!display->drm_conn->state ||
	    !(crtc = display->drm_conn->state->crtc) ||
	    !sde_cp_crtc_has_pcc(crtc)) {
		drm_modeset_unlock_all(display->drm_dev);
		return -ENODEV;
	}

	WRITE_ONCE(panel->ea_enabled, enable);
	sde_cp_crtc_update_ea(crtc);
	drm_modeset_unlock_all(display->drm_dev);

	rc = dsi_display_set_backlight(display->drm_conn, display,
				       panel->bl_config.bl_level);
	if (rc) {
		WRITE_ONCE(panel->ea_enabled, previous);
		drm_modeset_lock_all(display->drm_dev);
		if (display->drm_conn->state && display->drm_conn->state->crtc)
			sde_cp_crtc_update_ea(display->drm_conn->state->crtc);
		drm_modeset_unlock_all(display->drm_dev);
		return rc;
	}
	return 0;
}
