<?php

new class {
    function __construct() {
        weechat_register('phptest', 'adsr', '0.1', 'GPL3', 'test', '', '');
        weechat_hook_signal("*,irc_in2_privmsg", [$this, 'onPrivMsg'], 'crap');
    }

    function onPrivMsg($crap, $signal, $type_data, $signal_data) {
        weechat_printf(null, 'crap=' . json_encode($crap) . ' signal='  . json_encode($signal) . ' signal_data=' . json_encode($signal_data));
        $info = weechat_info_get_hashtable("irc_message_parse", [ "message" => $signal_data ]);
        weechat_printf(null, "info=" . json_encode($info));
        list($srv, ) = explode(',', $signal, 2);
        $mynick = weechat_info_get("irc_nick", $srv);
        $chn = $info["channel"];
        $nick = $info["nick"];
        $buffer = weechat_info_get("irc_buffer", $srv . "," . $chn);
        weechat_printf(null, "buffer=$buffer info=" . json_encode($info));
        weechat_command($buffer, "/say hello $nick on $srv:$chn I am $mynick");
        return WEECHAT_RC_OK;
    }
};
