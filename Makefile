#Connor Cormier
#gtkmm Makefile

CXX = g++
CXXFLAGS = -Wall -O2 `pkg-config gtkmm-3.0 --cflags --libs` `pkg-config thrift --libs --cflags` `pkg-config webkitgtk-3.0 --cflags --libs` `pkg-config sqlite3 --cflags --libs` -I'$(CURDIR)'

SRC_DIR = "src/"
SRCS = $(shell find . -name "*.cpp")
EXE_NAME = betternote

OFILES = $(SRCS:.cpp=.o)

.SUFFIXES: .o .cpp

link: $(OFILES)
	$(CXX) $(CXXFLAGS) $^ -o $(EXE_NAME)

#update dependencies
up_deps: $(SRCS)
	sed '/#START\ DEPS/q' Makefile > Makefile.out
	$(CXX) -I'$(CURDIR)' -MM $(SRCS) $^ >> Makefile.out
	mv Makefile.out Makefile

clean:
	find ./$(SRC_DIR) -name *~ -o -name *# -o -name *.o -delete

burn:
	find . -name *~ -o -name *# -o -name *.o -delete

#START DEPS
NoteStore_constants.o: evernote_api/src/NoteStore_constants.cpp \
 evernote_api/src/NoteStore_constants.h \
 evernote_api/src/NoteStore_types.h evernote_api/src/UserStore_types.h \
 evernote_api/src/Types_types.h evernote_api/src/Limits_types.h \
 evernote_api/src/Errors_types.h
Errors_constants.o: evernote_api/src/Errors_constants.cpp \
 evernote_api/src/Errors_constants.h evernote_api/src/Errors_types.h
Errors_types.o: evernote_api/src/Errors_types.cpp \
 evernote_api/src/Errors_types.h
Limits_constants.o: evernote_api/src/Limits_constants.cpp \
 evernote_api/src/Limits_constants.h evernote_api/src/Limits_types.h
Limits_types.o: evernote_api/src/Limits_types.cpp \
 evernote_api/src/Limits_types.h
NoteStore.o: evernote_api/src/NoteStore.cpp evernote_api/src/NoteStore.h \
 evernote_api/src/NoteStore_types.h evernote_api/src/UserStore_types.h \
 evernote_api/src/Types_types.h evernote_api/src/Limits_types.h \
 evernote_api/src/Errors_types.h
NoteStore_types.o: evernote_api/src/NoteStore_types.cpp \
 evernote_api/src/NoteStore_types.h evernote_api/src/UserStore_types.h \
 evernote_api/src/Types_types.h evernote_api/src/Limits_types.h \
 evernote_api/src/Errors_types.h
Types_constants.o: evernote_api/src/Types_constants.cpp \
 evernote_api/src/Types_constants.h evernote_api/src/Types_types.h \
 evernote_api/src/Limits_types.h
Types_types.o: evernote_api/src/Types_types.cpp \
 evernote_api/src/Types_types.h evernote_api/src/Limits_types.h
UserStore.o: evernote_api/src/UserStore.cpp evernote_api/src/UserStore.h \
 evernote_api/src/UserStore_types.h evernote_api/src/Types_types.h \
 evernote_api/src/Limits_types.h evernote_api/src/Errors_types.h
UserStore_constants.o: evernote_api/src/UserStore_constants.cpp \
 evernote_api/src/UserStore_constants.h \
 evernote_api/src/UserStore_types.h evernote_api/src/Types_types.h \
 evernote_api/src/Limits_types.h evernote_api/src/Errors_types.h
UserStore_types.o: evernote_api/src/UserStore_types.cpp \
 evernote_api/src/UserStore_types.h evernote_api/src/Types_types.h \
 evernote_api/src/Limits_types.h evernote_api/src/Errors_types.h
NotebookTreeStore.o: src/NotebookTreeStore.cpp src/NotebookTreeStore.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h
BetternoteUtils.o: src/BetternoteUtils.cpp src/BetternoteUtils.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h
DatabaseClient.o: src/DatabaseClient.cpp src/DatabaseClient.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h
EvernoteClient.o: src/EvernoteClient.cpp src/EvernoteClient.h \
 src/DatabaseClient.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_constants.h
Main.o: src/Main.cpp src/NoteWindow.h src/EvernoteClient.h \
 src/DatabaseClient.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_constants.h \
 src/NoteView.h src/NotebookSelector.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 src/NotebookTreeStore.h src/NoteSelector.h src/BetternoteUtils.h
NotebookSelector.o: src/NotebookSelector.cpp src/NotebookSelector.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h \
 src/NotebookTreeStore.h
NoteListStore.o: src/NoteListStore.cpp src/NoteListStore.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h
NoteSelector.o: src/NoteSelector.cpp src/NoteSelector.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h \
 src/NoteListStore.h
NoteView.o: src/NoteView.cpp src/NoteView.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h \
 src/DatabaseClient.h src/BetternoteUtils.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h
NoteWindow.o: src/NoteWindow.cpp src/NoteWindow.h src/EvernoteClient.h \
 src/DatabaseClient.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_constants.h \
 src/NoteView.h src/NotebookSelector.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 src/NotebookTreeStore.h src/NoteSelector.h src/BetternoteUtils.h
NoteStore_constants.o: evernote_api/src/NoteStore_constants.cpp \
 evernote_api/src/NoteStore_constants.h \
 evernote_api/src/NoteStore_types.h evernote_api/src/UserStore_types.h \
 evernote_api/src/Types_types.h evernote_api/src/Limits_types.h \
 evernote_api/src/Errors_types.h
Errors_constants.o: evernote_api/src/Errors_constants.cpp \
 evernote_api/src/Errors_constants.h evernote_api/src/Errors_types.h
Errors_types.o: evernote_api/src/Errors_types.cpp \
 evernote_api/src/Errors_types.h
Limits_constants.o: evernote_api/src/Limits_constants.cpp \
 evernote_api/src/Limits_constants.h evernote_api/src/Limits_types.h
Limits_types.o: evernote_api/src/Limits_types.cpp \
 evernote_api/src/Limits_types.h
NoteStore.o: evernote_api/src/NoteStore.cpp evernote_api/src/NoteStore.h \
 evernote_api/src/NoteStore_types.h evernote_api/src/UserStore_types.h \
 evernote_api/src/Types_types.h evernote_api/src/Limits_types.h \
 evernote_api/src/Errors_types.h
NoteStore_types.o: evernote_api/src/NoteStore_types.cpp \
 evernote_api/src/NoteStore_types.h evernote_api/src/UserStore_types.h \
 evernote_api/src/Types_types.h evernote_api/src/Limits_types.h \
 evernote_api/src/Errors_types.h
Types_constants.o: evernote_api/src/Types_constants.cpp \
 evernote_api/src/Types_constants.h evernote_api/src/Types_types.h \
 evernote_api/src/Limits_types.h
Types_types.o: evernote_api/src/Types_types.cpp \
 evernote_api/src/Types_types.h evernote_api/src/Limits_types.h
UserStore.o: evernote_api/src/UserStore.cpp evernote_api/src/UserStore.h \
 evernote_api/src/UserStore_types.h evernote_api/src/Types_types.h \
 evernote_api/src/Limits_types.h evernote_api/src/Errors_types.h
UserStore_constants.o: evernote_api/src/UserStore_constants.cpp \
 evernote_api/src/UserStore_constants.h \
 evernote_api/src/UserStore_types.h evernote_api/src/Types_types.h \
 evernote_api/src/Limits_types.h evernote_api/src/Errors_types.h
UserStore_types.o: evernote_api/src/UserStore_types.cpp \
 evernote_api/src/UserStore_types.h evernote_api/src/Types_types.h \
 evernote_api/src/Limits_types.h evernote_api/src/Errors_types.h
NotebookTreeStore.o: src/NotebookTreeStore.cpp src/NotebookTreeStore.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h
BetternoteUtils.o: src/BetternoteUtils.cpp src/BetternoteUtils.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h
DatabaseClient.o: src/DatabaseClient.cpp src/DatabaseClient.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h
EvernoteClient.o: src/EvernoteClient.cpp src/EvernoteClient.h \
 src/DatabaseClient.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_constants.h
Main.o: src/Main.cpp src/NoteWindow.h src/EvernoteClient.h \
 src/DatabaseClient.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_constants.h \
 src/NoteView.h src/NotebookSelector.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 src/NotebookTreeStore.h src/NoteSelector.h src/BetternoteUtils.h
NotebookSelector.o: src/NotebookSelector.cpp src/NotebookSelector.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h \
 src/NotebookTreeStore.h
NoteListStore.o: src/NoteListStore.cpp src/NoteListStore.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h
NoteSelector.o: src/NoteSelector.cpp src/NoteSelector.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h \
 src/NoteListStore.h
NoteView.o: src/NoteView.cpp src/NoteView.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h \
 src/DatabaseClient.h src/BetternoteUtils.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h
NoteWindow.o: src/NoteWindow.cpp src/NoteWindow.h src/EvernoteClient.h \
 src/DatabaseClient.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/UserStore_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Types_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Errors_types.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/Limits_constants.h \
 src/NoteView.h src/NotebookSelector.h \
 /mnt/cold_storage/Files/Arch\ Documents/programs/cpp/betternote/evernote_api/src/NoteStore_types.h \
 src/NotebookTreeStore.h src/NoteSelector.h src/BetternoteUtils.h
