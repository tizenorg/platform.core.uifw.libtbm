%bcond_with x
%bcond_with wayland

Name:           libtbm
Version:        1.2.2
Release:        1
License:        MIT
Summary:        The library for Tizen Buffer Manager
Group:          System/Libraries
Source0:        %{name}-%{version}.tar.gz
Source1001:		%name.manifest

BuildRequires:  pkgconfig(libdrm)
BuildRequires:  pkgconfig(wayland-server)
BuildRequires:  pkgconfig(wayland-client)
BuildRequires:  pkgconfig(pthread-stubs)
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


%__mkdir_p %{buildroot}%{_unitdir}
install -m 644 service/tbm-drm.service %{buildroot}%{_unitdir}
install -m 644 service/tbm-drm.path %{buildroot}%{_unitdir}
%__mkdir_p %{buildroot}%{_unitdir_user}
install -m 644 service/tbm-drm-user.service %{buildroot}%{_unitdir_user}
install -m 644 service/tbm-drm-user.path %{buildroot}%{_unitdir_user}

%clean
rm -rf %{buildroot}

%pre
%__mkdir_p %{_unitdir}/graphical.target.wants
ln -sf ../tbm-drm.path %{_unitdir}/graphical.target.wants/

%__mkdir_p %{_unitdir_user}/default.target.wants
ln -sf ../tbm-drm-user.path %{_unitdir_user}/default.target.wants/

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig
rm -f %{_unitdir}/graphical.target.wants/tbm-drm.path
rm -f %{_unitdir_user}/default.target.wants/tbm-drm-user.path

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
%{_includedir}/tbm_drm_helper.h
%{_unitdir}/tbm-drm.path
%{_unitdir}/tbm-drm.service
%{_unitdir_user}/tbm-drm-user.path
%{_unitdir_user}/tbm-drm-user.service
%{_libdir}/libtbm.so
%{_libdir}/pkgconfig/libtbm.pc
