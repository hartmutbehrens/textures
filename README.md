Small OpenGL app to experiment with UBO's and textures as shader parameter storage mechanisms.
Adapted from http://doc.qt.io/qt-5/qtopengl-textures-example.html 

The example requires Qt5.6

To compile and run the app with a texture-based shader parameter storage mechanism:

~~~~
qmake textures.pro
make
./textures
~~~~

To compile and run the app with a UBO-based shader parameter storage mechanism:

~~~~
qmake DEFINES+=USE_UBO textures.pro
make clean
make
./textures
~~~~
