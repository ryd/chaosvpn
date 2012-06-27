; /*
;     chaosvpn.nsi -- NSIS installer script for chaosvpn
;     Copyright (C) 2012 Guus Sliepen <guus@tinc-vpn.org>
; 
;     This program is free software; you can redistribute it and/or modify
;     it under the terms of the GNU General Public License as published by
;     the Free Software Foundation; either version 2 of the License, or
;     (at your option) any later version.
; 
;     This program is distributed in the hope that it will be useful,
;     but WITHOUT ANY WARRANTY; without even the implied warranty of
;     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;     GNU General Public License for more details.
; 
;     You should have received a copy of the GNU General Public License
;     along with this program; if not, write to the Free Software
;     Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
; */

!include "MUI.nsh"

!define VERSION "2.12"

;General

Name "chaosvpn ${VERSION}"

OutFile "chaosvpn-${VERSION}-install.exe"

InstallDir "$PROGRAMFILES\tinc"
InstallDirRegKey HKLM "Software\chaosvpn" ""

;Interface settings

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "chaosknoten1.bmp"
!define MUI_ABORTWARNING

!define MUI_LICENSEPAGE_TEXT_BOTTOM "If you have read the license, click OK to continue. You do not have to agree with this license if you do not intend to distribute copies of this software."
!define MUI_LICENSEPAGE_BUTTON "OK"

!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README.txt"
!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED

;Pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

;Languages

!insertmacro MUI_LANGUAGE "English"

;Installer section

Section "The chaosvpn daemon" SecDaemon

SetOutPath "$INSTDIR"
SetOverwrite on
File "chaosvpn.exe"
File "chaosvpn.conf"
File "LICENSE.txt"
File "README.txt"
WriteRegStr HKLM "Software\chaosvpn" "" $INSTDIR

WriteUninstaller "$INSTDIR\Uninstall.exe"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\chaosvpn" "DisplayName" "ChaosVPN ${VERSION}"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\chaosvpn" "UninstallString" '"$INSTDIR\Uninstall.exe"'
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\chaosvpn" "DisplayVersion" "${VERSION}"

SectionEnd

Section "The documentation" SecDocumentation

SetOutPath "$INSTDIR\doc"
SetOverwrite on
File "chaosvpn.1.html"
File "chaosvpn.conf.5.html"

SectionEnd

;Uninstaller section

Section "Uninstall"

Delete "$INSTDIR\doc\chaosvpn.1.html"
Delete "$INSTDIR\doc\chaosvpn.conf.5.html"
RMDir "$INSTDIR\doc"
Delete "$INSTDIR\chaosvpn.exe"
Delete "$INSTDIR\chaosvpn.conf"
Delete "$INSTDIR\LICENSE.txt"
Delete "$INSTDIR\README.txt"
Delete "$INSTDIR\Uninstall.exe"
RMDir "$INSTDIR"
DeleteRegKey HKLM "Software\chaosvpn"
DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\chaosvpn"

SectionEnd
