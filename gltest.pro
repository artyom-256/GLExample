TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH+=$(GLEW_INC)
INCLUDEPATH+=$(GLFW_INC)
INCLUDEPATH+=$(GLM_INC)

LIBS+=$(GLFW_LIB)\libglfw3.a
LIBS+=$(STD_LIB)\libgdi32.a
LIBS+=$(GLEW_LIB)\libglew32.a
LIBS+=$(STD_LIB)\libopengl32.a

SOURCES += main.cpp \
    bitmap_image.cpp \
    object.cpp

HEADERS += \
    bitmap_image.h \
    object.h

DISTFILES += \
    main.fs \
    main.vs
