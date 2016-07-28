Name:           libtbm
Version:        2.0.0
Release:        1
License:        MIT
Summary:        The library for Tizen Buffer Manager
Group:          System/Libraries
Source0:        %{name}-%{version}.tar.gz

%description
Description: %{summary}

%prep
# our %{name}-%{version}.tar.gz archive hasn't top-level directory,
# so we create it
%setup -q -c %{name}-%{version}

%build
PKG_CONFIG_PATH=/usr/local/lib/pkgconfig \
./autogen.sh --build=x86_64-unknown-linux-gnu \
	     --host=arm-linux-androideabi \
	     --enable-android-support \
	     CFLAGS="${CFLAGS} -Wall -Werror" \
	     LDFLAGS="${LDFLAGS} -Wl,--hash-style=both -Wl,--as-needed"
	     
make %{?_smp_mflags}

%install
rm -rf %{buildroot}

%make_install

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
/usr/local/*
