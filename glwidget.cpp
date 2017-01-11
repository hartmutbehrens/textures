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

#include "glwidget.h"

GLWidget::GLWidget(const QString& texturePath, QWidget *parent, QGLWidget *shareWidget)
  : QGLWidget(parent, shareWidget), _texturePath(texturePath)
{
  clearColor = Qt::black;
  xRot = 0;
  yRot = 0;
  zRot = 0;
  program = 0;
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
  updateGL();
}

void GLWidget::setClearColor(const QColor &color)
{
  clearColor = color;
  updateGL();
}

void GLWidget::destructinatePixmaps()
{
  // QSharedPointer will dtor, and payload will also be deleted
  this->pixmaps.clear();
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
  const char *vsrc =
      "#version 150\n"
      "in vec4 vertex;\n"
      "in vec2 texCoord;\n"
      "out vec2 texc;\n"
      "\n"
      "uniform mediump mat4 matrix;\n"
      "void main(void)\n"
      "{\n"
      "    gl_Position = matrix * vertex;\n"
      "    texc = texCoord;\n"
      "}\n";
  vshader->compileSourceCode(vsrc);

  QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
  const char *fsrc =
      "#version 150\n"
      "uniform sampler2D tex;\n"
      "in vec2 texc;\n"
      "out vec4 fragColor;\n"
      "void main(void)\n"
      "{\n"
      "    fragColor = texture(tex, texc);\n"
      "    //fragColor = vec4(0.0, 0.0, 0.0, 0.0);\n"
      "}\n";
  fshader->compileSourceCode(fsrc);

  program = new QOpenGLShaderProgram(this);
  program->addShader(vshader);
  program->addShader(fshader);
  program->bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
  program->bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
  program->link();

  program->bind();
  program->setUniformValue("tex", 0);

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
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  QMatrix4x4 m;
  m.ortho(-0.5f, +0.5f, +0.5f, -0.5f, 4.0f, 15.0f);
  m.translate(0.0f, 0.0f, -10.0f);
  m.rotate(xRot / 16.0f, 1.0f, 0.0f, 0.0f);
  m.rotate(yRot / 16.0f, 0.0f, 1.0f, 0.0f);
  m.rotate(zRot / 16.0f, 0.0f, 0.0f, 1.0f);

  program->setUniformValue("matrix", m);
  program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
  program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
  program->setAttributeArray
      (PROGRAM_VERTEX_ATTRIBUTE, vertices.constData());
  program->setAttributeArray
      (PROGRAM_TEXCOORD_ATTRIBUTE, texCoords.constData());

  glBindTexture(GL_TEXTURE_2D, textures[0]);
  glBindFragDataLocation(program->programId(), 0, "fragColor");
  for (int i = 0; i < 6; ++i) {
    //glBindTexture(GL_TEXTURE_2D, textures[i]);
    glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
  }
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

  // only load into memory once for this experiment
  // move into loop to load N-times
  this->pixmaps.push_back(QSharedPointer<QPixmap>(new QPixmap(_texturePath)));
  // for this experiment, only bind the texture once (actually even if we bind multiple times, Qt sees the same pixmap, so returns same handle)
  // default bind options are: LinearFilteringBindOption | InvertedYBindOption | MipmapBindOption
  // for the experiment we are interested in memory use, so we disable the mirroring and mipmapping, both of which result in extra mem use CPU/GPU
  textures[0] = bindTexture(*this->pixmaps[0], GL_TEXTURE_2D, GL_RGBA, QGLContext::LinearFilteringBindOption);
  qDebug() << "TEXTURE HANDLE for this glwidget " << textures[0] << endl;

  // destroy the pixmaps immediately
  // you can comment this out if you would prefer to destroy manually by clicking three times
  // this->pixmaps.clear();
  // COMMENTED OUT: destroy manually by clicking three times!!!

  for (int j=0; j < 6; ++j) {
    //textures[j] = bindTexture(*this->pixmaps[0], GL_TEXTURE_2D);
    //        textures[j] = bindTexture
    //            (QPixmap(QString(":/images/side%1.png").arg(j + 1)), GL_TEXTURE_2D);
  }

  float pixmapBytes = 0;
  foreach (const QSharedPointer<QPixmap>& pm, this->pixmaps){
    pixmapBytes += pm->width() * pm->height() * pm->depth() / 8.0;
  }

  qDebug() << "GLWidget pixmap Mbytes " << pixmapBytes / 1024 / 1024;

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
