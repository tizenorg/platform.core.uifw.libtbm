Name:           libtbm
Version:        1.0.4
Release:        2
License:        MIT
Summary:        the library for Tizen Buffer Manager
Group:          System/Libraries
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  pkgconfig(pthread-stubs)
BuildRequires:  pkgconfig(libdrm)

%description
Description: %{summary}

%package devel
Summary:        the library for Tizen Buffer Manager
Group:          Development/Libraries
Requires:       libdrm2

%description devel
the library for Tizen Buffer Manager

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
%defattr(-,root,root,-)
/usr/share/license/%{name}
%{_libdir}/libtbm.so.*
%{_libdir}/libdrm_slp.so.*

%files devel
%defattr(-,root,root,-)
%dir %{_includedir}
%{_includedir}/tbm_bufmgr.h
%{_includedir}/tbm_bufmgr_backend.h
%{_libdir}/libtbm.so
%{_libdir}/libdrm_slp.so
%{_libdir}/pkgconfig/libtbm.pc

