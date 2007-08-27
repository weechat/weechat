# growl-notify.pl -- Have Weechat send notifications thru Mac::Growl
# Copyright (c) 2007 Zak B. Elep <zakame@spunge.org>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

use strict;

use Mac::Growl;

my $app = 'Weechat (Curses)';
my @messages_events = (
	[ nick => 'Changed Nick', '$nick has changed nick to $text' ],
	[ join => 'Joined Room', '$nick has joined $text' ],
	[ part => 'Left Room', '$nick has left $channel($text)' ],
	[ quit => 'Quit', '$nick has quit($text)' ],
	[ topic => 'Set Topic', '$nick has set topic to \'$text\'' ],
	[ weechat_highlight => 'Highlight Mentioned', '$text in $channel' ],
	[ weechat_pv => 'Private Message', '$nick: $text' ],
);
my $notes;
push @$notes, $_->[1] foreach @messages_events;

Mac::Growl::RegisterNotifications $app, $notes, $notes;

my $version = '0.2';
weechat::register 'growl-notify', $version, '',
	'Send Weechat notifications thru Mac::Growl';

for my $message (@messages_events) {
	no strict 'refs';	# access symbol table
	my $subname = join '', __PACKAGE__, '::handler_', $message->[0];
	*{$subname} = sub
	{
		my($server, $args) = @_;
		# weechat::print $args;
		my($mask, $nick, $channel, $text);
		(undef, $channel, $text) = split /:/, $args, 3;
		($mask, undef, $channel) = split / /, $channel;
		($nick, undef) = split /!/, $mask;
		Mac::Growl::PostNotification $app, $message->[1],
			$message->[1], eval qq("$message->[2]"),
			($message->[0] =~ /(pv|highlight)/ ? 1 : 0);
		return weechat::PLUGIN_RC_OK;
	};
	weechat::add_message_handler $message->[0], $subname;
}

__END__

=head1 NAME

growl-notify.pl - Send Weechat notifications thru the Growl framework for Mac

=head1 SYNOPSIS

	# in Weechat console, do
	/perl load /path/to/growl-notify.pl

	# or put script in .weechat/perl/autoload/

=head1 DESCRIPTION

growl-notify is a script plugin for Weechat that sends notifications of common
IRC events (such as joins, highlights, and private messages) to the Growl
notification framework for the Macintosh.  It uses the perl module L<Mac::Growl>
to do its magic, even allowing the ncurses version of Weechat to generate
graphical or audible event notifications.

=head1 SEE ALSO

L<http://growl.info>, L<Mac::Growl>, L<http://weechat.flashtux.org>

=head1 AUTHORS

Zak B. Elep, C<< zakame at spunge.org >>

=cut

