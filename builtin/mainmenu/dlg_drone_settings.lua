-- Air Minecraft - Drone Settings Dialog
-- SPDX-License-Identifier: LGPL-2.1-or-later

local function drone_settings_formspec()
	local speed = core.settings:get("drone_speed") or "20"
	local fov = core.settings:get("drone_fov") or "90"
	local show_hud = core.settings:get_bool("drone_show_hud", true)
	local collision = core.settings:get_bool("drone_collision", false)
	local auto_drone = core.settings:get_bool("drone_mode_on_start", false)

	local formspec = {
		"formspec_version[10]" ..
		"size[12,9]" ..
		"style_type[label;font=bold]" ..
		"label[0.5,0.5;Air Minecraft - " .. fgettext("Drone Settings") .. "]" ..
		"style_type[label;font=normal]"

		.. "label[0.5,1.5;" .. fgettext("Flight Speed (blocks/sec):") .. "]"
		.. "field[5,1.2;3,0.6;drone_speed;;" .. core.formspec_escape(speed) .. "]"
		.. "field_close_on_enter[drone_speed;false]"

		.. "label[0.5,2.5;" .. fgettext("Field of View (degrees):") .. "]"
		.. "field[5,2.2;3,0.6;drone_fov;;" .. core.formspec_escape(fov) .. "]"
		.. "field_close_on_enter[drone_fov;false]"

		.. "checkbox[0.5,3.3;cb_show_hud;" .. fgettext("Show Drone HUD (altitude, speed, coordinates)") .. ";" ..
		tostring(show_hud) .. "]"

		.. "checkbox[0.5,3.9;cb_collision;" .. fgettext("Enable Collision Detection") .. ";" ..
		tostring(collision) .. "]"

		.. "checkbox[0.5,4.5;cb_auto_drone;" .. fgettext("Auto-enable Drone Mode on Start") .. ";" ..
		tostring(auto_drone) .. "]"

		.. "label[0.5,5.5;" .. fgettext("Default Camera View:") .. "]"
		.. "dropdown[5,5.2;3,0.6;drone_default_view;Front,Back,Left,Right,Bottom;1]"

		.. "label[0.5,6.5;" .. fgettext("Controls:") .. "]"
		.. "label[0.5,7.0;V=" .. fgettext("Toggle") .. "  C=" .. fgettext("Cycle View") ..
		"  [/]=" .. fgettext("Switch View") ..
		"  Space=" .. fgettext("Up") .. "  Shift=" .. fgettext("Down") .. "]"

		.. "button[0.5,8;3,0.7;btn_drone_cancel;" .. fgettext("Cancel") .. "]"
		.. "button[8.5,8;3,0.7;btn_drone_save;" .. fgettext("Save") .. "]"
	}

	return table.concat(formspec, "")
end


local function drone_settings_buttonhandler(this, fields)
	if fields.btn_drone_save then
		-- Save settings
		if fields.drone_speed then
			core.settings:set("drone_speed", fields.drone_speed)
		end
		if fields.drone_fov then
			core.settings:set("drone_fov", fields.drone_fov)
		end
		if fields.cb_show_hud then
			core.settings:set_bool("drone_show_hud", core.is_yes(fields.cb_show_hud))
		end
		if fields.cb_collision then
			core.settings:set_bool("drone_collision", core.is_yes(fields.cb_collision))
		end
		if fields.cb_auto_drone then
			core.settings:set_bool("drone_mode_on_start", core.is_yes(fields.cb_auto_drone))
		end
		if fields.drone_default_view then
			core.settings:set("drone_default_view", fields.drone_default_view)
		end
		this:delete()
		return true
	elseif fields.btn_drone_cancel then
		this:delete()
		return true
	end
end


local function event_handler(event)
	if event == "DialogShow" then
		mm_game_theme.set_engine(true)
		return true
	end
	return false
end


function create_drone_settings_dlg()
	local retval = dialog_create("dlg_drone_settings",
		drone_settings_formspec,
		drone_settings_buttonhandler,
		event_handler)
	return retval
end
