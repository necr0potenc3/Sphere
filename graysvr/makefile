VER	= 0.56a
PLAT	= Fc2

# Generic makefile
OPT 	= -O2 -fno-omit-frame-pointer -ffast-math -fno-expensive-optimizations -march=i686
#WARN	= -Wall -W
WARN	= -w

DEBUG	= -ggdb2
DEBUG	= -s
INCLUDE	= -I../common

# Linux
LIBS	= -L/usr/lib -lpthread
# FreeBSD
#LIBS	= -L/usr/lib -pthread
#DEFBSD	= -D_BSD

DEFINES = -DGRAY_SVR -D_CONSOLE $(DEFBSD)

EXE	= graysvr
REXE	= spheresvr

CC	= g++

NO	= -fno-rtti -fno-exceptions
#EX	= -fexceptions -fasynchronous-exceptions -fsjlj-exceptions
EX	= -fexceptions -fnon-call-exceptions
STRICT  = # -mstrict-align
SPECIAL = $(EX) $(STRICT) $(DEBUG)

PROF	= -pg
PIPE	= -pipe

SRC	:= 	CAccount.cpp \
		CBackTask.cpp \
		CBase.cpp \
		CChar.cpp \
		CCharact.cpp \
		CCharBase.cpp \
		CCharFight.cpp \
		CCharNPC.cpp \
		CCharNPCAct.cpp \
		CCharNPCPet.cpp \
		CCharNPCStatus.cpp \
		CCharSkill.cpp \
		CCharSpell.cpp \
		CCharStatus.cpp \
		CCharUse.cpp \
		CChat.cpp \
		CClient.cpp \
		CClientDialog.cpp \
		CClientEvent.cpp \
		CClientGMPage.cpp \
		CClientLog.cpp \
		CClientMsg.cpp \
		CClientTarg.cpp \
		CClientUse.cpp \
		CContain.cpp \
		CGMPage.cpp \
		CItem.cpp \
		CItemBase.cpp \
		CItemMulti.cpp \
		CItemSp.cpp \
		CItemStone.cpp \
		CItemVend.cpp \
		CLog.cpp \
		CObjBase.cpp \
		CPathFinder.cpp \
		CProfileData.cpp \
		CResource.cpp \
		CResourceCalc.cpp \
		CResourceDef.cpp \
		CSector.cpp \
		CServer.cpp \
		CServRef.cpp \
		CQuest.cpp \
		CWebPage.cpp \
		CWorld.cpp \
		CWorldImport.cpp \
		CWorldMap.cpp \
		graysvr.cpp \
		../common/twofish/twofish2.cpp \
		../common/CArray.cpp \
		../common/CAtom.cpp \
		../common/CAssoc.cpp \
		../common/CDataBase.cpp \
		../common/CEncrypt.cpp \
		../common/CExpression.cpp \
		../common/CFile.cpp \
		../common/CFileList.cpp \
		../common/CGrayData.cpp \
		../common/CGrayInst.cpp \
		../common/CGrayMap.cpp \
		../common/CMD5.cpp \
		../common/CQueue.cpp \
		../common/CRect.cpp \
		../common/CRegion.cpp \
		../common/CResourceBase.cpp \
		../common/CScript.cpp \
		../common/CScriptObj.cpp \
		../common/CSectorTemplate.cpp \
		../common/CSocket.cpp \
		../common/CTime.cpp \
		../common/Calc.cpp \
		../common/CString.cpp \
		../common/CThread.cpp \
		../common/graycom.cpp

O_FLAGS	= $(WARN) $(PIPE) $(SPECIAL) $(STATIC) 
C_FLAGS	= $(OPT) $(INCLUDE) $(DEFINES)


.PHONY:	all clean tidy

all:	$(EXE)

clean:	tidy
	rm -f *.o ../common/*.o ../common/twofish/*.o $(EXE) $(REXE)
#	$(MAKE)

.depend:
	gcc -MM $(INCLUDE) $(SRC) $(COMMONSRC) > .depend
	perl -pi -e 's/([^.]+)\.o/o\/\1.o/g' .depend

tidy:
	rm -f *~ *orig *bak *rej ../common/*~ ../common/*orig ../common*bak ../common/twofish/*~ ../common/twofish/*orig ../common/twofish/*bak

tags:	$(SRC)
	ctags $(SRC)

#cmn:	$(COMMONSRC:.cpp=.o)

gray:	$(SRC:.cpp=.o)

#commonlib:	$(COMMONSRC:.cpp=.o)
#	ar r $@ $?

flags:  
	@echo "Compiler Flags: $(CC) -c $(O_FLAGS) $(C_FLAGS)"

$(EXE): flags gray
	g++ $(O_FLAGS) $(C_FLAGS) $(LIBS) -o $(EXE) *.o ../common/*.o ../common/twofish/*.o

version.h:	VERSION
	echo "#define VERSION 'head -l VERSION'" > version.h
	echo "#define COMPILED_BY \"$(COMPILED_BY)\"" >> version.h

%.o:	%.cpp
	@echo " Compiling $<"
	@$(CC) -c $(O_FLAGS) $(C_FLAGS) $< -o $@

dist:
	@cp $(EXE) $(REXE)
	@cp REVISIONS.TXT Revisions$(VER).txt
	@cp MANUAL.TXT Manual$(VER).txt
	@tar cvfz SphereServer$(VER)-$(PLAT).tgz $(REXE)  Revisions$(VER).txt  Manual$(VER).txt  ../scripts/sphere_msgs.scp ../scripts/sphere_types.scp ../scripts/sphere_serv_triggers.scp
	@echo DONE -------
	@ls -la SphereServer$(VER)-$(PLAT).tgz
	@tar tvfz SphereServer$(VER)-$(PLAT).tgz
	@echo -----------
#	@scp SphereServer$(VER)-$(PLAT).tgz REVISIONS.TXT MANUAL.TXT uo@www.dagonar.com:~/public_html/sphere/files/
	@rm Revisions$(VER).txt Manual$(VER).txt $(REXE)
