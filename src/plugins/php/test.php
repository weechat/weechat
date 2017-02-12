<?php

new class {

    function __construct() {
        weechat_register('phptest', 'adsr', '0.1', 'GPL3', 'test', '', '');
        weechat_hook_signal('*,irc_in2_privmsg', __CLASS__ . '::weechat_hook_signal_cb', 'anything');
        weechat_hook_completion('phpcompletion', 'desc', __CLASS__ . '::weechat_hook_completion_cb', 'anything');
        weechat_hook_command('phptestcmd', 'desc1', '', 'desc2', '', __CLASS__ . '::weechat_hook_command_cb', 'anything');
        weechat_hook_config('weechat.look.item_time_format', __CLASS__ . '::weechat_hook_config_cb', 'anything');
        weechat_hook_process('date', 10, __CLASS__ . '::weechat_hook_process_cb', 'anything');
    }

    function weechat_hook_signal_cb($anything, $signal, $type_data, $signal_data) {
        weechat_printf(null, 'anything=' . json_encode($anything) . ' signal='  . json_encode($signal) . ' signal_data=' . json_encode($signal_data));
        $info = weechat_info_get_hashtable('irc_message_parse', [ 'message' => $signal_data ]);
        weechat_printf(null, 'info=' . json_encode($info));
        list($srv, ) = explode(',', $signal, 2);
        $mynick = weechat_info_get('irc_nick', $srv);
        $chn = $info['channel'];
        $nick = $info['nick'];
        $buffer = weechat_info_get('irc_buffer', $srv . ',' . $chn);
        weechat_printf(null, "buffer=$buffer info=" . json_encode($info));
        weechat_command($buffer, "/say hello $nick on $srv:$chn I am $mynick");
        return WEECHAT_RC_OK;
    }

    function weechat_hook_completion_cb($anything, $item, $buffer, $completion) {
        weechat_printf(null, 'weechat_hook_completion: ' . json_encode(func_get_args()));
        return WEECHAT_RC_OK;
    }

    function weechat_hook_command_cb($anything, $modifier, $modifier_data, $str) {
        weechat_printf(null, 'weechat_hook_command: ' . json_encode(func_get_args()));
        return WEECHAT_RC_OK;
    }

    function weechat_hook_config_cb($anything, $option, $value) {
        weechat_printf(null, 'weechat_hook_config: ' . json_encode(func_get_args()));
        return WEECHAT_RC_OK;
    }

    function weechat_hook_process_cb($anything, $command, $ret_code, $out, $err) {
        weechat_printf(null, 'weechat_hook_process: ' . json_encode(func_get_args()));
        return WEECHAT_RC_OK;
    }
};
