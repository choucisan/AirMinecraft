-- Air Minecraft - Drone Mode Tab
-- SPDX-License-Identifier: LGPL-2.1-or-later

local function current_game()
	local gameid = core.settings:get("menu_last_game")
	local game = gameid and pkgmgr.find_by_gameid(gameid)
	if not game and #pkgmgr.games > 0 then
		local picked_game
		if pkgmgr.games[1].id == "devtest" and #pkgmgr.games > 1 then
			picked_game = 2
		else
			picked_game = 1
		end
		game = pkgmgr.games[picked_game]
		gameid = game.id
		core.settings:set("menu_last_game", gameid)
	end
	return game
end

local function apply_game(game)
	core.settings:set("menu_last_game", game.id)
	menudata.worldlist:set_filtercriteria(game.id)
	mm_game_theme.set_game(game)
end

local function get_formspec(tabview, name, tabdata)
	if #pkgmgr.games == 0 then
		local W = tabview.width
		local H = tabview.height
		local hypertext = "<global valign=middle halign=center size=18>" ..
				fgettext_ne("Welcome to Air Minecraft!") .. "\n" ..
				fgettext_ne("Please install a game to start drone flight.")
		local button_y = H * 2/3 - 0.6
		return table.concat({
			"hypertext[0.375,0;", W - 2*0.375, ",", button_y, ";ht;",
			core.formspec_escape(hypertext), "]",
			"button[5.25,", button_y, ";5,1.2;game_open_cdb;",
			fgettext("Install a game"), "]"
		})
	end

	local retval = ""

	local index = core.get_textlist_index("drone_worlds") or
		filterlist.get_current_index(menudata.worldlist,
			tonumber(core.settings:get("mainmenu_last_selected_world"))) or 0

	local list = menudata.worldlist:get_list()
	local world = list and list[math.min(index, #list)]

	-- Title
	retval = retval ..
		"label[0.375,0.2;Air Minecraft - " .. fgettext("Drone Mode") .. "]" ..
		"label[0.375,0.6;" .. fgettext("Free 3D flight with 5 directional cameras for RL research") .. "]"

	-- World list
	retval = retval ..
		"container[0.375,1.2]" ..
		"label[0,0;" .. fgettext("Select World:") .. "]" ..
		"textlist[0,0.3;9.875,4.0;drone_worlds;" ..
		menu_render_worldlist() ..
		";" .. index .. "]" ..
		"container_end[]"

	-- Buttons row
	retval = retval ..
		"container[0.375,5.8]" ..
		"button[0,0;2.4,0.8;drone_settings;".. fgettext("Settings") .. "]" ..
		"button[2.5,0;2.4,0.8;world_create;".. fgettext("New World") .. "]" ..
		"button[5.0,0;2.4,0.8;world_delete;".. fgettext("Delete") .. "]" ..
		"button[7.5,0;2.4,0.8;world_configure;".. fgettext("Select Mods") .. "]" ..
		"container_end[]"

	-- Controls panel (right side)
	retval = retval ..
		"container[10.5,0.2]" ..
		"box[0,0;4.8,5.0;#00000080]" ..
		"label[0.2,0.25;" .. fgettext("Controls:") .. "]" ..
		
		"label[0.2,0.85;C - " .. fgettext("Switch 5 Views") .. "]" ..
		"label[0.2,1.15;T - " .. fgettext("1st / 3rd Person") .. "]" ..
		"label[0.2,1.45;WASD - " .. fgettext("Move") .. "]" ..
		"label[0.2,1.75;Space - " .. fgettext("Ascend") .. "]" ..
		"label[0.2,2.05;Shift - " .. fgettext("Descend") .. "]" ..
		"label[0.2,2.35;R / F - " .. fgettext("Roll Left / Right") .. "]" ..
		"label[0.2,2.65;H - " .. fgettext("Toggle HUD Panel") .. "]" ..
		"label[0.2,2.95;+ / - - " .. fgettext("Map Zoom In/Out") .. "]" ..
		"label[0.2,3.35;" .. fgettext("Views:") .. "]" ..
		"label[0.2,3.65;Front | Back | Left | Right | Bottom]" ..
		"label[0.2,4.05;" .. fgettext("Navigation:") .. "]" ..
		"label[0.2,4.35;" .. fgettext("ESC Menu > Set Nav Target") .. "]" ..
		"label[0.2,4.65;" .. fgettext("Coords: Node (X,Y,Z)") .. "]"
	retval = retval .. "container_end[]"

	-- Start button (prominent, right side)
	if world then
		retval = retval ..
			"style[play_drone;font_size=+2;bgcolor=#006600]" ..
			"button[10.5,5.5;4.9375,0.9;play_drone;" ..
			fgettext("Start Drone Flight") .. "]"
	end

	return retval
end

local function main_button_handler(this, fields, name, tabdata)
	assert(name == "drone")

	if fields.drone_settings then
		local maintab = ui.find_by_name("maintab")
		local dlg = create_drone_settings_dlg()
		dlg:set_parent(maintab)
		maintab:hide()
		dlg:show()
		return true
	end

	if fields.game_open_cdb or fields.drone_game_open_cdb then
		local maintab = ui.find_by_name("maintab")
		local dlg = create_contentdb_dlg("game")
		dlg:set_parent(maintab)
		maintab:hide()
		dlg:show()
		return true
	end

	if this.dlg_create_world_closed_at == nil then
		this.dlg_create_world_closed_at = 0
	end

	local world_doubleclick = false

	if fields["drone_worlds"] ~= nil then
		local event = core.explode_textlist_event(fields["drone_worlds"])
		local selected = core.get_textlist_index("drone_worlds")

		menu_worldmt_legacy(selected)

		if event.type == "DCL" then
			world_doubleclick = true
		end

		if event.type == "CHG" and selected ~= nil then
			core.settings:set("mainmenu_last_selected_world",
				menudata.worldlist:get_raw_index(selected))
			return true
		end
	end

	if menu_handle_key_up_down(fields, "drone_worlds", "mainmenu_last_selected_world") then
		return true
	end

	if fields["play_drone"] ~= nil or world_doubleclick then
		local selected = core.get_textlist_index("drone_worlds")
		gamedata.selected_world = menudata.worldlist:get_raw_index(selected)

		if selected == nil or gamedata.selected_world == 0 then
			return true
		end

		-- Enable drone mode on start
		core.settings:set("drone_mode_on_start", "true")

		local world = menudata.worldlist:get_raw_element(gamedata.selected_world)
		if world then
			local game_obj = pkgmgr.find_by_gameid(world.gameid)
			if game_obj then
				core.settings:set("menu_last_game", game_obj.id)
			end
		end

		gamedata.singleplayer = true
		core.start()
		return true
	end

	if fields["world_create"] ~= nil then
		this.dlg_create_world_closed_at = 0
		local create_world_dlg = create_create_world_dlg()
		create_world_dlg:set_parent(this)
		this:hide()
		create_world_dlg:show()
		return true
	end

	if fields["world_delete"] ~= nil then
		local selected = core.get_textlist_index("drone_worlds")
		if selected ~= nil and
			selected <= menudata.worldlist:size() then
			local world = menudata.worldlist:get_list()[selected]
			if world ~= nil and
				world.name ~= nil and
				world.name ~= "" then
				local idx = menudata.worldlist:get_raw_index(selected)
				local delete_world_dlg = create_delete_world_dlg(world.name, idx)
				delete_world_dlg:set_parent(this)
				this:hide()
				delete_world_dlg:show()
			end
		end
		return true
	end

	if fields["world_configure"] ~= nil then
		local selected = core.get_textlist_index("drone_worlds")
		if selected ~= nil then
			local configdialog =
				create_configure_world_dlg(
					menudata.worldlist:get_raw_index(selected))

			if (configdialog ~= nil) then
				configdialog:set_parent(this)
				this:hide()
				configdialog:show()
			end
		end
		return true
	end
end

local function refresh_gamebar()
	local old_bar = ui.find_by_name("drone_game_button_bar")
	if old_bar ~= nil then
		old_bar:delete()
	end

	if #pkgmgr.games == 0 then
		return false
	end

	local function gamebar_button_handler(fields)
		for _, game in ipairs(pkgmgr.games) do
			if fields["drone_game_btnbar_" .. game.id] then
				apply_game(game)
				return true
			end
		end
	end

	local TOUCH_GUI = core.settings:get_bool("touch_gui")
	local gamebar_pos_y = MAIN_TAB_H
		+ TABHEADER_H
		+ (TOUCH_GUI and GAMEBAR_OFFSET_TOUCH or GAMEBAR_OFFSET_DESKTOP)

	local btnbar = buttonbar_create(
			"drone_game_button_bar",
			{x = 0, y = gamebar_pos_y},
			{x = MAIN_TAB_W, y = GAMEBAR_H},
			"#000000",
			gamebar_button_handler)

	for _, game in ipairs(pkgmgr.games) do
		local btn_name = "drone_game_btnbar_" .. game.id
		local image = nil
		local text = nil
		local tooltip = core.formspec_escape(game.title)

		if (game.menuicon_path or "") ~= "" then
			image = core.formspec_escape(game.menuicon_path)
		else
			local part1 = game.id:sub(1,5)
			local part2 = game.id:sub(6,10)
			local part3 = game.id:sub(11)
			text = part1 .. "\n" .. part2
			if part3 ~= "" then
				text = text .. "\n" .. part3
			end
		end
		btnbar:add_button(btn_name, text, image, tooltip)
	end

	local plus_image = core.formspec_escape(defaulttexturedir .. "plus.png")
	btnbar:add_button("drone_game_open_cdb", "", plus_image, fgettext("Install games from ContentDB"))
	return true
end

local function on_change(type)
	if type == "ENTER" then
		local game = current_game()
		if game then
			apply_game(game)
		else
			mm_game_theme.set_engine()
		end

		if refresh_gamebar() then
			ui.find_by_name("drone_game_button_bar"):show()
		end
	elseif type == "LEAVE" then
		menudata.worldlist:set_filtercriteria(nil)
		local gamebar = ui.find_by_name("drone_game_button_bar")
		if gamebar then
			gamebar:hide()
		end
	end
end

--------------------------------------------------------------------------------
return {
	name = "drone",
	caption = fgettext("Drone Mode"),
	cbf_formspec = get_formspec,
	cbf_button_handler = main_button_handler,
	on_change = on_change
}
