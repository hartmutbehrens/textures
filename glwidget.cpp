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

#include "glwidget.h"

GLWidget::GLWidget(const QString& texturePath, QWidget *parent)
  : QOpenGLWidget(parent),
    clearColor(Qt::black),
    xRot(0),
    yRot(0),
    zRot(0),
    rotIndex(0),
    program(0),
    _texturePath(texturePath)
{
}

GLWidget::~GLWidget()
{
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
  xRot += xAngle;
  yRot += yAngle;
  zRot += zAngle;
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

  makeObject();

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
#ifdef GL_TEXTURE_2D
  glEnable(GL_TEXTURE_2D);
#endif

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1

  QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
  QString vsrc =
      "in vec4 vertex;\n"
      "in vec2 texCoord;\n"
      "out vec2 texc;\n"
      "uniform int rotIndex;\n"
      "struct VertexData {\n"
      "  mediump mat4 rotMatrix;\n"
      "};\n"
      "layout(std140) uniform u_VertexData {\n"
      "  VertexData vData[2];\n"
      "};\n"
      "\n"
      "mediump mat4 getRotationMatrix(void)       { return vData[rotIndex].rotMatrix; }\n"
      "\n"
      "void main(void)\n"
      "{\n"
      "    gl_Position = getRotationMatrix() * vertex;\n"
      "    texc = texCoord;\n"
      "}\n";

  QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
  QString fsrc =
      "#ifdef GL_ES\n"
      "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
      "precision highp float;\n"
      "#else\n"
      "precision mediump float;\n"
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

  program = new QOpenGLShaderProgram(this);
  program->addShader(vshader);
  program->addShader(fshader);
  program->bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
  program->bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
  program->link();

  program->bind();
  program->setUniformValue("tex", 0);

//  GLint numUniformBlocks = 0;
//  GLsizei bufSize = 1024;
//  GLsizei length;
//  GLchar name[1024];
//  glGetProgramiv(program->programId(), GL_ACTIVE_UNIFORM_BLOCKS, &numUniformBlocks);
//  for (int i = 0; i < numUniformBlocks; i++) {
//    glGetActiveUniformBlockName(program->programId(), i, bufSize, &length, name);
//    GLint location = glGetUniformBlockIndex(program->programId(), name);
//    qDebug( "%p Uniform block name:'%s' location:'%d'", this, name, location);
//  }

  //create buffers
  vertexBuffer.create();
  vertexBuffer.bind();
  vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
  vertexBuffer.write(0, vertices.constData(), vertices.size() * sizeof(QVector3D));

  texCoordBuffer.create();
  texCoordBuffer.bind();
  texCoordBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
  texCoordBuffer.write(0, texCoords.constData(), texCoords.size() * sizeof(QVector3D));

  //create VAO
  vao.create();
  vao.bind();

  program->setAttributeBuffer(program->attributeLocation("vertex"), GL_FLOAT, 0, 3, sizeof(vertices));
  program->setAttributeBuffer(program->attributeLocation("texCoord"), GL_FLOAT, 0, 2, sizeof(texCoords));

  program->enableAttributeArray(program->attributeLocation("vertex"));
  program->enableAttributeArray(program->attributeLocation("texCoord"));

}

void GLWidget::paintGL()
{
  glClearColor(clearColor.red(), clearColor.green(), clearColor.blue(), clearColor.alpha());
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  QMatrix4x4 m;
  m.ortho(-0.5f, +0.5f, +0.5f, -0.5f, 4.0f, 15.0f);
  m.translate(0.0f, 0.0f, -10.0f);
  m.rotate(xRot / 16.0f, 1.0f, 0.0f, 0.0f);
  m.rotate(yRot / 16.0f, 0.0f, 1.0f, 0.0f);
  m.rotate(zRot / 16.0f, 0.0f, 0.0f, 1.0f);

  QMatrix4x4 n;
  n.ortho(-0.5f, +0.5f, +0.5f, -0.5f, 4.0f, 15.0f);
  n.translate(0.0f, 0.0f, -10.0f);
  n.rotate(-xRot / 16.0f, 1.0f, 0.0f, 0.0f);
  n.rotate(-yRot / 16.0f, 0.0f, 1.0f, 0.0f);
  n.rotate(-zRot / 16.0f, 0.0f, 0.0f, 1.0f);

  float* buffer = NULL;
  GLint uboSize;
  GLuint ubo;
  GLuint uboIndex = glGetUniformBlockIndex(program->programId(), "u_VertexData");
  if (uboIndex != GL_INVALID_INDEX) {
    glGetActiveUniformBlockiv(program->programId(), uboIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &uboSize);
    buffer = static_cast<float*>(malloc(uboSize));
    memcpy(buffer, m.constData(), 16*sizeof(float));
    memcpy(buffer + 16, n.constData(), 16*sizeof(float));

    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, uboSize, buffer, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, uboIndex, ubo);
  }
  program->setUniformValue("rotIndex", rotIndex);
  program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
  program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
  program->setAttributeArray
      (PROGRAM_VERTEX_ATTRIBUTE, vertices.constData());
  program->setAttributeArray
      (PROGRAM_TEXCOORD_ATTRIBUTE, texCoords.constData());

  texture->bind();
  for (int i = 0; i < 6; ++i) {
    glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
  }
  free(buffer);
}

void GLWidget::toggleRotationIndex()
{
  rotIndex = (rotIndex == 0) ? 1 : 0;
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

  texture = new QOpenGLTexture(QImage(_texturePath).mirrored());
  texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
  texture->setMagnificationFilter(QOpenGLTexture::Linear);

  for (int i = 0; i < 6; ++i) {
    for (int j = 0; j < 4; ++j) {
      texCoords.append
          (QVector2D(j == 0 || j == 3, j == 0 || j == 1));
      vertices.append
          (QVector3D(0.2 * coords[i][j][0], 0.2 * coords[i][j][1],
          0.2 * coords[i][j][2]));
    }
  }
}
