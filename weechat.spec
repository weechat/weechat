%define name weechat
%define version 0.0.3
%define release 1

Name:      %{name}
Summary:   portable, fast, light and extensible IRC client
Version:   %{version}
Release:   %{release}
Source:    http://weechat.flashtux.org/download/%{name}-%{version}.tar.gz
URL:       http://weechat.flashtux.org
Group:     Networking/IRC
BuildRoot: %{_tmppath}/%{name}-buildroot
License:   GPL

%description
WeeChat (Wee Enhanced Environment for Chat) is a portable, fast, light and
extensible IRC client. Everything can be done with a keyboard.
It is customizable and extensible with scripts.

%prep
rm -rf $RPM_BUILD_ROOT 
%setup

%build
./configure
make DESTDIR="$RPM_BUILD_ROOT" LOCALRPM="local"

%install 
make DESTDIR="$RPM_BUILD_ROOT" LOCALRPM="local" install

%find_lang %name

%clean 
rm -rf $RPM_BUILD_ROOT 

%files -f %{name}.lang
%defattr(-,root,root,0755) 
%doc AUTHORS BUGS ChangeLog COPYING FAQ INSTALL NEWS README TODO
/usr/local/man/man1/weechat.1*
/usr/local/bin/weechat-curses

%changelog 
* Mon Nov 03 2003 FlashCode <flashcode@flashtux.org> 0.0.3-1
- Released version 0.0.3
* Sun Oct 05 2003 FlashCode <flashcode@flashtux.org> 0.0.2-1
- Released version 0.0.2
* Sat Sep 27 2003 FlashCode <flashcode@flashtux.org> 0.0.1-1
- Released version 0.0.1
