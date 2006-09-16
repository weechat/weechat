-- Author: Julien Louis <ptitlouis@sysif.net>
-- License: GPLv2
-- Description: This lua script prints in the infobar the machine load average
--
weechat.register("loadavg", "0.1", "unload", "Print the load average in infobar")

local refresh = weechat.get_config("loadavg_refresh")

if refresh == "" then
	refresh = 5
	weechat.set_config("loadavg_refresh", 5)
end
	
weechat.add_timer_handler(refresh, "loadavg")

function loadavg()
	local load = io.open("/proc/loadavg"):read()
	load = string.gsub(load, "^([%w.]+) ([%w.]+) ([%w.]+).*", "%1 %2 %3")
	weechat.print_infobar(refresh, "load: "..load)
	return weechat.PLUGIN_RC_OK;
end

function unload()
	weechat.remove_timer_handler("loadavg")
	return weechat.remove_infobar(1)
end

