##############################################################################
#                                                                            #
#                        Away highlite loger                                 #
#                                                                            #
# Perl script for WeeChat.                                                   #
#                                                                            #
# Log highlite/private msg when you are away                                 #
#                                                                            #
#                                                                            #
#                                                                            #
# Copyright (C) 2006  Jiri Golembiovsky <golemj@gmail.com>                   #
#                                                                            #
# This program is free software; you can redistribute it and/or              #
# modify it under the terms of the GNU General Public License                #
# as published by the Free Software Foundation; either version 2             #
# of the License, or (at your option) any later version.                     #
#                                                                            #
# This program is distributed in the hope that it will be useful,            #
# but WITHOUT ANY WARRANTY; without even the implied warranty of             #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              #
# GNU General Public License for more details.                               #
#                                                                            #
# You should have received a copy of the GNU General Public License          #
# along with this program; if not, write to the Free Software                #
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,                 #
# MA  02110-1301, USA.                                                       #
#                                                                            #
##############################################################################

weechat::register( "AwayLog", "0.4", "", "Log privmsg/highlite when you are away" );

weechat::add_message_handler( "PRIVMSG", "awaylog" );

sub test_highlight {
  $str = shift;
  $irc_highlight = weechat::get_config( "irc_highlight" );
  @arr = split( ",", $irc_highlight );
  $b = 0;
  $str = lc( $str );
  while( $item = pop( @arr ) ) {
    $item = lc( $item );
    if( substr( $item, 0, 1 ) eq '*' ) { $item = '.' . $item; }
    if( substr( $item, length( $item ) - 1, 1 ) eq '*' ) { $item = substr( $item, , 0, length( $item ) - 1 ) . ".*"; }
    if( $str =~ /$item/ ) { $b++; }
  }
  return $b;
}

sub awaylog {
  if( weechat::get_info( "away", $_[0] ) == 1 ) {
    $i = index( $_[1], " PRIVMSG " );
    $hostmask = substr( $_[1], 0, $i );
    $str = substr( $_[1], $i + 9 );
    $i = index( $str, ":" );
    $channel = substr( $str, 0, $i - 1 );
    $message = substr( $str, $i + 1 );
    if( substr( $hostmask, 0, 1 ) eq  ":" ) {
      $hostmask = substr( $hostmask, 1 );
    }
    ($nick, $host) = split( "!", $hostmask );
    $mynick = weechat::get_info( "nick", $_[0] );
    if( ( index( $message, $mynick ) != -1 ) || ( $channel eq $mynick ) || ( test_highlight( $message ) > 0 ) ) {
      weechat::print( "$channel -- $nick :: $message", "", $_[0] );
    }
  }
  return 0;
}

