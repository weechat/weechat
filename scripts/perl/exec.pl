##############################################################################
#                                                                            #
#                                Exec                                        #
#                                                                            #
# Perl script for WeeChat.                                                   #
#                                                                            #
# Execute the command and print it to the actual buffer or server            #
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

use Config;

$Config{usethreads} or die( "Recompile Perl with threads to run this script." );

use Thread;

my $registred = 0;
weechat::register( "Exec", "0.2", "", "Execute the command and print it to some buffer" );

weechat::add_command_handler(
  "exec",
  execute,
  "Execute the command and print it to some buffer",
  "[-o][ -win server:::channel] <cmd line>",
  "    -o                       print as msg\n".
  "    -win server:::channel    print the output to the specific buffer",
  ""
);

sub execute {
  if( !$registred ) { return; }
  my $i = 0;
  my $cmd = "";
  my $msg = 0;
  my $win = "";
  my @arr = split( ' ', $_[1] );
  my $cmdStart = 0;
  if( $#arr < 0 ) { return; }
  for( $i = 0; $i <= $#arr; $i++ ) {
    if( @arr[$i] eq "-win" ) {
      $i++;
      $win = @arr[$i];
      $cmdStart = $i + 1;
    }
    if( @arr[$i] eq "-o" ) {
      $msg = 1;
      $cmdStart = $i + 1;
    }
  }
  for( $i = $cmdStart; $i <= $#arr; $i++ ) {
    if( length( $cmd ) ) { $cmd = $cmd . ' '; }
    $cmd = $cmd . @arr[$i];
  }
  
  my $thr = new Thread \&func_execute, $cmd, $msg, $win;
}

sub func_execute {
  my $command = shift;
  my $msg = shift;
  my $win = shift;
  my $c = 1;
  my $char = '';
  my $date = `date +%Y%m%d%H%M%S%N`;
  my $out = "";

  if( !length( $command ) ) {
    weechat::print( "No command to execute (try -? for help)" );
    return;
  }

  if( substr( $date, length( $date ) - 1, 1 ) eq "\n" ) { $date = substr( $date, 0, length( $date ) - 1 ); }
  
  if( length( $command ) ) { system( $command . " > /tmp/weechat_" . $date . " 2>&1" ); }

  open( FD, '<', "/tmp/weechat_" . $date ) or weechat::print( "/tmp/weechat_" . $date . ": $!" );
  do {
    $c = read( FD, $char, 1 );
    $out = $out . $char;
  } while( $c );
  close( FD );
  system( "rm -f /tmp/weechat_" . $date );
  my $j = index( $win, ':::' );
  if( length( $win ) && ( $j >= 0 ) ) {
    if( $msg ) {
      my $win1 = substr( $win, $j + 3 );
      my $win2 = substr( $win, 0, $j );
      my @its = split( "\n", $out );
      for( $j = 0; $j <= $#its; $j++ ) {
        weechat::command( $its[$j], $win1, $win2 );
      }
    } else {
      weechat::print( $out, substr( $win, $j + 3 ), substr( $win, 0, $j ) );
    }
  } else {
    if( $msg ) {
      my $win1 = substr( $win, $j + 3 );
      my $win2 = substr( $win, 0, $j );
      my @its = split( "\n", $out );
      for( $j = 0; $j <= $#its; $j++ ) {
        weechat::command( $its[$j], $win1, $win2 );
      }
    } else {
      weechat::print( $out );
    }
  }
}

$registred = 1;

