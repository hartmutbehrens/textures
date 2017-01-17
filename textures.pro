HEADERS = GLWidget.h \
          Window.h
SOURCES = GLWidget.cpp \
          Window.cpp \
          main.cpp

RESOURCES     = textures.qrc
QT           += opengl widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/textures
INSTALLS += target
