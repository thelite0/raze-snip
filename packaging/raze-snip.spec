Name:           raze-snip
Version:        0.1.2
Release:        1%{?dist}
Summary:        Clipboard-first capture overlay for KWin Wayland
License:        LicenseRef-Raze-Snip-1.0
Source0:        %{name}-%{version}.tar.gz
BuildRequires:  gcc-c++
BuildRequires:  cmake
BuildRequires:  ninja-build
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtbase-private-devel
BuildRequires:  wayland-devel
BuildRequires:  wayland-protocols-devel
BuildRequires:  pkgconf-pkg-config
BuildRequires:  qt6-qtdeclarative-devel
BuildRequires:  layer-shell-qt-devel
BuildRequires:  kf6-kglobalaccel-devel
BuildRequires:  kf6-kguiaddons-devel >= 6.29
Requires:       qt6-qtwayland
Requires:       qt6-qtdeclarative

%description
Native Qt 6 capture overlay for KDE Plasma 6 on Wayland, using the authorized
org.kde.KWin.ScreenShot2 interface and KDE's clipboard and global shortcuts.

%prep
%autosetup

%build
%cmake -G Ninja -DBUILD_TESTING=OFF -DCMAKE_INSTALL_SYSCONFDIR=/etc
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%doc README.md CONTRIBUTING.md docs/UX-reference.md
%{_bindir}/raze-snip
%{_datadir}/applications/org.raze.snip.desktop
%{_datadir}/dbus-1/services/org.raze.snip.service
%{_sysconfdir}/xdg/autostart/org.raze.snip.desktop

%changelog
* Mon Oct 05 2026 Raze <raze@localhost> - 0.1.2-1
- Suppress KWin hot corners during capture without changing desktop settings

* Mon Oct 05 2026 Raze <raze@localhost> - 0.1.1-1
- Show capture surfaces without scale or fade animations; support Print shortcut

* Mon Oct 05 2026 Raze <raze@localhost> - 0.1.0-1
- Initial capture overlay
