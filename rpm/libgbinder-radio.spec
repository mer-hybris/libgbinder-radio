Name: libgbinder-radio

Version: 1.4.7
Release: 0
Summary: Binder client library for Android radio interfaces
License: BSD
URL: https://github.com/mer-hybris/libgbinder-radio
Source: %{name}-%{version}.tar.bz2

%define libgbinder_version 1.1.14
%define libglibutil_version 1.0.49

BuildRequires: pkgconfig
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(libglibutil) >= %{libglibutil_version}
BuildRequires: pkgconfig(libgbinder) >= %{libgbinder_version}

# license macro requires rpm >= 4.11
BuildRequires: pkgconfig(rpm)
%define license_support %(pkg-config --exists 'rpm >= 4.11'; echo $?)

Requires: libglibutil >= %{libglibutil_version}
Requires: libgbinder >= %{libgbinder_version}
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
Binder client library for Android radio interfaces

%package devel
Summary: Development library for %{name}
Requires: %{name} = %{version}

%description devel
This package contains the development library for %{name}.

%prep
%setup -q

%build
make %{_smp_mflags} LIBDIR=%{_libdir} KEEP_SYMBOLS=1 release pkgconfig

%install
rm -rf %{buildroot}
make LIBDIR=%{_libdir} DESTDIR=%{buildroot} install-dev

%check
make -C unit test

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/%{name}.so.*
%if %{license_support} == 0
%license LICENSE
%endif

%files devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/*.pc
%{_libdir}/%{name}.so
%{_includedir}/gbinder-radio/*.h
