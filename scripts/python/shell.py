
# =============================================================================
#  shell.py (c) March 2006 by Kolter <kolter+dev@openics.org>
#
#  Licence     : GPL v2
#  Description : running shell commands in WeeChat
#  Syntax      : try /help shell to get some help on this script
#  Precond     : needs weechat > 0.1.7 to run else it will crash WeeChat ;-)
#
#
# ### changelog ###
#
#  * version 0.1 :
#      - first release
#
# =============================================================================

import weechat, os, popen2

SHELL_CMD="shell"
SHELL_PREFIX="[shell] "

weechat.register ("Shell", "0.1", "", "Running shell commands in WeeChat")
weechat.add_command_handler(
    SHELL_CMD,
    "shell",
    "Running shell commands in WeeChat",
    "[-o] <command line>",
    "            -o :  print output on current server/channel\n"
    "<command line> :  shell command or builtin like cd, getenv, setenv, unsetenv",
    "-o|cd|getenv|setenv|unsetenv cd|getenv|setenv|unsetenv"
   )

def shell_exec(command):
    proc = popen2.Popen3(command, True)
    status = proc.wait()
    results = []
    if status == 0:
        results = proc.fromchild.readlines()
    else:
        results = proc.childerr.readlines()
    return status, results
            
def shell_output(command, inchan):
    status, results = shell_exec(command)
    if status == 0:
        for line in results:
            if inchan:
                weechat.command(line.rstrip('\n'))
            else:
                weechat.prnt(line.rstrip('\n'))
    else:
        weechat.prnt("%san error occured while running command `%s'" % (SHELL_PREFIX, command))
        for line in results:
            weechat.prnt(line.rstrip('\n'))


def shell_chdir(directory):
    if directory == "":
        if os.environ.has_key('HOME'):
            directory = os.environ['HOME']
    try:
        os.chdir(directory)
    except:
        weechat.prnt("%san error occured while running command `cd %s'" % (SHELL_PREFIX, directory))
    else:
        pass

def shell_getenv(var, inchan):
    var = var.strip()
    if var == "":
        weechat.prnt("%swrong syntax, try 'getenv VAR'" % (SHELL_PREFIX))
        return
        
    value = os.getenv(var)
    if value == None:
        weechat.prnt("%s$%s is not set" % (SHELL_PREFIX, var))
    else:
        if inchan:
            weechat.command("$%s=%s" % (var, os.getenv(var)))
        else:
            weechat.prnt("%s$%s=%s" % (SHELL_PREFIX, var, os.getenv(var)))
        
def shell_setenv(expr, inchan):
    expr = expr.strip()
    lexpr = expr.split('=')
    
    if (len(lexpr) < 2):
        weechat.prnt("%swrong syntax, try 'setenv VAR=VALUE'" % (SHELL_PREFIX))
        return

    os.environ[lexpr[0].strip()] = "=".join(lexpr[1:])
    if not inchan:
        weechat.prnt("%s$%s is now set to '%s'" % (SHELL_PREFIX, lexpr[0], "=".join(lexpr[1:])))

def shell_unsetenv(var, inchan):
    var = var.strip()
    if var == "":
        weechat.prnt("%swrong syntax, try 'unsetenv VAR'" % (SHELL_PREFIX))
        return

    if os.environ.has_key(var):
        del os.environ[var]
        weechat.prnt("%s$%s is now unset" % (SHELL_PREFIX, var))
    else:
        weechat.prnt("%s$%s is not set" % (SHELL_PREFIX, var))        
    
def shell(server, args):    
    largs = args.split(" ")    

    #strip spaces
    while '' in largs:
        largs.remove('')
    while ' ' in largs:
        largs.remove(' ')

    if len(largs) ==  0:
        weechat.command("/help %s" % SHELL_CMD)
    else:
        inchan = False
        if largs[0] == '-o':
            inchan = True
            largs = largs[1:]

        if largs[0] == 'cd':
            shell_chdir(" ".join(largs[1:]), inchan)
        elif largs[0] == 'getenv':
            shell_getenv (" ".join(largs[1:]), inchan)
        elif largs[0] == 'setenv':
            shell_setenv (" ".join(largs[1:]), inchan)
        elif largs[0] == 'unsetenv':
            shell_unsetenv (" ".join(largs[1:]), inchan)
        else:
            shell_output(" ".join(largs), inchan)

    return weechat.PLUGIN_RC_OK
