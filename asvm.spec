Name: asvm
Version: 0.1
Release: 1
Summary: Service monitoring tool
Group: System/Base
License: GPLv2
Source0: %{name}-%{version}.tar.gz
BuildRequires: doxygen

%description
%{summary}.

%files
%defattr(-,root,root,-)
%{_bindir}/sv*
%{_sbindir}/asvm


%prep
%setup -q


%build
make


%install
make DESTDIR=%{buildroot} install
