# File generated by kdevelop's qmake manager. 
# ------------------------------------------- 
# Subdir relative project main directory: ./Engine/StandAloneEngine/ElasticContactLaw
# Target is a library:  

LIBS += -lSDECLinkPhysics \
        -lElasticContactParameters \
        -lSDECLinkGeometry \
        -lMacroMicroContactGeometry \
        -lBodyMacroParameters \
        -lyade-lib-serialization \
        -lyade-lib-wm3-math \
        -lyade-lib-multimethods \
        -lForce \
        -lMomentum \
        -lSphere \
        -lRigidBodyParameters \
        -rdynamic 
INCLUDEPATH += /usr/local/include/ \
               ../../../DataClass/InteractionPhysics/SDECLinkPhysics \
               ../../../DataClass/InteractionPhysics/ElasticContactParameters \
               ../../../DataClass/InteractionGeometry/SDECLinkGeometry \
               ../../../DataClass/InteractionGeometry/MacroMicroContactGeometry \
               ../../../DataClass/PhysicalParameters/BodyMacroParameters 
QMAKE_LIBDIR = ../../../../../bin \
               ../../../../../bin \
               ../../../../../bin \
               ../../../../../bin \
               ../../../../../bin \
               /usr/local/lib/yade/yade-package-common/ \
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
HEADERS += ElasticContactLaw.hpp 
SOURCES += ElasticContactLaw.cpp 
