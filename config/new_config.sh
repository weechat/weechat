
/mouse enable

# set aspell

/set aspell.check.default_dict en
/set aspell.check.suggestions 3
/set aspell.color.suggestion *green
/aspell enable

# set bar buflist

/set irc.look.server_buffer independent
/set buflist.format.buffer "${format_number}${indent}${eval:${format_name}}${format_hotlist} ${color:31}${buffer.local_variables.filter}${buffer.local_variables.conky_Load_Average}${buffer.local_variables.weather}"
/set buflist.format.buffer_current ${if:${type}==server?${color:*white,31}:${color:*white}}${hide:>,${buffer[last_gui_buffer].number}} ${indent}${if:${type}==server&&${info:irc_server_isupport_value,${name},NETWORK}?${info:irc_server_isupport_value,${name},NETWORK}:${name}} ${color:31}${buffer.local_variables.filter}${buffer.local_variables.conky_Load_Average}${buffer.local_variables.weather}
/set buflist.format.hotlist " ${color:239}${hotlist}${color:239}"
/set buflist.format.hotlist_highlight "${color:163}"
/set buflist.format.hotlist_message "${color:229}"
/set buflist.format.hotlist_private "${color:121}"
/set buflist.format.indent "${if:${type}==channel&&${buffer.name}=~fr$||${info:aspell_dict,${buffer.full_name}}=~fr?${color:blue}f :  }${color:*white}"
/set buflist.format.name "${if:${type}==server?${color:white}:${color_hotlist}}${if:${type}==server||${type}==channel||${type}==private?${if:${cutscr:8,+,${name}}!=${name}?${cutscr:8,${color:${weechat.color.chat_prefix_more}}+,${if:${type}==server&&${info:irc_server_isupport_value,${name},NETWORK}?${info:irc_server_isupport_value,${name},NETWORK}:${name}}}:${cutscr:8, ,${if:${type}==server&&${info:irc_server_isupport_value,${name},NETWORK}?${info:irc_server_isupport_value,${name},NETWORK}                              :${name}                              }}}:${name}}"
/set buflist.format.number "${if:${type}==server?${color:black,31}:${color:239}}${if:${number_displayed}?.: }"
/set weechat.bar.buflist.size 18
/set weechat.bar.buflist.size_max 18
/bar add activetitle window top 1 0 buffer_title
/set weechat.bar.activetitle.priority 500
/set weechat.bar.activetitle.conditions "${active}"
/set weechat.bar.activetitle.color_fg white
/set weechat.bar.activetitle.color_bg 31
/set weechat.bar.activetitle.separator on
/set weechat.bar.title.conditions "${inactive}"
/set weechat.bar.title.color_fg black
/set weechat.bar.title.color_bg 31
/bar add rootstatus root bottom 1 0 [time],[buffer_count],[buffer_plugin],buffer_number+:+buffer_name+(buffer_modes)+{buffer_nicklist_count}+buffer_filter,[bitlbee_typing_notice],[lag],[aspell_dict],[aspell_suggest],completion,scroll
/set weechat.bar.rootstatus.color_fg 31
/set weechat.bar.rootstatus.color_bg 234
/set weechat.bar.rootstatus.separator on
/set weechat.bar.rootstatus.priority 500
/bar del status
/bar set rootstatus name status
/bar add rootinput root bottom 1 0 [buffer_name]+[input_prompt]+(away),[input_search],[input_paste],input_text
/set weechat.bar.rootinput.color_bg black
/set weechat.bar.rootinput.priority 1000
/bar del input
/bar set rootinput name input
/set weechat.bar.nicklist.color_fg 229
/set weechat.bar.nicklist.separator on
/set weechat.bar.nicklist.conditions "${nicklist} && ${window.number} == 1 && ${buffer.full_name} !~ ^irc.freenode.(#newsbin|##news)$"
/set weechat.bar.nicklist.size_max 14
/set weechat.bar.nicklist.size 14
/set weechat.look.bar_more_down "▼"
/set weechat.look.bar_more_left "◀"
/set weechat.look.bar_more_right "▶"
/set weechat.look.bar_more_up "▲"




