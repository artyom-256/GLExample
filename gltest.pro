TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH+=D:\Programming\gl\glew-2.1.0-win32\glew-2.1.0\include
INCLUDEPATH+=D:\Programming\gl\glfw-3.2.1\include
INCLUDEPATH+=D:\Programming\gl\glm-0.9.9.0\install\include

LIBS+=D:\Programming\gl\glfw-3.2.1\build\src\libglfw3.a
LIBS+=C:\Tools\Qt\Tools\mingw530_32\i686-w64-mingw32\lib\libgdi32.a
LIBS+=D:\Programming\gl\glew-2.1.0\build\cmake\build\lib\libglew32.a
LIBS+=C:\Tools\Qt\Tools\mingw530_32\i686-w64-mingw32\lib\libopengl32.a
LIBS+=D:\Programming\gl\glm-0.9.9.0\build\glm\libglm_static.a

SOURCES += main.cpp \
    glwindow.cpp \
    glapp.cpp

HEADERS += \
    glwindow.h \
    glapp.h
