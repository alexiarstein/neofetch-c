%global debug_package %{nil}
%undefine _debugsource_packages

Name:           neofetch-c
Version:        1.0.0
Release:        3%{?dist}
Summary:        A commandline system information tool written in C

License:        MIT
Group:          Applications/System
URL:            https://github.com/atomicturtle/neofetch-c
Source0:        https://github.com/atomicturtle/%{name}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildArch:      x86_64

BuildRequires:  gcc
BuildRequires:  make

Requires:       pciutils

%description
Neofetch-c is a commandline system information tool written in C.
It displays system information alongside an ASCII logo of your distribution.

%prep
%autosetup -n %{name}-%{version}

%build
make %{?_smp_mflags}

%install
# Manual installation since upstream Makefile doesn't support DESTDIR
install -D -m 0755 neofetch %{buildroot}%{_bindir}/neofetch

# Install ASCII art files
install -d -m 0755 %{buildroot}%{_datadir}/neofetch/ascii
install -m 0644 ascii/*.ascii %{buildroot}%{_datadir}/neofetch/ascii/

%files
%license LICENSE
%doc README.md
%{_bindir}/neofetch
%dir %{_datadir}/neofetch
%dir %{_datadir}/neofetch/ascii
%{_datadir}/neofetch/ascii/*.ascii

%changelog
* Thu Jan 16 2026 Scott R. Shinn <scott@atomicorp.com> - 1.0.0-3
- Fix ASCII art installation path to /usr/share/neofetch/ascii

* Thu Jan 16 2026 Scott R. Shinn <scott@atomicorp.com> - 1.0.0-2
- Add pciutils runtime dependency for lspci command

* Thu Jan 16 2026 Scott R. Shinn <scott@atomicorp.com> - 1.0.0-1
- Initial RPM release
