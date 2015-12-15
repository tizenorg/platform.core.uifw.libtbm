%bcond_with x
%bcond_with wayland

Name:           libtbm
Version:        1.2.1
Release:        1
License:        MIT
Summary:        The library for Tizen Buffer Manager
Group:          System/Libraries
Source0:        %{name}-%{version}.tar.gz
Source1001:		%name.manifest

BuildRequires:  pkgconfig(libdrm)
BuildRequires:  pkgconfig(pthread-stubs)
%if %{with wayland}
BuildRequires:  pkgconfig
BuildRequires:  pkgconfig(wayland-client)
%else
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(libdri2)
%endif
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
cp %{SOURCE1001} .

%build

%if %{with wayland}
%reconfigure --prefix=%{_prefix} --with-tbm-platform=WAYLAND \
            CFLAGS="${CFLAGS} -Wall -Werror" LDFLAGS="${LDFLAGS} -Wl,--hash-style=both -Wl,--as-needed"
%else
%reconfigure --prefix=%{_prefix} --with-tbm-platform=X11 \
            CFLAGS="${CFLAGS} -Wall -Werror" LDFLAGS="${LDFLAGS} -Wl,--hash-style=both -Wl,--as-needed"
%endif

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

%files devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%dir %{_includedir}
%{_includedir}/tbm_bufmgr.h
%{_includedir}/tbm_surface.h
%{_includedir}/tbm_surface_internal.h
%{_includedir}/tbm_surface_queue.h
%{_includedir}/tbm_bufmgr_backend.h
%{_includedir}/tbm_type.h
%{_libdir}/libtbm.so
%{_libdir}/pkgconfig/libtbm.pc
