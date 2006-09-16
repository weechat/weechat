# -*- coding: iso-8859-15 -*-

"""
  :Author: Henning Hasemann <hhasemann [at] web [dot] de>

  Usage:
  
    - Load this plugin
    - In a channel or query type "/flip foo something" to
      send the reversed text "gnihtemos oof"
    - In a channel or query type "/leet something else" to
      send the h4x02-5r!pT - Version of your text.
      (Please use with caution such crap is discouraged in most channels)
  
  Released under GPL licence.
"""

__version__ = "0.1"
__history__ = """
  0.1 initial
"""

short_syntax = """TEXT"""
syntax_flip = """  Example:

  /flip foo bar
    sends "rab oof" to the channel
"""
syntax_leet = """  Example:
  
  /leet eleet
    sends "31337" (or similar) to the channel
"""

import weechat as wc
import string, random

wc.register("gimmicks", __version__, "", "string gimmicks")
wc.add_command_handler("flip", "flip", "", short_syntax, syntax_flip)
wc.add_command_handler("leet", "leet", "", short_syntax, syntax_leet)

leet_dict = {
  "e": ["3"],
  "l": ["1", "!", "|"],
  "r": ["|2"],
  "b": ["8"],
  "v": [r'\/'],
  "t": ["7"],
  "i": ["!"],
  "w": [r'\/\/', 'vv'],
  "a": ["/\\", "<|", "4"],
  "k": ["x"],
  "n": [r'|\|'],
  "s": ["5","$"],
  "q": ["O."],
  "z": ["zZz", "7_"],
  "u": ["(_)"],
  "p": ["|°", "|*"],
  "d": ["|)", "I>", "ol"],
  "f": ["i="],
  "g": ["@"],
  "h": ["|-|"],
  "j": ["_I"],
  "y": ["`/"],
  "x": ["><"],
  "c": ["[", "(", "{"],
  "m": ["|v|", "nn"],
  "o": ["0", "()"],
}

def leet(server, args):
  casechange = True 
  strange = True
  stay = False
  
  r = ""
  luflag = 0
  for x in list(args):
    if stay:
      alt = [x]
    else:
      alt = []
    if casechange:
      alt.append(luflag and x.lower() or x.upper())
      luflag = not luflag
    if strange:
      alt += leet_dict.get(x.lower(), [])
    r += random.choice(alt)
  wc.command(r)
  return 0

def flip(server, args):
  l = list(args)
  l.reverse()
  wc.command("".join(l))
  return 0



