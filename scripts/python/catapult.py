#!/usr/bin/env python
"""
Catapult by Stalwart <stlwrt@gmail.com>
Licensed under GNU GPL 2
"""

import weechat
import random

weechat.register("Catapult", "0.1", "", "Less ordinary abuse generator")
weechat.add_command_handler("slap", "slapper", "Creative slapper", "<target>", "", "%n")
weechat.add_command_handler("fortune", "fortune", "Fortune cookies!")
weechat.add_command_handler("winjoke", "winjoke", "Windoze jokes")
weechat.add_command_handler("linjoke", "linjoke", "Linux jokes")
weechat.add_command_handler("give", "giver", "Creative giver", "<target>", "", "%n")
weechat.add_command_handler("hate", "hater", "Creative hater", "<target>", "", "%n")
weechat.add_command_handler("love", "lover", "Creative lover", "<target>", "", "%n")
weechat.add_command_handler("dance", "dancer", "Creative dancer", "<target>", "", "%n")

# slapper 
def slapper(server, args):
	objects = [
		"a rather large squid",
		"a hydraulic pump",
		"a book by Stephen King",
		"a 10mbit network card",
		"a ladies handbag",
		"some girl scouts",
		"a football team",
		"a bottle",
		"a yellow marshmellow",
		"a match",
		"the queen of England",
		"a taxi",
		"100 feet of wire",
		"a bag of Cheerios",
		"a hat",
		"a fist",
		"the back hand",
		"with the forehead",
		"a computer moniter",
		"a coconut",
		"a microfone",
		"a cellphone",
		"a snowplough",
		"a doggy",
		"Bill Clinton",
		"a stone",
		"a club. With a nail in it",
		"a small asteroid, rich in iron",
		"a small interstellar spaceship",
		"a fresh zuccini",
		"a laptop",
		"a big dictionary",
		"a baseball bat",
		"NeverNet",
		"some porn",
		"a mIRC script",
		"a canoe",
		"a tortoise",
		"a horse",
		"the book of Kells",
		"a whale",
		"a rubber dildo",
		"a well groomed poodle",
		"a channel operator",
		"a news paper (New York Times Sunday Edition)",
		"a gnarly werewolf",
		"a vampire. They really suck",
		"a perl script",
		"a bag of doggie chow",
		"a fat walrus",
		"an IP adress",
		"a catholic priest",
		"James Dean",
		"Ronald MacDonald (he *IS* good for something)",
		"Autoconf version 2.13",
		"a PRIVMSG",
		"an email adress",
		"some ANSI color codes",
		"a thermonuclear weapon. Yehaw",
		"the hitch hikers guide to the galaxy, revised edition",
		"Nessie, the Loch Ness monster",
		"a tuna. Still in the can! *BONK* That will leave a mark",
		"a few fluffy pillows",
		"a red chinese dragon",
		"a linux-manual (signed by L. Torvalds)",
		"Stage1",
		"Bill Gates underpants",
		"GM Abraham and the whole OW-Staff",
		"Sphere 1.0",
		"a Linuxkernel",
		"Lenin's Collected Works",
		"Stalin's Collected Works",
		"a iron Tux",
		"a glowing 23",
		"your mum (CLEAN YOUR ROOM!)",
		"a complete GNOME-Documentation",
		"a Portagetree",
		"thand[Z]'s transparent Tanga",
		"a Kernelpanic",
		"Windoze XP",
		"an AK-47 (die, you imperialist dog!)",
		"a bag full with Michael Jacksons droped noses",
		"an NBP-Manifesto (Hail Limonew! Hail our Leader! Hail!)",
		"an NBP-Flag (Lead us to freedom, Leader Limonew!)",
		"Mein Kampf (Doppelausgabe, Hardcover)",
		"Invader Zim's iron fist",
		"some ASCII-Arts",
		"The Family Learning Channel",
		"GOD",
		"Dorian Grey's picture",
		"some unlocked Grenades *BOOM*",
		"the Win2k Buglist",
		"a widescreen TV (it can damage your brain!)",
		"a chain saw",
		"a huge tree"
	]
	if args <> "":
		weechat.command("/me slaps %s with %s." % (args, random.choice(objects)))
		return weechat.PLUGIN_RC_OK
	else:
		weechat.prnt("You must specify target")
		return weechat.PLUGIN_RC_KO

# fortune cookies
def fortune(server, args):
	objects = [
		"There are three kinds of people: men, women, and unix.",
		"BOFH Excuse #118: the router thinks its a printer.",
		"CHUBBY CHECKER just had a CHICKEN SANDWICH in downtown DULUTH!",
		"Think big. Pollute the Mississippi.",
		"An optimist is a man who looks forward to marriage. A pessimist is a married optimist.",
		"There are never any bugs you haven't found yet.",
		"It don't mean a THING if you ain't got that SWING!!",
		"Atomic batteries to power, turbines to speed. -- Robin, The Boy Wonder",
		"Two wrongs don't make a right, but three lefts do.",
		"A log may float in a river, but that does not make it a crocodile.",
		"Art is Nature speeded up and God slowed down. -- Chazal",
		"Texas law forbids anyone to have a pair of pliers in his possession.",
		"UFOs are for real: the Air Force doesn't exist.",
		"Expansion means complexity; and complexity decay.",
		"I came, I saw, I deleted all your files.",
		"The Magic of Windows: Turns a 486 back into a PC/XT.",
		"Our vision is to speed up time, eventually eliminating it. -- Alex Schure",
		"Breaking Windows isn't just for kids anymore...",
		"Neckties strangle clear thinking. -- Lin Yutang",
		"I'm prepared for all emergencies but totally unprepared for everyday life.",
		"Yow!  I just went below the poverty line!",
		"Break into jail and claim police brutality.",
		"BOFH Excuse #290: The CPU has shifted, and become decentralized.",
		"Did you hear about the model who sat on a broken bottle and cut a nice figure?",
		"We have a equal opportunity Calculus class -- it's fully integrated.",
		"BOFH Excuse #170: popper unable to process jumbo kernel",
		"It's later than you think.",
		"My Aunt MAUREEN was a military advisor to IKE & TINA TURNER!!",
		"Everyone hates me because I'm paranoid.",
		"When you are in it up to your ears, keep your mouth shut.",
		"Theory is gray, but the golden tree of life is green. -- Goethe",
		"Most Texans think Hanukkah is some sort of duck call. -- Richard Lewis",
		"grasshopotomaus: A creature that can leap to tremendous heights... once.",
		"It takes both a weapon, and two people, to commit a murder.",
		"Facts are the enemy of truth. -- Don Quixote",
		"Forty two.",
		"BOFH Excuse #424: operation failed because: there is no message for this error (#1014)",
		"Never leave anything to chance; make sure all your crimes are premeditated.",
		"BOFH Excuse #60: system has been recalled",
		"Science and religion are in full accord but science and faith are in complete discord.",
		"A likely impossibility is always preferable to an unconvincing possibility. -- Aristotle",
		"The angry man always thinks he can do more than he can. -- Albertano of Brescia",
		"Hello again, Peabody here... -- Mister Peabody",
		"Nobody ever died from oven crude poisoning.",
		"Dogs crawl under fences... software crawls under Windows 95.",
		"Say something you'll be sorry for, I love receiving apologies.",
		"NOTICE: -- THE ELEVATORS WILL BE OUT OF ORDER TODAY -- (The nearest working elevator is in the building across the street.)",
		"THERE ARE PLENTY OF BUSINESSES LIKE SHOW BUSINESS -- Bart Simpson on chalkboard in episode 1F19",
		"Life is what happens to you while you're busy making other plans. -- John Lennon, Beautiful Boy",
		"While having never invented a sin, I'm trying to perfect several.",
		"Emacs, n.: A slow-moving parody of a text editor.",
		"	... with liberty and justice for all ... who can afford it.",
		"Princess Leia: Aren't you a little short for a stormtrooper?",
		"The Official Colorado State Vegetable is now the 'state legislator'",
		"Abandon the search for Truth; settle for a good fantasy.",
		"There's nothing to writing.  All you do is sit at a typewriter and open a vein. -- Red Smith",
		"Our OS who art in CPU, UNIX be thy name. Thy programs run, thy syscalls done, In kernel as it is in user!",
		"All hope abandon, ye who enter here! -- Dante Alighieri",
		"The human mind ordinarily operates at only ten percent of its capacity -- the rest is overhead for the operating system.",
		"Professor: 'If a dog craps anywhere in the universe, you can bet I won't be out of loop.'",
		"I WILL NOT MAKE FLATUENT NOISES IN CLASS -- Bart Simpson on chalkboard in episode 7F13",
		"Listen you donkey raping shit eater.",
		"In specifications, Murphy's Law supersedes Ohm's.",
		"One of Bender's kids: Our dad is a giant toy!",
		"I am a jelly donut.  I am a jelly donut.",
		"We are all in the gutter, but some of us are looking at the stars. -- Oscar Wilde",
		"Bender to Zoidberg: 'You're looking less nuts, crabby.'",
		"Things will be bright in P.M.  A cop will shine a light in your face.",
		"A journey of a thousand miles begins with a cash advance.",
		"Everything that can be invented has been invented. -- Charles Duell, Director of U.S. Patent Office, 1899",
		"Vote anarchist.",
		"paranoia, n.: A healthy understanding of the way the universe works.",
		"BOFH Excuse #401: Sales staff sold a product we don't offer.",
		"BOFH Excuse #200: The monitor needs another box of pixels.",
		"The important thing is not to stop questioning.",
		"Not all men who drink are poets.  Some of us drink because we aren't poets.",
		"Oh my god, dude!",
		"BOFH Excuse #441: Hash table has woodworm",
		"BOFH Excuse #112: The monitor is plugged into the serial port",
		"There's so much to say but your eyes keep interrupting me.",
		"For the next hour, WE will control all that you see and hear.",
		"Reality continues to ruin my life. -- Calvin",
		"The shortest distance between two points is under construction. -- Noelie Alito",
		"BOFH Excuse #192: runaway cat on system.",
		"My haircut is totally traditional!",
		"You!  What PLANET is this! -- McCoy, 'The City on the Edge of Forever', stardate 3134.0",
		"BOFH Excuse #433: error: one bad user found in front of screen",
		"Dyslexics have more fnu.",
		"I'm not stupid, I'm not expendable, and I'M NOT GOING!",
		"It's clever, but is it art?",
		"His life was formal; his actions seemed ruled with a ruler.",
		"Yeah. Except for being entirely different, they're pretty much the same.",
		"Oh, I get it!!  'The BEACH goes on', huh, SONNY??",
		"Ban the bomb.  Save the world for conventional warfare.",
		"Everything that you know is wrong, but you can be straightened out.",
		"If I pull this SWITCH I'll be RITA HAYWORTH!!  Or a SCIENTOLOGIST!",
		"Tex SEX!  The HOME of WHEELS!  The dripping of COFFEE!!  Take me to Minnesota but don't EMBARRASS me!!",
		"clovek, ktory si ako prvy kupil fax musel byt strasny kokot.",
		"Are you a turtle?",
		"You will be the last person to buy a Chrysler.",
		"If in doubt, mumble.",
		"Nice guys don't finish nice.",
		"You cannot use your friends and have them too.",
		"Microsoft is to Software as McDonalds is to Cuisine.",
		"panic: can't find /",
		"I used to be an agnostic, but now I'm not so sure.",
		"May your SO always know when you need a hug.",
		"We are MicroSoft.  You will be assimilated.  Resistance is futile. (Attributed to B.G., Gill Bates)",
		"Life is a whim of several billion cells to be you for a while.",
		"BOFH Excuse #347: The rubber band broke",
		"Death has been proven to be 99% fatal in laboratory rats.",
		"There are only two kinds of tequila.  Good and better.",
		"C for yourself.",
		"Tact, n.: The unsaid part of what you're thinking.",
		"Shit Happens."
	]
	weechat.command(("Wanda the Fish says: %s") % random.choice(objects))
	return weechat.PLUGIN_RC_OK

# winjoke
def winjoke(server, args):
	objects = [
		"Windows NT, from the people who invented EDLIN!",
		"Windows: Microsoft's tax on computer illiterates.",
		"The nice thing about Windows is - It does not just crash, it displays a dialog box and lets you press 'OK' first.",
		"Why use Windows, since there is a door?",
		"In a world without fences who needs Gates?",
		"Another name for a Windows tutorial is crash course!",
		"Failure is not an option -- it comes bundled with Windows.",
		"NT... the last two letters of bowel movement",
		"Some software money can't buy. For everything else there's Micros~1.",
		"Sticks and Stones may break my bones but FUD will never concern me.",
		"Every program expands until it can send mail. ...Except Exchange. ",
		"Microsoft: 'You've got questions. We've got dancing paperclips.'",
		".vbs = Virus Bearing Script?",
		"Technology is positive when the creators put the interests of their users before their bottom line.",
		"Have you ever noticed that at trade shows Microsoft is always the one giving away stress balls?",
		"Do you remember when you only had to pay for windows when *you* broke them? (Submitted by Noel Maddy)",
		"National Weather Service advice for those threatened by severe thunderstorms: 'Go inside a sturdy building and stay away from WINDOWS!' (Submitted by Ben Bullock)",
		"Microsoft is to Software as McDonalds is to Cuisine.",
		"Microsoft should switch to the vacuum cleaner business where people actually want products that suck. (Submitted by Bruno Bratti)",
		"Everyone seems so impatient and angry these days. I think it's because so many people use Windows at work -- do you think you'd be Politeness Man after working on Windows 8 hrs. or more? (Submitted by Chip Atkinson)",
		"NT 5.0 so vaporous it's in danger of being added to the periodic table as a noble gas. (Spotted in a Slashdot discussion)",
		"My Beowulf cluster will beat your Windows NT network any day. (Submitted by wbogardt[at]gte.net)",
		"It's no wonder they call it WinNT; WNT = VMS++; (Submitted by Chris Abbey)",
		"Double your disk space - delete Windows! (Submitted by Albert Dorofeev)",
		"The Edsel. New Coke. Windows 2000. All mandatory case studies for bizschool students in 2020. (From a LinuxToday post by Bear Giles)",
		"I will never trust someone called GATES that sells WINDOWS. (Submitted by Federico Roman)",
		"'Microsoft technology' -- isn't that an oxymoron?",
		"MCSE == Mentally Challenged Slave of the Empire.",
		"Windows NT -- it'll drive you buggy!",
		"Where do you want to go today? Don't ask Microsoft for directions.",
		"MS and Y2K: Windows 95, 98, ... and back again to 01",
		"There's the light at the end of the the Windows.",
		"People use dummies for crash-tests. Windows is so difficult they had to educate the dummies first -- by giving them 'Windows for Dummies' books!",
		"Windows: The first user interface where you click Start to turn it off.",
		"NT == No Thanks",
		"With Windows Millennium, Microsoft was able to get the boot time down to 25 seconds. That's almost as short as it's uptime.",
		"Windows 2000: Designed for the Internet. The Internet: Designed for UNIX.",
		"MCSE = Minesweeper Consultant, Solitaire Expert",
		"MCSE = Meaningless Certificate, Software Expired",
		"I'm not a programmer, but I play one at Microsoft.",
		"Microsoft Zen - Become one with the blue screen.",
		"The next hot technology from Microsoft will be object-oriented assembly."
	] 
	weechat.command(("Wanda the Fish says: %s") % random.choice(objects))
	return weechat.PLUGIN_RC_OK

# linjoke
def linjoke(server, args):
	objects = [
		"Got Linux?",
		"Microsoft gives you Windows... Linux gives you the whole house.",
		"Linux, DOS, Windows NT -- The Good, the Bad, and the Ugly",
		"Linux: the operating system with a CLUE... Command Line User Environment",
		"If Bill Gates is the Devil then Linus Torvalds must be the Messiah.",
		"Linux. Where do you want to go tomorrow?",
		"Linux: The choice of a GNU generation",
		"When you say I wrote a program that crashed Windows, people just stare at you blankly and say Hey, I got those with the system, *for free*. -- Linus Torvalds",
		"We all know Linux is great...it does infinite loops in 5 seconds. -- Linus Torvalds",
		"Some people have told me they dont think a fat penguin really embodies the grace of Linux, which just tells me they have never seen a angry penguin charging at them in excess of 100mph. Theyd be a lot more careful about what they say if they had. -- Linus Torvalds",
		"Veni, vidi, Linux!",
		"Type cat vmlinuz > /dev/audio to hear the Voice of God.",
		"Linux: Because a PC is a terrible thing to waste.",
		"Linux: Because rebooting is for adding new hardware",
		"We are Linux. Resistance is measured in Ohms.",
		"Free Software: the Software by the People, of the People and for the People. Develop! Share! Enhance! and Enjoy! (Submitted by Andy Tai)",
		"Get it up, keep it up... LINUX: Viagra for the PC. (Submitted by Chris Abbey)",
		"Peace, Love and Compile the kernel.... (Submitted by Justin L. Herreman)",
		"Free your software, and your ass will follow",
		"Reset button? Which reset button? - Linux, the OS that never sleeps.",
		"Linux: Where do you want to GO... Oh, Im already there!",
		"Windows contains FAT. Use Linux -- you wont ever have to worry about your weight.",
		"Oh My God! They Killed init! You Bastards!",
		"Unix: Where /sbin/init is still Job 1."
	]
	weechat.command(("Wanda the Fish says: %s") % random.choice(objects))
	return weechat.PLUGIN_RC_OK

# giver
def giver(server, args):
	objects = [
		"a binary four",
		"a nice cup of SHUT THE FUCK UP",
		"an asskick",
		"a sign: 'Please kill yourself'",
		"a sign: 'Please go home now'",
		"an unlocked Grenade *tick tick BOOM*",
		"a gun - do it for mankind!"
 
	]
	if args <> "":
		weechat.command("/me gives %s %s." % (args, random.choice(objects)))
		return weechat.PLUGIN_RC_OK
	else:
		weechat.prnt("You must specify target")
		return weechat.PLUGIN_RC_KO

# hater 
def hater(server, args):
	objects = [
		"so much, that he spits hellfire",
		"so much, that he want's to shoot someone",
		"SOOOO MUUUUCH!!!"

	]
	if args <> "":
		weechat.command("/me hates %s %s." % (args, random.choice(objects)))
		return weechat.PLUGIN_RC_OK
	else:
		weechat.prnt("You must specify target")
		return weechat.PLUGIN_RC_KO

# lover 
def lover(server, args):
	actions = [
		"a *mwah*",
		"a wet kiss",
		"a tight and long hug",
		"an ass-squeeze",
		"a tight hug",
		"a wet kiss",
		"a nice, tight hug",
		"a kiss on the cheek",
		"a wet, french kiss",
		"a kiss",
		"a hug",
		"sex",
		"sweet romance",
		"some groping",
		"bed actions",
		"oral sex",
		"a porn-tape",
		"anal love",
	]
	if args <> "":
		weechat.command("/me cheers %s with %s.." % (args, random.choice(actions)))
		return weechat.PLUGIN_RC_OK
	else:
		weechat.prnt("You must specify target")
		return weechat.PLUGIN_RC_KO

# dancer 
def dancer(server, args):
	objects = [
		"used condoms",
		"condoms",
		"used tampons",
		"tampons",
		"roses",
		"rosebuds",
		"rice",
		"uncooked rice",
		"bananas",
		"little pieces of paper with the text *YAY!* on",
		"little pieces of paper with the text *w00t!* on",
		"little pieces of paper with the text *WOOHOO!* on",
		"little pieces of paper with the text *Kiss my cheek!* on",
		"coconuts with faces on",
		"little pieces of paper with the text *LOVE ROCKS!* on",
		"little pieces of paper with the text *LETS HAVE SEX!* on",
		"little pieces of paper with the text *GOD BLESS AMERICA!* on",
		"balloons with faces on",
		"balloons",
		"chocolate cakes",
		"english teachers",
		"pr0n magazines",
		"vietnamese kids",
		"snowballs",
		"rocks",
		"Michael Jackson (naked)",
		"unlocked Grenades *BOOM*",
		"Nine Inch Nails",
		"Bodyparts (bloody, wet & sexy)",
	]
	if args <> "":
		weechat.command("/me dances around %s throwing %s.." % (args, random.choice(objects)))
		return weechat.PLUGIN_RC_OK
	else:
		weechat.prnt("You must specify target")
		return weechat.PLUGIN_RC_KO
