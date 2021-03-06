/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>
#include <QtOpenGL>
#include <QOpenGLContext>

#include "GLWidget.h"

GLWidget::GLWidget(const QString& texturePath, QWidget *parent)
  : QOpenGLWidget(parent),
    _clearColor(Qt::black),
    _xRot(0),
    _yRot(0),
    _zRot(0),
    _rotIndex(0),
    _program(0),
    _texturePath(texturePath),
#ifdef USE_UBO
    _uboId(0),
    _uboIndex(0),
#else
    _floatStorageTexId(0),
    _intStorageTexId(0),
#endif
    _vboId(0),
    _f(0)
{
}

GLWidget::~GLWidget()
{
  makeCurrent();
  _vao.destroy();
  delete _texture;
  delete _program;
  doneCurrent();
}

QSize GLWidget::minimumSizeHint() const
{
  return QSize(50, 50);
}

QSize GLWidget::sizeHint() const
{
  return QSize(200, 200);
}

void GLWidget::rotateBy(int xAngle, int yAngle, int zAngle)
{
  _xRot += xAngle;
  _yRot += yAngle;
  _zRot += zAngle;
  update();
}

void GLWidget::setClearColor(const QColor &color)
{
  _clearColor = color;
  update();
}

void GLWidget::initializeGL()
{
  initializeOpenGLFunctions();
  _f = QOpenGLContext::currentContext()->extraFunctions();

  //create VAO
  _vao.create();
  _vao.bind();

  makeObject();

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_STENCIL_TEST);
  glEnable(GL_CULL_FACE);
  glEnable(GL_TEXTURE_2D);
  glViewport(0, 0, width(), height());

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1


#ifdef USE_UBO
  QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
  QString vsrc =
      "in vec4 vertex;\n"
      "in vec2 texCoord;\n"
      "out vec2 texc;\n"
      "flat out int materialID;"
      "uniform int rotIndex;\n"
      "\n"
      "int getRotoationIndex(void);\n"
      "mat4 getRotationMatrix(int Index);\n"
      "mat4 getRotationMatrix(void);\n"
      "\n"
      "int getMaterialId(int Index);\n"
      "int getMaterialId(void);\n"
      "\n"
      "struct VertexData {\n"
      "  mat4 rotMatrix;\n"
      "  int material;\n" //the iMX6 needs at least two elements in a struct, otherwise graphical corruption
      "  int dummy1;\n"
      "  int dummy2;\n"
      "  int dummy3;\n"
      "};\n"
      "layout(std140) uniform u_VertexData {\n"
      "  VertexData vData[2];\n"
      "};\n"
      "\n"
      "int getRotationIndex(void)        { return rotIndex; }\n"
      "mat4 getRotationMatrix(int Index) { return vData[Index].rotMatrix; }\n"
      "mat4 getRotationMatrix(void)      { return getRotationMatrix(getRotationIndex()); }\n"
      "\n"
      "int getMaterialId(int Index)      { return vData[Index].material; }\n"
      "int getMaterialId(void)           { return getMaterialId(getRotationIndex()); }\n"
      "\n"
      "void main(void)\n"
      "{\n"
      "    mat4 rotMatrix = getRotationMatrix();\n"
      "    gl_Position = rotMatrix * vertex;\n"
      "    materialID = getMaterialId();\n"
      "    texc = texCoord;\n"
      "}\n";
#else
  QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
  QString vsrc =
      "#ifdef GL_ES\n"
      "precision mediump float;\n"
      "precision mediump sampler2D;\n"
      "precision mediump isampler2D;\n"
      "#endif\n"
      "in vec4 vertex;\n"
      "in vec2 texCoord;\n"
      "flat out int materialID;\n"
      "out vec2 texc;\n"
      "uniform int rotIndex;\n"
      "uniform sampler2D floatSampler;\n"
      "uniform isampler2D intSampler;\n"
      "\n"
      "int getRotoationIndex(void);\n"
      "mat4 getRotationMatrix(int Index);\n"
      "mat4 getRotationMatrix(void);\n"
      "\n"
      "int getMaterialId(int Index);\n"
      "int getMaterialId(void);\n"
      "\n"
      "int getRotationIndex(void)        { return rotIndex; }\n"
      "mat4 getRotationMatrix(void)      { return getRotationMatrix(getRotationIndex()); }\n"
      "mat4 getRotationMatrix(int index) { return mat4(texelFetch(floatSampler, ivec2(0+5*index,0), 0), texelFetch(floatSampler, ivec2(1+5*index,0), 0), texelFetch(floatSampler, ivec2(2+5*index,0), 0), texelFetch(floatSampler, ivec2(3+5*index,0), 0)); }\n"
      "\n"
      "int getMaterialId(int index)      { return int(texelFetch(intSampler, ivec2(4+5*index,0), 0).r); }\n"
      "int getMaterialId(void)           { return getMaterialId(getRotationIndex()); }\n"
      "\n"
      "void main(void)\n"
      "{\n"
      "    mat4 rotMatrix = getRotationMatrix(rotIndex);\n"
      "    gl_Position = rotMatrix * vertex;\n"
      "    materialID = getMaterialId();\n"
      "    texc = texCoord;\n"
      "}\n";
#endif

  QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
  QString fsrc =
      "#ifdef GL_ES\n"
      "precision mediump float;\n"
      "precision mediump sampler2D;\n"
      "precision mediump isampler2D;\n"
      "#endif\n"
      "in vec2 texc;\n"
      "flat in int materialID;"
      "out vec4 fragColor;\n"
      "uniform sampler2D tex;\n"
      "void main(void)\n"
      "{\n"
      "  if (materialID == 1) {\n"
      "    fragColor = mix(texture(tex, texc), vec4(1.0, 0.0, 0.0, 0.5), 0.4);\n"
      "  }\n"
      "  if (materialID == 7) {\n"
      "    fragColor = mix(texture(tex, texc), vec4(0.0, 0.0, 1.0, 0.5), 0.4);\n"
      "  }\n"
      "}\n";


  if (QOpenGLContext::currentContext()->isOpenGLES()) {
    vsrc.prepend(QByteArrayLiteral("#version 300 es\n"));
    fsrc.prepend(QByteArrayLiteral("#version 300 es\n"));
  }
  else {
    vsrc.prepend(QByteArrayLiteral("#version 150\n"));
    fsrc.prepend(QByteArrayLiteral("#version 150\n"));

  }
  vshader->compileSourceCode(vsrc);
  fshader->compileSourceCode(fsrc);

  _program = new QOpenGLShaderProgram(this);
  if (!_program->addShader(vshader)) {
    qDebug("Could not add vertex shader. Error log is:");
    qWarning() << _program->log();
    exit(EXIT_FAILURE);
  }
  if (!_program->addShader(fshader)) {
    qDebug("Could not add fragment shader. Error log is:");
    qWarning() << _program->log();
    exit(EXIT_FAILURE);
  }
  _program->bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
  _program->bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
  if (!_program->link()) {
     qDebug("Could not link shader program. Error log is:");
    qWarning() << _program->log();
    exit(EXIT_FAILURE);
  }

  _program->bind();
  _program->setUniformValue("tex", 0);

 #ifdef USE_UBO
  qDebug("UBO-based shader parameter mechanism");
  //use UBO as a shader parameter mechanism
  glGenBuffers(1, &_uboId);
  _uboIndex = _f->glGetUniformBlockIndex(_program->programId(), "u_VertexData");
  if (_uboIndex == GL_INVALID_INDEX) {
    qWarning("u_VertexData uniform block index could not be determined.");
    exit(EXIT_FAILURE);
  }
  else {
    _f->glGetActiveUniformBlockiv(_program->programId(), _uboIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &_uboSize);
  }

  //create UBO data store
  glBindBuffer(GL_UNIFORM_BUFFER, _uboId);
  glBufferData(GL_UNIFORM_BUFFER, _uboSize, NULL, GL_DYNAMIC_DRAW);
  _f->glBindBufferBase(GL_UNIFORM_BUFFER, _uboIndex, _uboId);
#else
  qDebug("Texture-based shader parameter mechanism");
  //use texture as shader parameter mechanism
  //create texture for float data
  glGenTextures(1, &_floatStorageTexId);
  glBindTexture(GL_TEXTURE_2D, _floatStorageTexId);
  //we're using this texture as storage, so do not want mipmapping
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  //create the storage
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 10, 1, 0, GL_RGBA, GL_FLOAT, NULL);

  //create texture for int data
  glGenTextures(1, &_intStorageTexId);
  glBindTexture(GL_TEXTURE_2D, _intStorageTexId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32I, 10, 1, 0, GL_RGBA_INTEGER, GL_INT, NULL);
#endif

  _vao.release();
}

void GLWidget::paintGL()
{
  glClearColor(_clearColor.red(), _clearColor.green(), _clearColor.blue(), _clearColor.alpha());
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  _vao.bind();

  QMatrix4x4 m;
  m.ortho(-0.5f, +0.5f, +0.5f, -0.5f, 4.0f, 15.0f);
  m.translate(0.0f, 0.0f, -10.0f);
  m.rotate(_xRot / 16.0f, 1.0f, 0.0f, 0.0f);
  m.rotate(_yRot / 16.0f, 0.0f, 1.0f, 0.0f);
  m.rotate(_zRot / 16.0f, 0.0f, 0.0f, 1.0f);

  if (_rotIndex == 0) {
    int material[4] = {1,0,0,0};
    memcpy(_buffer, m.constData(), 16*sizeof(GLfloat));
    memcpy(&_buffer[16], material, 4*sizeof(GLint));
  }
  else {
    QMatrix4x4 n = m;
    n.scale(0.5, 0.5, 0.5);
    int material[4] = {7,0,0,0};
    memcpy(_buffer, n.constData(), 16*sizeof(GLfloat));
    memcpy(&_buffer[16], material, 4*sizeof(GLint));
  }

#ifdef USE_UBO
  //update UBO
  glBindBuffer(GL_UNIFORM_BUFFER, _uboId);
  if (_rotIndex == 0) {
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 16*sizeof(GLfloat)+4*sizeof(GLint), _buffer);
  }
  else {
    glBufferSubData(GL_UNIFORM_BUFFER, 16*sizeof(GLfloat)+4*sizeof(GLint), 16*sizeof(GLfloat)+4*sizeof(GLint), _buffer);
  }
#else
  //update float texture
  glActiveTexture(GL_TEXTURE1);
  _program->setUniformValue("floatSampler", 1);
  glBindTexture(GL_TEXTURE_2D, _floatStorageTexId);
  if (_rotIndex == 0) {
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 5, 1, GL_RGBA, GL_FLOAT, _buffer);
  }
  else {
    glTexSubImage2D(GL_TEXTURE_2D, 0, 5, 0, 5, 1, GL_RGBA, GL_FLOAT, _buffer);
  }
  //update int texture
  glActiveTexture(GL_TEXTURE2);
  _program->setUniformValue("intSampler", 2);
  glBindTexture(GL_TEXTURE_2D, _intStorageTexId);
  if (_rotIndex == 0) {
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 5, 1, GL_RGBA_INTEGER, GL_INT, _buffer);
  }
  else {
    glTexSubImage2D(GL_TEXTURE_2D, 0, 5, 0, 5, 1, GL_RGBA_INTEGER, GL_INT, _buffer);
  }

#endif

  _program->setUniformValue("rotIndex", _rotIndex);
  _program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
  _program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
  _program->setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
  _program->setAttributeBuffer(PROGRAM_TEXCOORD_ATTRIBUTE, GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));

  glActiveTexture(GL_TEXTURE0);
  _texture->bind();
  for (int i = 0; i < 6; ++i) {
    glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
  }
  _vao.release();
}

void GLWidget::toggleRotationIndex()
{
  _rotIndex = (_rotIndex == 0) ? 1 : 0;
  update();
}

void GLWidget::resizeGL(int width, int height)
{
  int side = qMin(width, height);
  glViewport((width - side) / 2, (height - side) / 2, side, side);
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
  _lastPos = event->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
  int dx = event->x() - _lastPos.x();
  int dy = event->y() - _lastPos.y();

  if (event->buttons() & Qt::LeftButton) {
    rotateBy(8 * dy, 8 * dx, 0);
  } else if (event->buttons() & Qt::RightButton) {
    rotateBy(8 * dy, 0, 8 * dx);
  }
  _lastPos = event->pos();
}

void GLWidget::mouseReleaseEvent(QMouseEvent * /* event */)
{
  emit clicked();
}

void GLWidget::makeObject()
{
  static const int coords[6][4][3] = {
    { { +1, -1, -1 }, { -1, -1, -1 }, { -1, +1, -1 }, { +1, +1, -1 } },
    { { +1, +1, -1 }, { -1, +1, -1 }, { -1, +1, +1 }, { +1, +1, +1 } },
    { { +1, -1, +1 }, { +1, -1, -1 }, { +1, +1, -1 }, { +1, +1, +1 } },
    { { -1, -1, -1 }, { -1, -1, +1 }, { -1, +1, +1 }, { -1, +1, -1 } },
    { { +1, -1, +1 }, { -1, -1, +1 }, { -1, -1, -1 }, { +1, -1, -1 } },
    { { -1, -1, +1 }, { +1, -1, +1 }, { +1, +1, +1 }, { -1, +1, +1 } }
  };

  _texture = new QOpenGLTexture(QImage(_texturePath).mirrored());
  _texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
  _texture->setMagnificationFilter(QOpenGLTexture::Linear);

  QVector<GLfloat> vertData;
  for (int i = 0; i < 6; ++i) {
    for (int j = 0; j < 4; ++j) {
      // vertex position
      vertData.append(0.2 * coords[i][j][0]);
      vertData.append(0.2 * coords[i][j][1]);
      vertData.append(0.2 * coords[i][j][2]);
      // texture coordinate
      vertData.append(j == 0 || j == 3);
      vertData.append(j == 0 || j == 1);
    }
  }

  //create vertex buffer
  glGenBuffers(1, &_vboId);
  glBindBuffer(GL_ARRAY_BUFFER, _vboId);
  glBufferData(GL_ARRAY_BUFFER, vertData.count() * sizeof(GLfloat), vertData.constData(), GL_STATIC_DRAW);
}
