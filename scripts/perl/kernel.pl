# 
# This script is mostly a copy/paste from kernel.pl irssi script
# This script is in the public domain.
# 
# Julien Louis <ptitlouis@sysif.net>

use IO::Socket;

weechat::register("kernel", "0.1", "", "Return latest kernel versions from kernel.org" );

weechat::add_command_handler("kernel_version", kernel_version);

sub finger($$) {
    # Yes, Net::Finger is already done and i'm reinventing the wheel.
   my ($user, $host) = @_;
   my $buffer;
   if (my $socket = IO::Socket::INET->new(
   	PeerHost => $host,
        PeerPort => 'finger(79)',
        Proto    => 'tcp',
      ))
    {
      if (syswrite $socket, "$user\n") {
          unless (sysread $socket, $buffer, 1024) {
              # Should i use $! here?
              weechat::print("Unable to read from the socket: $!");
	  }
	} else {
		 # ..and here?
		weechat::print("Unable to write to the socket: $!");
	}
    } else {
	weechat::print("Connection to $host failed: $!");
   }
	return $buffer;
}

sub kernel_version {
    my @version;
    if (my $finger = finger("", "finger.kernel.org")) {
        # The magic of the regexps :)
        @version = $finger =~ /:\s*(\S+)\s*$/gm;
        # Modify this to do whatever you want.
        
	my %branches = (
        	26 => "",
	        24 => "",
        	22 => "",
	        20 => "",
	);

	foreach my $kernel (@version) {
        	if($kernel =~ /2\.6/) {
	                $branches{26} .= "   $kernel";
	        } elsif($kernel =~ /2\.4/) {
        	        $branches{24} .= "   $kernel";
	        } elsif ($kernel =~ /2\.2/) {
        	        $branches{22} .= "   $kernel";
	        } elsif ($kernel =~ /2\.0/) {
        	        $branches{20} .= "   $kernel";
	        }
	}

	my @keys = sort(keys(%branches));
	foreach my $key (@keys) {
	        weechat::print("branche " . join('.', split(//, $key)));
        	weechat::print("$branches{$key}");
	}

#        weechat::print("@version");
	return weechat::PLUGIN_RC_OK;
    }
}

