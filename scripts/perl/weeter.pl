# weeter.pl -- Send CTCP ACTION messages as Twitter status updates
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

use Net::Twitter;

my $version = '0.1';
weechat::register 'weeter', $version, '',
	'Send CTCP ACTION messages as Twitter status updates';

my %credentials;
for(qw(username password)){
	$credentials{$_} = weechat::get_plugin_config $_;
}
my $twitter = new Net::Twitter %credentials;

weechat::add_modifier 'irc_user', 'privmsg', 'twitter_update';
sub twitter_update
{
	my $message = $_[1];
	(my $status = $message) =~ s#^/me\s+##;
	update $twitter $status unless $status eq $message;
	return $message;
}

__END__

=head1 NAME

weeter.pl - Send Weechat CTCP ACTION messages to Twitter as status updates

=head1 SYNOPSIS

	# in Weechat console, do
	/setp perl.weeter.username = 'your-twitter-username'
	/setp perl.weeter.password = 'your-twitter-password'
	/perl load /path/to/weeter.pl

	# or do the /setp and put script in .weechat/perl/autoload

=head1 DESCRIPTION

weeter is a script plugin for Weechat that enables CTCP ACTION messages (aka
C</me> actions) to be sent to the Twitter web service as user status updates.
It uses the L<Net::Twitter> plugin from CPAN to do its work.

weeter requires the variables C<perl.weeter.username> and 
C<perl.weeter.password> defined prior to use.

=head1 SEE ALSO

L<http://twitter.com>, L<Net::Twitter>, L<http://weechat.flashtux.org>

=head1 AUTHORS

Zak B. Elep, C<< zakame at spunge.org >>

=cut

