Name:           libtbm
Version:        1.1.0
Release:        3
License:        MIT
Summary:        The library for Tizen Buffer Manager
Group:          System/Libraries
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  pkgconfig(pthread-stubs)
BuildRequires:  pkgconfig(libdrm)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(libdri2)
BuildRequires:  pkgconfig(capi-base-common)

%description
Description: %{summary}

%package devel
Summary:        Tizen Buffer Manager Library - Development
Group:          Development/Libraries
Requires:       libtbm = %{version}
Requires:       pkgconfig(capi-base-common)

%description devel
The library for Tizen Buffer Manager.

Development Files.

%prep
%setup -q

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
%manifest libtbm.manifest
%defattr(-,root,root,-)
/usr/share/license/%{name}
%{_libdir}/libtbm.so.*
%{_libdir}/libdrm_slp.so.*

%files devel
%defattr(-,root,root,-)
%dir %{_includedir}
%{_includedir}/tbm_bufmgr.h
%{_includedir}/tbm_surface.h
%{_includedir}/tbm_surface_internal.h
%{_includedir}/tbm_bufmgr_backend.h
%{_includedir}/tbm_type.h
%{_libdir}/libtbm.so
%{_libdir}/libdrm_slp.so
%{_libdir}/pkgconfig/libtbm.pc

