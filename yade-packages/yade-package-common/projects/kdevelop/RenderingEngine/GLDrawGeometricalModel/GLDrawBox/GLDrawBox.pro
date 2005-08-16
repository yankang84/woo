# File generated by kdevelop's qmake manager. 
# ------------------------------------------- 
# Subdir relative project main directory: ./RenderingEngine/GLDrawGeometricalModel/GLDrawBox
# Target is a library:  

LIBS += -lBox \
        -lyade-lib-opengl \
        -rdynamic 
INCLUDEPATH += /usr/local/include/ \
               ../../../DataClass/GeometricalModel/Box \
               ../../../RenderingEngine/OpenGLRenderingEngine 
QMAKE_LIBDIR = ../../../../../bin \
               /usr/local/lib/yade/yade-libs/ 
QMAKE_CXXFLAGS_RELEASE += -lpthread \
                          -pthread 
QMAKE_CXXFLAGS_DEBUG += -lpthread \
                        -pthread 
DESTDIR = ../../../../../bin 
CONFIG += debug \
          warn_on \
          dll 
TEMPLATE = lib 
HEADERS += GLDrawBox.hpp 
SOURCES += GLDrawBox.cpp 
