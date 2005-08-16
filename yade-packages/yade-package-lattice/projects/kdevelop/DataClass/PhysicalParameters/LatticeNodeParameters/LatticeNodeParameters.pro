# File generated by kdevelop's qmake manager. 
# ------------------------------------------- 
# Subdir relative project main directory: ./DataClass/PhysicalParameters/LatticeNodeParameters
# Target is a library:  

LIBS += -lRigidBodyParameters \
        -lyade-lib-wm3-math \
        -lyade-lib-multimethods \
        -lyade-lib-serialization \
        -rdynamic 
QMAKE_LIBDIR = /usr/local/lib/yade/yade-package-common/ \
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
HEADERS += LatticeNodeParameters.hpp 
SOURCES += LatticeNodeParameters.cpp 
INCLUDEPATH += /usr/local/include

