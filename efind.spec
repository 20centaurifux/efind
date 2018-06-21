Name: efind
Version: 0.4.2
Release:        1%{?dist}
Summary: An extendable wrapper for GNU find.

License: GPLv3+
URL: http://efind.dixieflatline.de
Source0: efind-0.4.2.tar.xz

BuildRequires: bison flex python-devel libffi-devel gettext
Requires: findutils libffi python-libs

%description
efind is a wrapper for GNU find providing an easier and more
intuitive expression syntax. It can be extended by custom
functions to filter search results.

%global debug_package %{nil}

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
%{_datarootdir}/efind/*
%{_datarootdir}/locale/*



%changelog
