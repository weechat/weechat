# -*- coding: iso-8859-1 -*-

# =============================================================================
#  autoauth.py (c) October 2005 by kolter <kolter+dev@openics.org>
#  Python script for WeeChat.
#
#  Licence     : GPL v2
#  Description : Permits to auto-authenticate when changing nick
#  Syntax      : try /auth help to get help on this script
#
#
# ### changelog ###
#
#  * version 0.5
#      - fix bug when script script is run for first time
#      - rewrite half script to improve access to settings
#      - add a feature to permit to run command(s) when identified
#      - add completion for commands
#  * version 0.4
#      - use set_plugin_config and get_plugin_config to read ans save settings
#      - remove deprecated import
#  * version 0.3
#      - add return codes
#  * version 0.2
#      - correct weechatdir with weechat_dir while using weechat.get_info
#  * version 0.1 :
#      - first release
#
# =============================================================================


VERSION="0.5"
NAME="autoauth"

import weechat

weechat.register (NAME, VERSION, "", "Auto authentification while changing nick")
weechat.add_message_handler("NOTICE", "auth_notice_check")
weechat.add_command_handler(
    "auth",
    "auth_command",
    "Auto authentification while changing nick",
    "{ add $nick $pass [$server=current] | del $nick [$server=current] | list | cmd [$command [$server=current]] }",
    "  add : add authorization for $nick with password $pass for $server\n"
    "  del : del authorization for $nick for $server\n"
    " list : list all authorization settings\n"
    "  cmd : command(s) (separated by '|') to run when identified for $server\n"
    "         %n will be replaced by current nick in each command",
    "add|del|list|cmd %- %S %S"
    )

def auth_cmdlist():
    cmd = ''
    cmds = weechat.get_plugin_config("commands")
    if cmds == '':
        weechat.prnt("[%s] commands (empty)" % (NAME))
    else:
        weechat.prnt("[%s] commands (list)" % (NAME))
        for c in cmds.split("####"):
            weechat.prnt("  --> %s : '%s' " % (c.split(":::")[0], c.split(":::")[1]))

def auth_cmdget(server):
    cmd = ''
    cmds = weechat.get_plugin_config("commands")
    if cmds != '':
        for c in cmds.split("####"):
            if c.find(":::") != -1:
                if c.split(":::")[0] == server:
                    cmd = ":::".join(c.split(":::")[1:])
                    break
    return cmd

def auth_cmdset(server, command):
    cmds = weechat.get_plugin_config("commands")

    found = False
    conf = []
    if cmds != '':
        for c in cmds.split("####"):
            if c.find(":::") != -1:
                if c.split(":::")[0] == server:
                    found = True
                    conf.append("%s:::%s" % (server, command))
                else:
                    conf.append(c)
    if not found:
        conf.append("%s:::%s" % (server, command))

    weechat.set_plugin_config("commands", "####".join(conf))
    weechat.prnt("[%s] command '%s' successfully added for server %s" % (NAME, command, server))

def auth_cmdunset(server):
    cmds = weechat.get_plugin_config("commands")

    found = False
    conf = []
    if cmds != '':
        for c in cmds.split("####"):
            if c.find(":::") != -1:
                if c.split(":::")[0] != server:
                    conf.append(c)
                else:
                    found = True
    if found:                
        weechat.prnt("[%s] command for server '%s' successfully removed" % (NAME, server))
        weechat.set_plugin_config("commands", "####".join(conf))
    
def auth_cmd(args, server):
    if server == '':
        if args == '':
            auth_cmdlist()
        else:
            weechat.prnt("[%s] error while setting command, can't find a server" % (NAME))
    else:
        if args == '':
            auth_cmdunset(server)
        else:
            auth_cmdset(server, args)

def auth_list():
    data = weechat.get_plugin_config("data")

    if data == "":
        weechat.prnt("[%s] accounts (empty)" % (NAME))
    else:
        weechat.prnt("[%s] accounts (list)" % (NAME))
        for e in data.split(","):
            if e.find("=") == -1:
                continue
            (serv_nick, passwd) = e.split("=")
            (server, nick) = serv_nick.split(".")
            weechat.prnt("  --> %s@%s " % (nick, server))

def auth_notice_check(server, args):
    if args.find("If this is your nickname, type /msg NickServ") != -1 or args.find("This nickname is registered and protected.") != -1 :
        passwd = auth_get(weechat.get_info("nick"), server)
        if passwd != None:
            weechat.command("/quote nickserv identify %s" % (passwd), "", server)
            commands = auth_cmdget(server)
            if commands != '':
                for c in commands.split("|"):
                    weechat.command(c.strip().replace("%n", weechat.get_info('nick')))
    
    return weechat.PLUGIN_RC_OK

def auth_del(the_nick, the_server):
    data = weechat.get_plugin_config("data")

    found = False
    conf = []
    for e in data.split(","):
        if e.find("=") == -1:
            continue
        (serv_nick, passwd) = e.split("=")
        (server, nick) = serv_nick.split(".")
        if the_nick == nick and the_server == server:
            found = True
        else:
            conf.append("%s.%s=%s" % (server, nick, passwd))

    if found:
        weechat.set_plugin_config("data", ",".join(conf))
        weechat.prnt("[%s] nick '%s@%s' successfully remove" % (NAME, the_nick, the_server))
    else:
        weechat.prnt("[%s] an error occured while removing nick '%s@%s' (not found)" % (NAME, the_nick, the_server))

def auth_add(the_nick, the_passwd, the_server):
    data = weechat.get_plugin_config("data")

    found = False
    conf = []    
    for e in data.split(","):
        if e.find("=") == -1:
            continue
        (serv_nick, passwd) = e.split("=")
        (server, nick) = serv_nick.split(".")
        if the_nick == nick and the_server == server:
            passwd = the_passwd
            found = True
        conf.append("%s.%s=%s" % (server, nick, passwd))

    if not found:
        conf.append("%s.%s=%s" % (the_server, the_nick, the_passwd))
        
    weechat.set_plugin_config("data", ",".join(conf))
    weechat.prnt("[%s] nick '%s@%s' successfully added" % (NAME, the_nick, the_server))

def auth_get(the_nick, the_server):
    data = weechat.get_plugin_config("data")

    for e in data.split(","):
        if e.find("=") == -1:
            continue
        (serv_nick, passwd) = e.split("=")
        (server, nick) = serv_nick.split(".")
        if the_nick == nick and the_server == server:
            return passwd
    return None

def auth_command(server, args):
    list_args = args.split(" ")
    
    #strip spaces
    while '' in list_args:
        list_args.remove('')
    while ' ' in list_args:
        list_args.remove(' ')

    if len(list_args) ==  0:
        weechat.command("/help auth") 
    elif list_args[0] not in ["add", "del", "list", "cmd"]:
        weechat.prnt("[%s] bad option while using /auth command, try '/help auth' for more info" % (NAME))
    elif list_args[0] == "cmd":
        if len(list_args[1:]) == 1 and list_args[1] in weechat.get_server_info().keys():
            auth_cmd("", list_args[1])
        elif len(list_args[1:]) == 1:
            auth_cmd(list_args[1], weechat.get_info('server'))
        elif len(list_args[1:]) >= 2:
            if list_args[-1] in weechat.get_server_info().keys():
                auth_cmd(" ".join(list_args[1:-1]), list_args[-1])
            else:
                auth_cmd(" ".join(list_args[1:]), weechat.get_info('server'))
        else:
            auth_cmd(" ".join(list_args[1:]), weechat.get_info(server))            
    elif list_args[0] == "list":
        auth_list()
    elif list_args[0] == "add":
        if len(list_args) < 3 or (len(list_args) == 3 and weechat.get_info("server") == ''):
            weechat.prnt("[%s] bad option while using /auth command, try '/help auth' for more info" % (NAME))
        else:
            if len(list_args) == 3:
                auth_add(list_args[1], list_args[2], weechat.get_info("server"))
            else:
                auth_add(list_args[1], list_args[2], list_args[3])
    elif list_args[0] == "del":
       if len(list_args) < 2:
           weechat.prnt("[%s] bad option while using /auth command, try '/help auth' for more info" % (NAME))
       else:
            if len(list_args) == 2:
                auth_del(list_args[1], weechat.get_info("server"))
            else:
                auth_del(list_args[1], list_args[2])
    else:
        pass
    return weechat.PLUGIN_RC_OK
