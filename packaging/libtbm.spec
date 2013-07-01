Name:           libtbm
Version:        1.0.5
Release:        2
License:        MIT
Summary:        Tizen Buffer Manager Library
Group:          System/Libraries
Source0:        %{name}-%{version}.tar.gz
Source1001: 	libtbm.manifest

BuildRequires:  pkgconfig(pthread-stubs)
BuildRequires:  pkgconfig(libdrm)

%description
The library for Tizen Buffer Manager.

%package devel
Summary:        Tizen Buffer Manager Library - Development
Group:          Development/Libraries
Requires:       libtbm = %{version}
Requires:       pkgconfig(libdrm)

%description devel
The library for Tizen Buffer Manager.

Development Files.

%prep
%setup -q
cp %{SOURCE1001} .

%build

%reconfigure --prefix=%{_prefix} \
            CFLAGS="${CFLAGS} -Wall -Werror" LDFLAGS="${LDFLAGS} -Wl,--hash-style=both -Wl,--as-needed"

make %{?_smp_mflags}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp -af COPYING %{buildroot}/usr/share/license/%{name}
%make_install


%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig


%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
/usr/share/license/%{name}
%{_libdir}/libtbm.so.*
%{_libdir}/libdrm_slp.so.*

%files devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%dir %{_includedir}
%{_includedir}/tbm_bufmgr.h
%{_includedir}/tbm_bufmgr_backend.h
%{_libdir}/libtbm.so
%{_libdir}/libdrm_slp.so
%{_libdir}/pkgconfig/libtbm.pc

