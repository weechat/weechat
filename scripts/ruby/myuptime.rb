# ==================================================================================================
#  myuptime.rb (c) April 2006 by David DEMONCHY (fusco) <fusco.spv@gmail.com>
#
#  Licence     : GPL v2
#  Description : Print machine uptime in the current canal
#  Syntax      : /myuptime
#    => <nick> uptime  <hostname> :  00:16:58 up 11:09,  4 users,  load average: 0.05, 0.21, 0.29 
#
# ==================================================================================================

def weechat_init
    Weechat.register("myuptime", "0.1", "", "Script ruby WeeChat for display my uptime")
    Weechat.add_command_handler("myuptime", "myuptime")
    return Weechat::PLUGIN_RC_OK
end

def myuptime(server, args)
Weechat.command("/me uptime "+ `hostname`.chomp + ":"  + `uptime`.chomp)
return Weechat::PLUGIN_RC_OK
end
