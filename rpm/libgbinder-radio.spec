Name: libgbinder-radio
Version: 1.2.6
Release: 0
Summary: Binder client library for Android radio interfaces
Group: Development/Libraries
License: BSD
URL: https://github.com/mer-hybris/libgbinder-radio
Source: %{name}-%{version}.tar.bz2

%define libgbinder_version 1.0.9

BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(libglibutil)
BuildRequires: pkgconfig(libgbinder) >= %{libgbinder_version}
Requires: libgbinder >= %{libgbinder_version}
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
Binder client library for Android radio interfaces

%package devel
Summary: Development library for %{name}
Requires: %{name} = %{version}
Requires: pkgconfig

%description devel
This package contains the development library for %{name}.

%prep
%setup -q

%build
make LIBDIR=%{_libdir} KEEP_SYMBOLS=1 release pkgconfig

%install
rm -rf %{buildroot}
make LIBDIR=%{_libdir} DESTDIR=%{buildroot} install-dev

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/%{name}.so.*

%files devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/*.pc
%{_libdir}/%{name}.so
%{_includedir}/gbinder-radio/*.h
