Name: efind
Version: 0.1.0
Release:        1%{?dist}
Summary: An extendable wrapper for GNU find.

License: GPLv3+
URL: https://github.com/20centaurifux/efind
Source0: efind-0.1.0.tar.xz

BuildRequires: bison flex
Requires: findutils

%description
efind is a wrapper for GNU find providing an easier and more
intuitive expression syntax. It can be extended by custom
functions to filter search results.

%prep
%setup -q


%build
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
%make_install


%files
%{_bindir}/*
%{_mandir}/man1/*



%changelog
