#
# Copyright (C) 2003-2017 Sébastien Helleu <flashcode@flashtux.org>
#
# This file is part of WeeChat, the extensible chat client.
#
# WeeChat is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# WeeChat is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
#
#

#
# This RPM spec file is designed for SuSE
#

%define name weechat
%define version 1.7.1
%define release 1

Name:      %{name}
Summary:   portable, fast, light and extensible IRC client
Version:   %{version}
Release:   %{release}
Source:    https://weechat.org/files/src/%{name}-%{version}.tar.gz
URL:       https://weechat.org/
Group:     Productivity/Networking/IRC
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
Requires:  perl, python, ruby, lua, tcl, guile, libv8, gcrypt, gnutls, ncurses, libcurl
License:   GPL
Vendor:    Sébastien Helleu <flashcode@flashtux.org>
Packager:  [Odin] <odin@dtdm.org>

%description
WeeChat (Wee Enhanced Environment for Chat) is a portable, fast, light and
extensible chat client. Everything can be done with a keyboard.
It is customizable and extensible with scripts.

%prep
rm -rf $RPM_BUILD_ROOT
%setup

%build
./autogen.sh
./configure --prefix=/usr --mandir=/usr/share/man --enable-doc --enable-man --with-debug=0
make

%install
%makeinstall
mkdir -p $RPM_BUILD_ROOT%{_libdir}/%{name}/plugins/
mv $RPM_BUILD_ROOT%{_libdir}/*.* $RPM_BUILD_ROOT%{_libdir}/%{name}/plugins/

%find_lang %name

%clean
rm -rf $RPM_BUILD_ROOT

%files -f %{name}.lang
%defattr(-,root,root,0755)
%doc AUTHORS.adoc ChangeLog.adoc Contributing.adoc COPYING README.adoc ReleaseNotes.adoc
%doc %{_docdir}/%{name}/*.html
%{_mandir}/man1/%{name}.1*
%{_mandir}/*/man1/%{name}.1*
%{_bindir}/%{name}
%{_bindir}/%{name}-curses
%{_libdir}/%{name}/plugins/*
%{_libdir}/pkgconfig/weechat.pc
%{_includedir}/%{name}/weechat-plugin.h
%{_prefix}/share/icons/hicolor/32x32/apps/weechat.png

%changelog
* Sat Apr 22 2017 Sébastien Helleu <flashcode@flashtux.org> 1.7.1-1
- Released version 1.7.1
* Sun Jan 15 2017 Sébastien Helleu <flashcode@flashtux.org> 1.7-1
- Released version 1.7
* Sun Oct 02 2016 Sébastien Helleu <flashcode@flashtux.org> 1.6-1
- Released version 1.6
* Sun May 01 2016 Sébastien Helleu <flashcode@flashtux.org> 1.5-1
- Released version 1.5
* Sun Jan 10 2016 Sébastien Helleu <flashcode@flashtux.org> 1.4-1
- Released version 1.4
* Sun Aug 16 2015 Sébastien Helleu <flashcode@flashtux.org> 1.3-1
- Released version 1.3
* Sun May 10 2015 Sébastien Helleu <flashcode@flashtux.org> 1.2-1
- Released version 1.2
* Sun Jan 25 2015 Sébastien Helleu <flashcode@flashtux.org> 1.1.1-1
- Released version 1.1.1
* Sun Jan 11 2015 Sébastien Helleu <flashcode@flashtux.org> 1.1-1
- Released version 1.1
* Sun Sep 28 2014 Sébastien Helleu <flashcode@flashtux.org> 1.0.1-1
- Released version 1.0.1
* Fri Aug 15 2014 Sébastien Helleu <flashcode@flashtux.org> 1.0-1
- Released version 1.0
* Sun Feb 09 2014 Sébastien Helleu <flashcode@flashtux.org> 0.4.3-1
- Released version 0.4.3
* Sun Oct 06 2013 Sébastien Helleu <flashcode@flashtux.org> 0.4.2-1
- Released version 0.4.2
* Mon May 20 2013 Sébastien Helleu <flashcode@flashtux.org> 0.4.1-1
- Released version 0.4.1
* Sun Jan 20 2013 Sébastien Helleu <flashcode@flashtux.org> 0.4.0-1
- Released version 0.4.0
* Sat Sep 29 2012 Sébastien Helleu <flashcode@flashtux.org> 0.3.9-1
- Released version 0.3.9
* Sun Jun 03 2012 Sébastien Helleu <flashcode@flashtux.org> 0.3.8-1
- Released version 0.3.8
* Sun Feb 26 2012 Sébastien Helleu <flashcode@flashtux.org> 0.3.7-1
- Released version 0.3.7
* Sat Oct 22 2011 Sébastien Helleu <flashcode@flashtux.org> 0.3.6-1
- Released version 0.3.6
* Sun May 15 2011 Sébastien Helleu <flashcode@flashtux.org> 0.3.5-1
- Released version 0.3.5
* Sun Jan 16 2011 Sébastien Helleu <flashcode@flashtux.org> 0.3.4-1
- Released version 0.3.4
* Sat Aug 07 2010 Sébastien Helleu <flashcode@flashtux.org> 0.3.3-1
- Released version 0.3.3
* Sun Apr 18 2010 Sébastien Helleu <flashcode@flashtux.org> 0.3.2-1
- Released version 0.3.2
* Sat Jan 23 2010 Sébastien Helleu <flashcode@flashtux.org> 0.3.1-1
- Released version 0.3.1
* Sun Sep 06 2009 Sébastien Helleu <flashcode@flashtux.org> 0.3.0-1
- Released version 0.3.0
* Thu Sep 06 2007 Sébastien Helleu <flashcode@flashtux.org> 0.2.6-1
- Released version 0.2.6
* Thu Jun 07 2007 Sébastien Helleu <flashcode@flashtux.org> 0.2.5-1
- Released version 0.2.5
* Thu Mar 29 2007 Sébastien Helleu <flashcode@flashtux.org> 0.2.4-1
- Released version 0.2.4
* Wed Jan 10 2007 Sébastien Helleu <flashcode@flashtux.org> 0.2.3-1
- Released version 0.2.3
* Sat Jan 06 2007 Sébastien Helleu <flashcode@flashtux.org> 0.2.2-1
- Released version 0.2.2
* Sun Oct 01 2006 Sébastien Helleu <flashcode@flashtux.org> 0.2.1-1
- Released version 0.2.1
* Sat Aug 19 2006 Sébastien Helleu <flashcode@flashtux.org> 0.2.0-1
- Released version 0.2.0
* Thu May 25 2006 Sébastien Helleu <flashcode@flashtux.org> 0.1.9-1
- Released version 0.1.9
* Sat Mar 18 2006 Sébastien Helleu <flashcode@flashtux.org> 0.1.8-1
- Released version 0.1.8
* Sat Jan 14 2006 Sébastien Helleu <flashcode@flashtux.org> 0.1.7-1
- Released version 0.1.7
* Fri Nov 11 2005 Sébastien Helleu <flashcode@flashtux.org> 0.1.6-1
- Released version 0.1.6
* Sat Sep 24 2005 Sébastien Helleu <flashcode@flashtux.org> 0.1.5-1
- Released version 0.1.5
* Sat Jul 30 2005 Sébastien Helleu <flashcode@flashtux.org> 0.1.4-1
- Released version 0.1.4
* Sat Jul 02 2005 Sébastien Helleu <flashcode@flashtux.org> 0.1.3-1
- Released version 0.1.3
* Sat May 21 2005 Sébastien Helleu <flashcode@flashtux.org> 0.1.2-1
- Released version 0.1.2
* Sat Mar 20 2005 Sébastien Helleu <flashcode@flashtux.org> 0.1.1-1
- Released version 0.1.1
* Sat Feb 12 2005 Sébastien Helleu <flashcode@flashtux.org> 0.1.0-1
- Released version 0.1.0
* Sat Jan 01 2005 Sébastien Helleu <flashcode@flashtux.org> 0.0.9-1
- Released version 0.0.9
* Sat Oct 30 2004 Sébastien Helleu <flashcode@flashtux.org> 0.0.8-1
- Released version 0.0.8
* Sat Aug 08 2004 Sébastien Helleu <flashcode@flashtux.org> 0.0.7-1
- Released version 0.0.7
* Sat Jun 05 2004 Sébastien Helleu <flashcode@flashtux.org> 0.0.6-1
- Released version 0.0.6
* Thu Feb 02 2004 Sébastien Helleu <flashcode@flashtux.org> 0.0.5-1
- Released version 0.0.5
* Thu Jan 01 2004 Sébastien Helleu <flashcode@flashtux.org> 0.0.4-1
- Released version 0.0.4
* Mon Nov 03 2003 Sébastien Helleu <flashcode@flashtux.org> 0.0.3-1
- Released version 0.0.3
* Sun Oct 05 2003 Sébastien Helleu <flashcode@flashtux.org> 0.0.2-1
- Released version 0.0.2
* Sat Sep 27 2003 Sébastien Helleu <flashcode@flashtux.org> 0.0.1-1
- Released version 0.0.1
