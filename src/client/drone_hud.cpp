// Air Minecraft
// SPDX-License-Identifier: LGPL-2.1-or-later
// Drone mode HUD overlay

#include "drone_hud.h"
#include "client.h"
#include "camera.h"
#include "localplayer.h"
#include "minimap.h"
#include "fontengine.h"
#include "renderingengine.h"
#include "settings.h"
#include "constants.h"
#include "util/numeric.h"
#include <IGUIFont.h>
#include <IVideoDriver.h>
#include <IGUIEnvironment.h>
#include <IGUIStaticText.h>
#include <ICameraSceneNode.h>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

// Minecraft pixel-style colors
static const video::SColor PANEL_BG(160, 30, 30, 30);
static const video::SColor PANEL_BORDER_OUTER(200, 50, 50, 50);
static const video::SColor PANEL_BORDER_INNER(200, 160, 160, 160);
static const video::SColor TEXT_COLOR(255, 220, 220, 220);
static const video::SColor TEXT_COLOR_DIM(200, 160, 160, 160);
static const video::SColor TEXT_COLOR_WARN(255, 255, 80, 80);
static const video::SColor RETICLE_COLOR(220, 255, 255, 255);
static const video::SColor COMPASS_COLOR(200, 255, 255, 255);
static const video::SColor NAV_COLOR(200, 255, 255, 0);
static const video::SColor MARKER_PLAYER(255, 255, 255, 255);
static const video::SColor MARKER_TARGET(255, 255, 80, 80);

DroneHud *DroneHud::s_active_instance = nullptr;

DroneHud::DroneHud(Client *client)
	: m_client(client)
{
	m_driver = RenderingEngine::get_video_driver();
	m_font_height = g_fontengine->getDefaultFontSize();
}

const char *DroneHud::getCompassDirection(f32 yaw)
{
	yaw = wrapDegrees_0_360(yaw);
	int sector = static_cast<int>((yaw + 22.5f) / 45.0f) % 8;
	static const char *dirs[8] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
	return dirs[sector];
}

const char *DroneHud::getViewName(int view_index)
{
	static const char *names[5] = {"FRONT", "BACK", "LEFT", "RIGHT", "BOTTOM"};
	if (view_index >= 0 && view_index < 5)
		return names[view_index];
	return "???";
}

void DroneHud::draw(const Camera *camera, const LocalPlayer *player)
{
	if (!camera || !player)
		return;

	s_active_instance = this;

	// Large map overlay mode: draw fullscreen map and return
	if (m_show_large_map) {
		drawLargeMap();
		return;
	}

	drawReticle();
	drawCompass(player);
	drawViewIndicator(camera);

	if (g_settings->getBool("drone_show_info_panel")) {
		drawInfoPanel(camera, player);
		drawSpeedBar(player);
		drawMinimap();
	}
}

void DroneHud::drawReticle()
{
	v2u32 ss = RenderingEngine::getWindowSize();
	s32 cx = ss.X / 2;
	s32 cy = ss.Y / 2;
	s32 size = 12;
	s32 gap = 4;

	m_driver->draw2DLine(v2s32(cx - size, cy), v2s32(cx - gap, cy), RETICLE_COLOR);
	m_driver->draw2DLine(v2s32(cx + gap, cy), v2s32(cx + size, cy), RETICLE_COLOR);
	m_driver->draw2DLine(v2s32(cx, cy - size), v2s32(cx, cy - gap), RETICLE_COLOR);
	m_driver->draw2DLine(v2s32(cx, cy + gap), v2s32(cx, cy + size), RETICLE_COLOR);

	m_driver->draw2DRectangle(RETICLE_COLOR,
		core::rect<s32>(cx - 1, cy - 1, cx + 2, cy + 2));
}

void DroneHud::drawInfoPanel(const Camera *camera, const LocalPlayer *player)
{
	gui::IGUIFont *font = g_fontengine->getFont();
	if (!font)
		return;

	v2u32 ss = RenderingEngine::getWindowSize();
	v3f pos = player->getPosition();
	v3f vel = player->last_speed;
	f32 speed = vel.getLength();
	f32 yaw = player->getYaw();
	f32 pitch = player->getPitch();
	f32 roll = player->getRoll();
	f32 altitude = pos.Y;

	std::wostringstream os;
	os << std::fixed << std::setprecision(1);

	os << L"ALT  " << std::setw(8) << altitude << L" m\n";
	os << L"SPD  " << std::setw(8) << speed << L" m/s\n";
	os << L"X    " << std::setw(8) << pos.X << L"\n";
	os << L"Y    " << std::setw(8) << pos.Y << L"\n";
	os << L"Z    " << std::setw(8) << pos.Z << L"\n";
	os << L"YAW  " << std::setw(8) << yaw << L"\xC2\xB0\n";
	os << L"PITCH" << std::setw(8) << pitch << L"\xC2\xB0\n";
	os << L"ROLL " << std::setw(8) << roll << L"\xC2\xB0\n";

	// Navigation info
	if (m_has_nav_target) {
		// Horizontal distance (XZ plane only, ignore altitude)
		f32 dx = m_nav_target.X - pos.X;
		f32 dz = m_nav_target.Z - pos.Z;
		f32 nav_dist = std::sqrt(dx * dx + dz * dz);
		os << L"NAV  " << std::setw(8) << nav_dist << L" m\n";
		os << L"TGT  " << std::setw(8) << m_nav_target.X / BS
		   << L"," << m_nav_target.Z / BS << L"\n";
	}

	std::wstring text = os.str();

	u32 line_height = g_fontengine->getLineHeight();
	u32 text_width = font->getDimension(text.c_str()).Width;
	s32 panel_w = text_width + 16;
	int line_count = 8 + (m_has_nav_target ? 2 : 0);
	s32 panel_h = line_height * line_count + 12;

	s32 panel_x = ss.X - panel_w - 10;
	s32 panel_y = 10;

	core::rect<s32> panel_rect(panel_x, panel_y, panel_x + panel_w, panel_y + panel_h);
	m_driver->draw2DRectangle(PANEL_BG, panel_rect);

	// Pixel-style double border: outer dark + inner highlight
	// Top
	m_driver->draw2DRectangle(PANEL_BORDER_OUTER, core::rect<s32>(panel_x, panel_y, panel_x + panel_w, panel_y + 2));
	m_driver->draw2DRectangle(PANEL_BORDER_INNER, core::rect<s32>(panel_x + 2, panel_y + 2, panel_x + panel_w - 2, panel_y + 3));
	// Bottom
	m_driver->draw2DRectangle(PANEL_BORDER_INNER, core::rect<s32>(panel_x, panel_y + panel_h - 3, panel_x + panel_w, panel_y + panel_h - 2));
	m_driver->draw2DRectangle(PANEL_BORDER_OUTER, core::rect<s32>(panel_x, panel_y + panel_h - 2, panel_x + panel_w, panel_y + panel_h));
	// Left
	m_driver->draw2DRectangle(PANEL_BORDER_OUTER, core::rect<s32>(panel_x, panel_y, panel_x + 2, panel_y + panel_h));
	m_driver->draw2DRectangle(PANEL_BORDER_INNER, core::rect<s32>(panel_x + 2, panel_y + 2, panel_x + 3, panel_y + panel_h - 2));
	// Right
	m_driver->draw2DRectangle(PANEL_BORDER_INNER, core::rect<s32>(panel_x + panel_w - 3, panel_y + 2, panel_x + panel_w - 2, panel_y + panel_h - 2));
	m_driver->draw2DRectangle(PANEL_BORDER_OUTER, core::rect<s32>(panel_x + panel_w - 2, panel_y, panel_x + panel_w, panel_y + panel_h));

	core::rect<s32> text_rect(panel_x + 8, panel_y + 6, panel_x + panel_w, panel_y + panel_h);
	font->draw(text.c_str(), text_rect, TEXT_COLOR, false, false, nullptr);

	std::wstring label = L"DRONE HUD";
	u32 label_w = font->getDimension(label.c_str()).Width;
	s32 label_x = panel_x + (panel_w - label_w) / 2;
	font->draw(label.c_str(),
		core::rect<s32>(label_x, panel_y - line_height - 2, label_x + label_w, panel_y),
		PANEL_BORDER_OUTER, false, false, nullptr);
}

void DroneHud::drawCompass(const LocalPlayer *player)
{
	gui::IGUIFont *font = g_fontengine->getFont();
	if (!font)
		return;

	v2u32 ss = RenderingEngine::getWindowSize();
	f32 yaw = player->getYaw();
	const char *dir = getCompassDirection(yaw);

	std::wostringstream os;
	os << dir << L" " << std::fixed << std::setprecision(0)
	   << wrapDegrees_0_360(yaw) << L"\xC2\xB0";
	std::wstring text = os.str();

	u32 text_w = font->getDimension(text.c_str()).Width;
	s32 x = (ss.X - text_w) / 2;
	s32 y = 10;

	m_driver->draw2DRectangle(PANEL_BG,
		core::rect<s32>(x - 8, y - 2, x + text_w + 8, y + m_font_height + 6));
	m_driver->draw2DRectangle(PANEL_BORDER_OUTER,
		core::rect<s32>(x - 8, y - 2, x + text_w + 8, y));
	m_driver->draw2DRectangle(PANEL_BORDER_INNER,
		core::rect<s32>(x - 6, y, x + text_w + 6, y + 1));

	font->draw(text.c_str(),
		core::rect<s32>(x, y, x + text_w, y + m_font_height),
		COMPASS_COLOR, false, false, nullptr);
}

void DroneHud::drawViewIndicator(const Camera *camera)
{
	gui::IGUIFont *font = g_fontengine->getFont();
	if (!font)
		return;

	v2u32 ss = RenderingEngine::getWindowSize();
	LocalPlayer *player = m_client->getEnv().getLocalPlayer();
	int view_index = player->drone_view_index;
	const char *view_name = getViewName(view_index);

	std::wstring text = L"[";
	text += utf8_to_wide(view_name);
	text += L"]";

	u32 text_w = font->getDimension(text.c_str()).Width;
	s32 x = (ss.X - text_w) / 2;
	s32 y = ss.Y - m_font_height - 30;

	m_driver->draw2DRectangle(PANEL_BG,
		core::rect<s32>(x - 8, y - 2, x + text_w + 8, y + m_font_height + 6));
	m_driver->draw2DRectangle(PANEL_BORDER_OUTER,
		core::rect<s32>(x - 8, y + m_font_height + 4, x + text_w + 8, y + m_font_height + 6));
	m_driver->draw2DRectangle(PANEL_BORDER_INNER,
		core::rect<s32>(x - 6, y + m_font_height + 3, x + text_w + 6, y + m_font_height + 4));

	font->draw(text.c_str(),
		core::rect<s32>(x, y, x + text_w, y + m_font_height),
		TEXT_COLOR, false, false, nullptr);
}

void DroneHud::drawSpeedBar(const LocalPlayer *player)
{
	v2u32 ss = RenderingEngine::getWindowSize();
	v3f vel = player->last_speed;
	f32 speed = vel.getLength();

	f32 max_speed = 30.0f;
	f32 ratio = speed / max_speed;
	if (ratio > 1.0f) ratio = 1.0f;

	s32 bar_w = 120;
	s32 bar_h = 6;
	s32 bar_x = 10;
	s32 bar_y = ss.Y - bar_h - 10;

	m_driver->draw2DRectangle(PANEL_BG,
		core::rect<s32>(bar_x, bar_y, bar_x + bar_w, bar_y + bar_h));

	video::SColor bar_color = speed > 20.0f ? TEXT_COLOR_WARN : TEXT_COLOR;
	s32 fill_w = static_cast<s32>(bar_w * ratio);
	if (fill_w > 0) {
		m_driver->draw2DRectangle(bar_color,
			core::rect<s32>(bar_x, bar_y, bar_x + fill_w, bar_y + bar_h));
	}

	m_driver->draw2DRectangle(PANEL_BORDER_OUTER,
		core::rect<s32>(bar_x, bar_y, bar_x + bar_w, bar_y + 1));
	m_driver->draw2DRectangle(PANEL_BORDER_OUTER,
		core::rect<s32>(bar_x, bar_y + bar_h - 1, bar_x + bar_w, bar_y + bar_h));
	m_driver->draw2DRectangle(PANEL_BORDER_OUTER,
		core::rect<s32>(bar_x, bar_y, bar_x + 1, bar_y + bar_h));
	m_driver->draw2DRectangle(PANEL_BORDER_OUTER,
		core::rect<s32>(bar_x + bar_w - 1, bar_y, bar_x + bar_w, bar_y + bar_h));

	gui::IGUIFont *font = g_fontengine->getFont();
	if (font) {
		std::wostringstream os;
		os << std::fixed << std::setprecision(1) << speed << L" m/s";
		std::wstring label = os.str();
		font->draw(label.c_str(),
			core::rect<s32>(bar_x, bar_y - m_font_height - 2,
				bar_x + bar_w, bar_y),
			TEXT_COLOR_DIM, false, false, nullptr);
	}
}

void DroneHud::drawMinimap()
{
	Minimap *minimap = m_client->getMinimap();
	if (!minimap || !minimap->getMinimapTexture())
		return;

	// Only drone HUD draws the minimap; suppress built-in HUD
	minimap->m_allow_draw = true;

	s32 map_size = m_minimap_size;
	s32 map_x = 10;
	s32 map_y = 10;

	// Pixel-style border
	s32 b = 2; // border thickness
	m_driver->draw2DRectangle(PANEL_BORDER_OUTER,
		core::rect<s32>(map_x - b, map_y - b, map_x + map_size + b, map_y + map_size + b));
	m_driver->draw2DRectangle(PANEL_BORDER_INNER,
		core::rect<s32>(map_x - 1, map_y - 1, map_x + map_size + 1, map_y + map_size + 1));
	m_driver->draw2DRectangle(PANEL_BG,
		core::rect<s32>(map_x, map_y, map_x + map_size, map_y + map_size));

	// Apply zoom: zoom in = fewer nodes (more detail), zoom out = more nodes (see more)
	u16 base_size = 128;
	u16 zoomed_size;
	if (m_minimap_zoom >= 0)
		zoomed_size = std::max<u16>(32, base_size >> m_minimap_zoom);
	else
		zoomed_size = std::min<u16>(512, base_size << (-m_minimap_zoom));
	if (minimap->data->mode.map_size != zoomed_size) {
		minimap->data->mode.map_size = zoomed_size;
		minimap->data->map_invalidated = true;
	}
	minimap->drawMinimap(core::rect<s32>(map_x, map_y, map_x + map_size, map_y + map_size));

	// Draw navigation line on minimap if target is set
	if (m_has_nav_target) {
		drawNavOnMinimap(map_x, map_y, map_size, zoomed_size);
	}

	// Draw player marker on minimap
	drawPlayerMarker(map_x, map_y, map_size);

	// Suppress built-in HUD minimap after drone HUD draws
	minimap->m_allow_draw = false;
}

void DroneHud::drawLargeMap()
{
	Minimap *minimap = m_client->getMinimap();
	if (!minimap || !minimap->getMinimapTexture())
		return;

	minimap->m_allow_draw = true;

	v2u32 ss = RenderingEngine::getWindowSize();

	// Dark overlay
	m_driver->draw2DRectangle(video::SColor(220, 0, 0, 0),
		core::rect<s32>(0, 0, ss.X, ss.Y));

	// Large map: 80% of screen, centered
	s32 map_size = std::min<s32>(ss.X * 4 / 5, ss.Y * 4 / 5);
	map_size = std::min<s32>(map_size, 1200);
	s32 map_x = (ss.X - map_size) / 2;
	s32 map_y = (ss.Y - map_size) / 2;

	// Border
	s32 b = 2;
	m_driver->draw2DRectangle(PANEL_BORDER_OUTER,
		core::rect<s32>(map_x - b, map_y - b, map_x + map_size + b, map_y + map_size + b));
	m_driver->draw2DRectangle(PANEL_BORDER_INNER,
		core::rect<s32>(map_x - 1, map_y - 1, map_x + map_size + 1, map_y + map_size + 1));
	m_driver->draw2DRectangle(video::SColor(255, 30, 30, 30),
		core::rect<s32>(map_x, map_y, map_x + map_size, map_y + map_size));

	// Apply zoom
	u16 base_size = 128;
	u16 zoomed_size;
	if (m_minimap_zoom >= 0)
		zoomed_size = std::max<u16>(32, base_size >> m_minimap_zoom);
	else
		zoomed_size = std::min<u16>(512, base_size << (-m_minimap_zoom));
	if (minimap->data->mode.map_size != zoomed_size) {
		minimap->data->mode.map_size = zoomed_size;
		minimap->data->map_invalidated = true;
	}
	minimap->drawMinimap(core::rect<s32>(map_x, map_y, map_x + map_size, map_y + map_size));

	// Draw nav line and markers
	if (m_has_nav_target)
		drawNavOnMinimap(map_x, map_y, map_size, zoomed_size);
	drawPlayerMarker(map_x, map_y, map_size);

	// Draw target marker at cursor position (if hovering)
	// Show hint text
	gui::IGUIFont *font = g_fontengine->getFont();
	if (font) {
		LocalPlayer *player = m_client->getEnv().getLocalPlayer();
		std::wstring hint;
		if (m_has_nav_target && player) {
			// Both m_nav_target and player->getPosition() are in world (BS) units
			// Use horizontal (XZ) distance only
			v3f pos = player->getPosition();
			f32 dx = m_nav_target.X - pos.X;
			f32 dz = m_nav_target.Z - pos.Z;
			f32 dist = std::sqrt(dx * dx + dz * dz);
			std::wostringstream os;
			os << L"Target: (" << std::fixed << std::setprecision(1)
			   << m_nav_target.X / BS << L", " << m_nav_target.Z / BS
			   << L")  Dist: " << dist << L"m";
			hint = os.str();
		} else {
			hint = L"Click to set target | ESC to close | +/- to zoom";
		}
		u32 hint_w = font->getDimension(hint.c_str()).Width;
		font->draw(hint.c_str(),
			core::rect<s32>((ss.X - hint_w) / 2, map_y + map_size + 8,
				(ss.X + hint_w) / 2, map_y + map_size + 8 + m_font_height),
			TEXT_COLOR, false, false, nullptr);
	}

	minimap->m_allow_draw = false;
}

bool DroneHud::handleLargeMapClick(s32 sx, s32 sy)
{
	if (!m_show_large_map)
		return false;

	Minimap *minimap = m_client->getMinimap();
	if (!minimap)
		return false;

	v2u32 ss = RenderingEngine::getWindowSize();
	s32 map_size = std::min<s32>(ss.X * 4 / 5, ss.Y * 4 / 5);
	map_size = std::min<s32>(map_size, 1200);
	s32 map_x = (ss.X - map_size) / 2;
	s32 map_y = (ss.Y - map_size) / 2;

	if (sx < map_x || sx > map_x + map_size || sy < map_y || sy > map_y + map_size)
		return false;

	// Convert screen coords to world coords, accounting for minimap rotation
	f32 offset_x = (sx - map_x - map_size / 2.0f);
	f32 offset_z = (sy - map_y - map_size / 2.0f);

	v3s16 minimap_pos = minimap->getPos();
	u16 base_size = 128;
	u16 zoomed_size;
	if (m_minimap_zoom >= 0)
		zoomed_size = std::max<u16>(32, base_size >> m_minimap_zoom);
	else
		zoomed_size = std::min<u16>(512, base_size << (-m_minimap_zoom));
	f32 pixels_per_node = map_size / (zoomed_size * 1.0f);

	// Forward: screen_dx = dx*cos(a) + dz*sin(a), screen_dy = dx*sin(a) - dz*cos(a)
	// Inverse (matrix is its own inverse):
	// dx = screen_dx*cos(a) + screen_dy*sin(a)
	// dz = screen_dx*sin(a) - screen_dy*cos(a)
	f32 angle_rad = minimap->getAngle() * core::DEGTORAD;
	f32 cos_a = std::cos(angle_rad);
	f32 sin_a = std::sin(angle_rad);
	f32 world_offset_x = (offset_x * cos_a + offset_z * sin_a) / pixels_per_node;
	f32 world_offset_z = (offset_x * sin_a - offset_z * cos_a) / pixels_per_node;

	LocalPlayer *player = m_client->getEnv().getLocalPlayer();

	// Store in world (BS) coordinates - same space as player->getPosition()
	// minimap_pos and world_offset are in node coordinates, multiply by BS to get world coords
	m_nav_target = v3f(
		(minimap_pos.X + world_offset_x) * BS,
		player ? player->getPosition().Y : 0,
		(minimap_pos.Z + world_offset_z) * BS);
	m_has_nav_target = true;

	return true;
}

void DroneHud::drawNavOnMinimap(s32 map_x, s32 map_y, s32 map_size, u16 zoomed_size)
{
	if (!m_has_nav_target)
		return;

	Minimap *minimap = m_client->getMinimap();
	if (!minimap)
		return;

	LocalPlayer *player = m_client->getEnv().getLocalPlayer();
	if (!player)
		return;

	v3s16 minimap_pos = minimap->getPos();

	// m_nav_target is in world (BS) coords, minimap_pos is in node coords
	// Convert m_nav_target to node coords by dividing by BS
	f32 dx = m_nav_target.X / BS - minimap_pos.X;
	f32 dz = m_nav_target.Z / BS - minimap_pos.Z;

	// Derived from mesh buffer: UV(0,1)=NW, UV(1,1)=NE, UV(0,0)=SW, UV(1,0)=SE
	// Texture: +X(east)=right(U), +Z(north)=bottom(V=1)
	// Mesh: mx=2*U-1, my=1-2*V → my≈-2*dz (north=my<0)
	// Rotation by (360-a): screen = R * mesh
	// screen_x = dx*cos(a) + dz*sin(a)
	// screen_y = dx*sin(a) - dz*cos(a)
	f32 angle_rad = minimap->getAngle() * core::DEGTORAD;
	f32 cos_a = std::cos(angle_rad);
	f32 sin_a = std::sin(angle_rad);
	f32 screen_dx = dx * cos_a + dz * sin_a;
	f32 screen_dy = dx * sin_a - dz * cos_a;

	// Convert to screen pixels using the actual zoomed map size
	f32 pixels_per_node = map_size / (zoomed_size * 1.0f);
	f32 map_center_x = map_x + map_size / 2.0f;
	f32 map_center_y = map_y + map_size / 2.0f;

	s32 px = static_cast<s32>(map_center_x);
	s32 py = static_cast<s32>(map_center_y);
	s32 tx = static_cast<s32>(map_center_x + screen_dx * pixels_per_node);
	s32 ty = static_cast<s32>(map_center_y + screen_dy * pixels_per_node);

	// Clamp to minimap bounds
	tx = std::clamp(tx, map_x + 2, map_x + map_size - 2);
	ty = std::clamp(ty, map_y + 2, map_y + map_size - 2);

	// Draw navigation line
	m_driver->draw2DLine(v2s32(px, py), v2s32(tx, ty), NAV_COLOR);

	// Draw target marker on minimap
	s32 marker = 4;
	m_driver->draw2DRectangle(MARKER_TARGET,
		core::rect<s32>(tx - marker, ty - marker, tx + marker, ty + marker));
}

void DroneHud::drawPlayerMarker(s32 map_x, s32 map_y, s32 map_size)
{
	// Player is always at the center of the minimap
	s32 cx = map_x + map_size / 2;
	s32 cy = map_y + map_size / 2;
	s32 marker = 3;
	m_driver->draw2DRectangle(MARKER_PLAYER,
		core::rect<s32>(cx - marker, cy - marker, cx + marker, cy + marker));
}

