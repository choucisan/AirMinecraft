// Air Minecraft
// SPDX-License-Identifier: LGPL-2.1-or-later
// Drone mode HUD overlay

#pragma once

#include "irrlichttypes.h"
#include "irr_v3d.h"
#include <IGUIFont.h>
#include <string>

class Client;
class LocalPlayer;
class Camera;

namespace video {
	class IVideoDriver;
}

class DroneHud
{
public:
	DroneHud(Client *client);
	~DroneHud() = default;

	void draw(const Camera *camera, const LocalPlayer *player);

	// Navigation target
	bool hasNavTarget() const { return m_has_nav_target; }
	v3f getNavTarget() const { return m_nav_target; }
	void setNavTarget(const v3f &pos) { m_nav_target = pos; m_has_nav_target = true; }
	void clearNavTarget() { m_has_nav_target = false; }

	// Map zoom: level 0 = default, positive = zoom in, negative = zoom out
	void zoomIn() { m_minimap_zoom = std::min<s32>(m_minimap_zoom + 1, 4); }
	void zoomOut() { m_minimap_zoom = std::max<s32>(m_minimap_zoom - 1, -4); }
	s32 getMapZoom() const { return m_minimap_zoom; }

	// Large map overlay for navigation target setting
	bool isShowingLargeMap() const { return m_show_large_map; }
	void showLargeMap() { m_show_large_map = true; }
	void hideLargeMap() { m_show_large_map = false; }
	bool handleLargeMapClick(s32 sx, s32 sy);

	// Get the current active DroneHud instance (set during rendering)
	static DroneHud *getActiveInstance() { return s_active_instance; }

private:
	void drawInfoPanel(const Camera *camera, const LocalPlayer *player);
	void drawCompass(const LocalPlayer *player);
	void drawViewIndicator(const Camera *camera);
	void drawReticle();
	void drawSpeedBar(const LocalPlayer *player);
	void drawMinimap();
	void drawLargeMap();
	void drawNavOnMinimap(s32 map_x, s32 map_y, s32 map_size, u16 zoomed_size);
	void drawPlayerMarker(s32 map_x, s32 map_y, s32 map_size);

	static const char *getCompassDirection(f32 yaw);
	static const char *getViewName(int view_index);

	Client *m_client;
	video::IVideoDriver *m_driver;
	s32 m_font_height;

	// Map state
	s32 m_minimap_size = 400;
	s32 m_minimap_zoom = 0; // 0=default, +N=zoom in, -N=zoom out
	bool m_show_large_map = false;

	// Navigation target (world coordinates)
	v3f m_nav_target;
	bool m_has_nav_target = false;

	// Active instance for cross-class access
	static DroneHud *s_active_instance;
};
