# Microsoft Developer Studio Project File - Name="GraySvr" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=GRAYSVR - WIN32 DEBUG
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "GraySvr.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GraySvr.mak" CFG="GRAYSVR - WIN32 DEBUG"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GraySvr - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "GraySvr - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "GraySvr - Win32 Testing" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GraySvr", FCAAAAAA"
# PROP Scc_LocalPath "d:\menace\graysvr"
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "GraySvr - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /G6 /Gr /MD /W2 /GR /GX /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "GRAY_SVR" /FD /c
# SUBTRACT CPP /Fr /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib /nologo /version:0.10 /subsystem:windows /map /debug /machine:I386 /out:"Release/sphereSvr.exe"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "GraySvr - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Gz /MDd /W2 /Gm /Gi /GR /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "GRAY_SVR" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib /nologo /version:0.12 /subsystem:windows /map /debug /machine:I386 /out:"Debug\sphereSvr.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /incremental:no

!ELSEIF  "$(CFG)" == "GraySvr - Win32 Testing"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "GraySvr___Win32_Testing"
# PROP BASE Intermediate_Dir "GraySvr___Win32_Testing"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Testing"
# PROP Intermediate_Dir "Testing"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /Gr /MD /W2 /GR /GX /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "GRAY_SVR" /FD /c
# SUBTRACT BASE CPP /Fr /YX
# ADD CPP /nologo /G6 /Gr /MD /W2 /GR /GX /Zi /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "GRAY_SVR" /Fr /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib /nologo /version:0.10 /subsystem:windows /map /debug /machine:I386 /out:"Release/sphereSvr.exe"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib /nologo /version:0.10 /subsystem:windows /map /debug /machine:I386 /out:"Release\sphereSvr.exe"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "GraySvr - Win32 Release"
# Name "GraySvr - Win32 Debug"
# Name "GraySvr - Win32 Testing"
# Begin Group "Sources"

# PROP Default_Filter "cpp,c,h,rc"
# Begin Source File

SOURCE=..\common\twofish\aes.h
# End Source File
# Begin Source File

SOURCE=.\CAccount.cpp
# End Source File
# Begin Source File

SOURCE=.\CBackTask.cpp
# End Source File
# Begin Source File

SOURCE=.\CBase.cpp
# End Source File
# Begin Source File

SOURCE=.\CChar.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharact.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharBase.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharFight.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharNPC.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharNPCAct.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharNPCPet.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharNPCStatus.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharSkill.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharSpell.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharStatus.cpp
# End Source File
# Begin Source File

SOURCE=.\CCharUse.cpp
# End Source File
# Begin Source File

SOURCE=.\CChat.cpp
# End Source File
# Begin Source File

SOURCE=.\CClient.cpp
# End Source File
# Begin Source File

SOURCE=.\CClientDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\CClientEvent.cpp
# End Source File
# Begin Source File

SOURCE=.\CClientGMPage.cpp
# End Source File
# Begin Source File

SOURCE=.\CClientLog.cpp
# End Source File
# Begin Source File

SOURCE=.\CClientMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\CClientTarg.cpp
# End Source File
# Begin Source File

SOURCE=.\CClientUse.cpp
# End Source File
# Begin Source File

SOURCE=.\CContain.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CData.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CData.h
# End Source File
# Begin Source File

SOURCE=..\common\CDataBase.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CDataBase.h
# End Source File
# Begin Source File

SOURCE=..\common\CEncrypt.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CEncrypt.h
# End Source File
# Begin Source File

SOURCE=.\CGMPage.cpp
# End Source File
# Begin Source File

SOURCE=.\CItem.cpp
# End Source File
# Begin Source File

SOURCE=.\CItemBase.cpp
# End Source File
# Begin Source File

SOURCE=.\CItemMulti.cpp
# End Source File
# Begin Source File

SOURCE=.\CItemSp.cpp
# End Source File
# Begin Source File

SOURCE=.\CItemStone.cpp
# End Source File
# Begin Source File

SOURCE=.\CItemVend.cpp
# End Source File
# Begin Source File

SOURCE=.\CLog.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CMD5.h
# End Source File
# Begin Source File

SOURCE=.\CObjBase.cpp
# End Source File
# Begin Source File

SOURCE=.\CPathFinder.cpp
# End Source File
# Begin Source File

SOURCE=.\CPathFinder.h
# End Source File
# Begin Source File

SOURCE=.\CProfileData.cpp
# End Source File
# Begin Source File

SOURCE=.\CQuest.cpp
# End Source File
# Begin Source File

SOURCE=.\CResource.cpp
# End Source File
# Begin Source File

SOURCE=.\CResourceCalc.cpp
# End Source File
# Begin Source File

SOURCE=.\CResourceDef.cpp
# End Source File
# Begin Source File

SOURCE=.\CSector.cpp
# End Source File
# Begin Source File

SOURCE=.\CServer.cpp
# End Source File
# Begin Source File

SOURCE=.\CServRef.cpp
# End Source File
# Begin Source File

SOURCE=.\CWebPage.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CWindow.h
# End Source File
# Begin Source File

SOURCE=.\CWorld.cpp

!IF  "$(CFG)" == "GraySvr - Win32 Release"

!ELSEIF  "$(CFG)" == "GraySvr - Win32 Debug"

!ELSEIF  "$(CFG)" == "GraySvr - Win32 Testing"

# SUBTRACT CPP /D "GRAY_SVR"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CWorldImport.cpp
# End Source File
# Begin Source File

SOURCE=.\CWorldMap.cpp
# End Source File
# Begin Source File

SOURCE=..\common\twofish\debug.h
# End Source File
# Begin Source File

SOURCE=.\exceptions.cpp
# End Source File
# Begin Source File

SOURCE=.\graysvr.cpp
# End Source File
# Begin Source File

SOURCE=.\GraySvr.rc
# End Source File
# Begin Source File

SOURCE=.\ntservice.cpp
# End Source File
# Begin Source File

SOURCE=.\ntwindow.cpp
# End Source File
# Begin Source File

SOURCE=..\common\twofish\platform.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=..\common\twofish\table.h
# End Source File
# Begin Source File

SOURCE=..\common\twofish\twofish2.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CAccount.h
# End Source File
# Begin Source File

SOURCE=.\CBase.h
# End Source File
# Begin Source File

SOURCE=.\CClient.h
# End Source File
# Begin Source File

SOURCE=.\CObjBase.h
# End Source File
# Begin Source File

SOURCE=.\CResource.h
# End Source File
# Begin Source File

SOURCE=.\CServRef.h
# End Source File
# Begin Source File

SOURCE=.\CWorld.h
# End Source File
# Begin Source File

SOURCE=.\graysvr.h
# End Source File
# End Group
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\calc.cpp
# End Source File
# Begin Source File

SOURCE=..\common\carray.cpp
# End Source File
# Begin Source File

SOURCE=..\common\carray.h
# End Source File
# Begin Source File

SOURCE=..\common\cassoc.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cassoc.h
# End Source File
# Begin Source File

SOURCE=..\common\catom.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CAtom.h
# End Source File
# Begin Source File

SOURCE=..\common\cexpression.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cexpression.h
# End Source File
# Begin Source File

SOURCE=..\common\cfile.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cfile.h
# End Source File
# Begin Source File

SOURCE=..\common\cfilelist.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cfilelist.h
# End Source File
# Begin Source File

SOURCE=..\common\cGrayData.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cgrayinst.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cgrayinst.h
# End Source File
# Begin Source File

SOURCE=..\common\cGrayMap.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cGrayMap.h
# End Source File
# Begin Source File

SOURCE=..\common\CMD5.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cmemblock.h
# End Source File
# Begin Source File

SOURCE=..\common\common.h
# End Source File
# Begin Source File

SOURCE=..\common\cQueue.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CQueue.h
# End Source File
# Begin Source File

SOURCE=..\common\crect.cpp
# End Source File
# Begin Source File

SOURCE=..\common\crect.h
# End Source File
# Begin Source File

SOURCE=..\common\cregion.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cregion.h
# End Source File
# Begin Source File

SOURCE=..\common\cresourcebase.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cresourcebase.h
# End Source File
# Begin Source File

SOURCE=..\common\cscript.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cscript.h
# End Source File
# Begin Source File

SOURCE=..\common\CScriptObj.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cscriptobj.h
# End Source File
# Begin Source File

SOURCE=..\common\cSectorTemplate.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cSectorTemplate.h
# End Source File
# Begin Source File

SOURCE=..\common\CSocket.cpp
# End Source File
# Begin Source File

SOURCE=..\common\csocket.h
# End Source File
# Begin Source File

SOURCE=..\common\cstring.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cstring.h
# End Source File
# Begin Source File

SOURCE=..\common\cthread.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cthread.h
# End Source File
# Begin Source File

SOURCE=..\common\CTime.cpp
# End Source File
# Begin Source File

SOURCE=..\common\ctime.h
# End Source File
# Begin Source File

SOURCE=..\common\cwindow.cpp
# End Source File
# Begin Source File

SOURCE=..\common\graycom.cpp
# End Source File
# Begin Source File

SOURCE=..\common\graycom.h
# End Source File
# Begin Source File

SOURCE=..\common\graymul.h
# End Source File
# Begin Source File

SOURCE=..\common\grayproto.h
# End Source File
# Begin Source File

SOURCE=..\common\grayver.h
# End Source File
# Begin Source File

SOURCE=.\ntservice.h
# End Source File
# End Group
# Begin Group "Other"

# PROP Default_Filter ""
# Begin Group "Scripts Test"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\scripts\test\blacksmith_menu.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\test\classmenus.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\test\clericbook.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\test\custom_vendor.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\test\helpgump2.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\test\helpgump3.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\test\housegump.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\test\housegump2.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\test\housegump3.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\test\necrobook.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\test\RewardSystem.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\test\runebook.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\test\runebook2.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\test\spherechar3.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\test\spheretest.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\test\Tinkering_menu.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\test\traveler.scp
# End Source File
# End Group
# Begin Group "Scripts Speech"

# PROP Default_Filter ""
# End Group
# Begin Group "Scripts bs3"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\scripts\bs3\bs2_crier.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\bs3\bs2_items.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\bs3\bs2_map.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\bs3\bs2_santa.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\bs3\bs2_stuff.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\bs3\bs3_borgvendors.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\bs3\bs3_classitems.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\bs3\bs3_drowvendors.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\bs3\bs3_dwarfvendors.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\bs3\bs3_elfvendors.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\bs3\bs3_gargoyle_town.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\bs3\bs3_halforcvendors.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\bs3\bs3_helpgump.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\bs3\bs3_ogre_town.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\bs3\bs3_ore_elem.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\bs3\bs3_patron.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\bs3\bs3_racemenu.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\bs3\bs3_troll_town.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\bs3\sphereclass.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\bs3\SphereRace.scp
# End Source File
# End Group
# Begin Group "Web Pages"

# PROP Default_Filter "htm"
# Begin Source File

SOURCE=..\public_html\gray\dev.htm
# End Source File
# Begin Source File

SOURCE=..\public_html\gray\ideas.htm
# End Source File
# Begin Source File

SOURCE=..\public_html\gray\index.html
# End Source File
# Begin Source File

SOURCE=..\public_html\gray\readme.htm
# End Source File
# End Group
# Begin Group "Data Files"

# PROP Default_Filter "scp"
# Begin Source File

SOURCE=.\docs\items.txt
# End Source File
# Begin Source File

SOURCE=.\docs\npcs.txt
# End Source File
# Begin Source File

SOURCE=.\docs\sounds.txt
# End Source File
# Begin Source File

SOURCE=.\docs\terrain.txt
# End Source File
# Begin Source File

SOURCE=.\docs\UoLog.txt
# End Source File
# End Group
# Begin Group "Scripts"

# PROP Default_Filter "scp,ini"
# Begin Source File

SOURCE=..\scripts\SPHEREBook.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\spherechar.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\spherechar_anim.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\spherechar_evil.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\spherechar_human.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\spherechar_misc.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\SPHEREDefs.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\spheredialog.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereevents.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\SPHEREhelp.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereirc.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitem2.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitem_colorarm.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitem_deed.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitem_magic_leather.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitem_magicarm.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitem_magicweap.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitem_maps.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitem_misc.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitem_multis.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitem_ore.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitem_potions.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitem_treas.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitem_wands.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitem_x.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitemb1.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitemb2.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitemb3.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitemb4.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitemb5.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitemb6.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereitemb7.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\SphereItemX.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\SPHEREmap.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\SPHEREMenu.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\SPHEREname.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\SPHEREnewb.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereregion.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\SphereResources.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereskill.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\spherespeech.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\spheretable_x.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\SPHERETables.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\spheretemp_loot.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\spheretemp_vend.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\SPHEREtemplate.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\SPHEREtrig.scp
# End Source File
# Begin Source File

SOURCE=..\scripts\sphereweb.scp
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=..\knownbugs.txt
# End Source File
# Begin Source File

SOURCE=.\REVISIONS.txt
# End Source File
# Begin Source File

SOURCE=..\sphere.dic
# End Source File
# Begin Source File

SOURCE=..\Sphere.ini
# End Source File
# Begin Source File

SOURCE=.\spheresvr.ico
# End Source File
# Begin Source File

SOURCE=..\TODO.TXT
# End Source File
# End Target
# End Project
