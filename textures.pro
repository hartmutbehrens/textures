HEADERS = GLWidget.h \
          Window.h
SOURCES = GLWidget.cpp \
          Window.cpp \
          main.cpp

RESOURCES     = textures.qrc
QT           += opengl widgets

# Uncomment this to use UBO-based shader parameter storage instead of texture-based storage
# DEFINES += USE_UBO

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/textures
INSTALLS += target
