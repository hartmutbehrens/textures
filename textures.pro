HEADERS = GLWidget.h \
          Window.h
SOURCES = GLWidget.cpp \
          Window.cpp \
          main.cpp

RESOURCES     = textures.qrc
QT           += opengl widgets

# DEFINES += USE_UBO

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/textures
INSTALLS += target
