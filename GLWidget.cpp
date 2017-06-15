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
    clearColor(Qt::black),
    _xRot(0),
    _yRot(0),
    _zRot(0),
    _rotIndex(0),
    _program(0),
    _texturePath(texturePath),
    _uboId(0),
    _uboIndex(0),
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
  clearColor = color;
  update();
}

void GLWidget::initializeGL()
{
  initializeOpenGLFunctions();
  _f = QOpenGLContext::currentContext()->extraFunctions();

  makeObject();

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_STENCIL_TEST);
  glEnable(GL_CULL_FACE);
  glEnable(GL_TEXTURE_2D);
  glViewport(0, 0, width(), height());

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1

  QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
  QString vsrc =
      "in vec4 vertex;\n"
      "in vec2 texCoord;\n"
      "out vec2 texc;\n"
      "uniform int rotIndex;\n"
      "\n"
      "int getRotoationIndex(void);\n"
      "mat4 getRotationMatrix(int Index);\n"
      "mat4 getRotationMatrix(void);\n"
      "\n"
      "struct VertexData {\n"
      "  mat4 rotMatrix;\n"
      "  vec4 dummy;\n" //the iMX6 needs at least two elements in a struct, otherwise graphical corruption
      "};\n"
      "layout(std140) uniform u_VertexData {\n"
      "  VertexData vData[2];\n"
      "};\n"
      "\n"
      "int getRotationIndex(void)         { return rotIndex; }\n"
      "mat4 getRotationMatrix(int Index)  { return vData[Index].rotMatrix; }\n"
      "mat4 getRotationMatrix(void)       { return getRotationMatrix(getRotationIndex()); }\n"
      "\n"
      "void main(void)\n"
      "{\n"
      "    mat4 rotMatrix = getRotationMatrix(rotIndex);\n"
      "    gl_Position = rotMatrix * vertex;\n"
      "    texc = texCoord;\n"
      "}\n";

  QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
  QString fsrc =
      "#ifdef GL_ES\n"
      "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
      "precision highp float;\n"
      "precision highp sampler2D;\n"
      "#else\n"
      "precision mediump float;\n"
      "precision mediump sampler2D;\n"
      "#endif\n"
      "#endif\n"
      "in vec2 texc;\n"
      "out vec4 fragColor;\n"
      "uniform sampler2D tex;\n"
      "void main(void)\n"
      "{\n"
      "    fragColor = texture(tex, texc);\n"
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
    qWarning() << _program->log();
    exit(EXIT_FAILURE);
  }
  if (!_program->addShader(fshader)) {
    qWarning() << _program->log();
    exit(EXIT_FAILURE);
  }
  _program->bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
  _program->bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
  if (!_program->link()) {
    qWarning() << _program->log();
    exit(EXIT_FAILURE);
  }

  _program->bind();
  _program->setUniformValue("tex", 0);

  //create VAO
  _vao.create();
  _vao.bind();

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
  // reset to default
  _vao.release();
}

void GLWidget::paintGL()
{
  glClearColor(clearColor.red(), clearColor.green(), clearColor.blue(), clearColor.alpha());
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  _vao.bind();

  QMatrix4x4 m;
  m.ortho(-0.5f, +0.5f, +0.5f, -0.5f, 4.0f, 15.0f);
  m.translate(0.0f, 0.0f, -10.0f);
  m.rotate(_xRot / 16.0f, 1.0f, 0.0f, 0.0f);
  m.rotate(_yRot / 16.0f, 0.0f, 1.0f, 0.0f);
  m.rotate(_zRot / 16.0f, 0.0f, 0.0f, 1.0f);

  //update UBO
  glBindBuffer(GL_UNIFORM_BUFFER, _uboId);
  if (_rotIndex == 0) {
    memcpy(_buffer, m.constData(), 16*sizeof(float));
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 16*sizeof(float), _buffer);
  }
  else {
    QMatrix4x4 n = m;
    n.scale(0.5, 0.5, 0.5);
    memcpy(_buffer, n.constData(), 16*sizeof(float));
    glBufferSubData(GL_UNIFORM_BUFFER, (16+4)*sizeof(float), 16*sizeof(float), _buffer);
  }

  _program->setUniformValue("rotIndex", _rotIndex);
  _program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
  _program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
  _program->setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
  _program->setAttributeBuffer(PROGRAM_TEXCOORD_ATTRIBUTE, GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));

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
  lastPos = event->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
  int dx = event->x() - lastPos.x();
  int dy = event->y() - lastPos.y();

  if (event->buttons() & Qt::LeftButton) {
    rotateBy(8 * dy, 8 * dx, 0);
  } else if (event->buttons() & Qt::RightButton) {
    rotateBy(8 * dy, 0, 8 * dx);
  }
  lastPos = event->pos();
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

  //create buffers
  glGenBuffers(1, &_vboId);
  glBindBuffer(GL_ARRAY_BUFFER, _vboId);
  glBufferData(GL_ARRAY_BUFFER, vertData.count() * sizeof(GLfloat), vertData.constData(), GL_STATIC_DRAW);
}
