#include "scene.h"

#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <QScopedPointer>

#include <cmath>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <limits>

const size_t POINT_STRIDE = 4; // x, y, z, index

Scene::Scene(const QString& plyFilePath, QWidget* parent)
  : QOpenGLWidget(parent),
    _pointSize(1),
    _colorMode(COLOR_BY_Z)
{
  _pickpointEnabled = false;
  _loadPLY(plyFilePath);
  setMouseTracking(true);

  // make trivial axes cross
  _axesLines.push_back(std::make_pair(QVector3D(0.0, 0.0, 0.0), QColor(1.0, 0.0, 0.0)));
  _axesLines.push_back(std::make_pair(QVector3D(1.0, 0.0, 0.0), QColor(1.0, 0.0, 0.0)));
  _axesLines.push_back(std::make_pair(QVector3D(0.0, 0.0, 0.0), QColor(0.0, 1.0, 0.0)));
  _axesLines.push_back(std::make_pair(QVector3D(0.0, 1.0, 0.0), QColor(0.0, 1.0, 0.0)));
  _axesLines.push_back(std::make_pair(QVector3D(0.0, 0.0, 0.0), QColor(0.0, 0.0, 1.0)));
  _axesLines.push_back(std::make_pair(QVector3D(0.0, 0.0, 1.0), QColor(0.0, 0.0, 1.0)));

}


void Scene::_loadPLY(const QString& plyFilePath) {

  // open stream
  std::fstream is;
  is.open(plyFilePath.toStdString().c_str(), std::fstream::in);

  // ensure format with magic header
  std::string line;
  std::getline(is, line);
  if (line != "ply") {
    throw std::runtime_error("not a ply file");
  }

  // parse header looking only for 'element vertex' section size
  _pointsCount = 0;
  while (is.good()) {
    std::getline(is, line);
    if (line == "end_header") {
      break;
    } else {
      std::stringstream ss(line);
      std::string tag1, tag2, tag3;
      ss >> tag1 >> tag2 >> tag3;
      if (tag1 == "element" && tag2 == "vertex") {
        _pointsCount = std::atof(tag3.c_str());
      }
    }
  }

  // read and parse 'element vertex' section
  if (_pointsCount > 0) {
    _pointsData.resize(_pointsCount * POINT_STRIDE);

    std::stringstream ss;
    std::string line;
    float *p = _pointsData.data();
    for (size_t i = 0; is.good() && i < _pointsCount; ++i) {
      std::getline(is, line);
      ss.str(line);
      float x, y, z;
      ss >> x >> y >> z;

      *p++ = x;
      *p++ = y;
      *p++ = z;
      *p++ = i;

      // update bounds
      _pointsBoundMax[0] = std::max(x, _pointsBoundMax[0]);
      _pointsBoundMax[1] = std::max(y, _pointsBoundMax[1]);
      _pointsBoundMax[2] = std::max(z, _pointsBoundMax[2]);
      _pointsBoundMin[0] = std::min(x, _pointsBoundMin[0]);
      _pointsBoundMin[1] = std::min(y, _pointsBoundMin[1]);
      _pointsBoundMin[2] = std::min(z, _pointsBoundMin[2]);
    }

    // check if we've got exact number of points mentioned in header
    if (p - _pointsData.data() < _pointsData.size()) {
      throw std::runtime_error("broken ply file");
    }
  }
}


Scene::~Scene()
{
  _cleanup();
}


void Scene::_cleanup()
{
  makeCurrent();
  _vertexBuffer.destroy();
  _shaders.reset();
  doneCurrent();
}


void Scene::initializeGL()
{
  connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &Scene::_cleanup);

  initializeOpenGLFunctions();
  glClearColor(0, 0, 0, 1.0);

  // the world is still for now
  _worldMatrix.setToIdentity();

  //
  // create shaders and map attributes
  //
  _shaders.reset(new QOpenGLShaderProgram());
  auto vsLoaded = _shaders->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vertex_shader.glsl");
  auto fsLoaded = _shaders->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fragment_shader.glsl");
  assert(vsLoaded && fsLoaded);
  // vector attributes
  _shaders->bindAttributeLocation("vertex", 0);
  _shaders->bindAttributeLocation("pointRowIndex", 1);
  // constants
  _shaders->bind();
  _shaders->setUniformValue("lightPos", QVector3D(0, 0, 50));
  _shaders->setUniformValue("pointsCount", static_cast<GLfloat>(_pointsCount));
  _shaders->link();
  _shaders->release();

  // create array container and load points into buffer
  _vao.create();
  QOpenGLVertexArrayObject::Binder vaoBinder(&_vao);
  _vertexBuffer.create();
  _vertexBuffer.bind();
  _vertexBuffer.allocate(_pointsData.constData(), _pointsData.size() * sizeof(GLfloat));
  QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
  f->glEnableVertexAttribArray(0);
  f->glEnableVertexAttribArray(1);
  f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat) + sizeof(GLfloat), 0);
  f->glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat) + sizeof(GLfloat), reinterpret_cast<void *>(3*sizeof(GLfloat)));
  _vertexBuffer.release();

}


void Scene::paintGL()
{
  // ensure GL flags
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_VERTEX_PROGRAM_POINT_SIZE); //required for gl_PointSize

  //
  // set camera
  //
  const CameraState camera = _currentCamera->state();
  // position and angles
  _cameraMatrix.setToIdentity();
  _cameraMatrix.translate(camera.position.x(), camera.position.y(), camera.position.z());
  _cameraMatrix.rotate(camera.rotation.x(), 1, 0, 0);
  _cameraMatrix.rotate(camera.rotation.y(), 0, 1, 0);
  _cameraMatrix.rotate(camera.rotation.z(), 0, 0, 1);

  // set clipping planes
  glEnable(GL_CLIP_PLANE1);
  glEnable(GL_CLIP_PLANE2);
  const double rearClippingPlane[] = {0., 0., -1., camera.rearClippingDistance};
  glClipPlane(GL_CLIP_PLANE1 , rearClippingPlane);
  const double frontClippingPlane[] = {0., 0., 1., camera.frontClippingDistance};
  glClipPlane(GL_CLIP_PLANE2 , frontClippingPlane);

  //
  // draw points cloud
  //
  QOpenGLVertexArrayObject::Binder vaoBinder(&_vao);
  const auto viewMatrix = _projectionMatrix * _cameraMatrix * _worldMatrix;
  _shaders->bind();
  _shaders->setUniformValue("pointsCount", static_cast<GLfloat>(_pointsCount));
  _shaders->setUniformValue("viewMatrix", viewMatrix);
  _shaders->setUniformValue("pointSize", _pointSize);
  _shaders->setUniformValue("colorAxisMode", static_cast<GLfloat>(_colorMode));
  _shaders->setUniformValue("pointsBoundMin", _pointsBoundMin);
  _shaders->setUniformValue("pointsBoundMax", _pointsBoundMax);
  glDrawArrays(GL_POINTS, 0, _pointsData.size());
  _shaders->release();

  //
  // draw picked points and line between
  //
  glBegin(GL_LINES);
  glColor3f(1., 1., 0.);
  {
    QMatrix4x4 mvMatrix = _cameraMatrix * _worldMatrix;
    for (auto vertex : _pickedPoints) {
      const auto translated = _projectionMatrix * mvMatrix * vertex;
      glVertex3f(translated.x(), translated.y(), translated.z());
    }
  }
  glEnd();
  for (auto vertex : _pickedPoints) {
    _drawMarkerBox(vertex, QColor(1., 1., 0.));
  }

  //
  // draw mouse-highlited point
  //
  if (_highlitedPoint != QVector3D()) {
    _drawMarkerBox(_highlitedPoint, QColor(0., 1., 1.));
  }

  _drawFrameAxis();

}


void Scene::_drawMarkerBox(const QVector3D& point, const QColor& color) {
  glBegin(GL_LINE_LOOP);
  glColor3f(color.red(), color.green(), color.blue());
  {
    const float dx = 0.01;
    const auto aspect = (float)width() / height();
    const auto dy = dx*aspect;
    const auto p = _projectionMatrix * _cameraMatrix * _worldMatrix * point;
    glVertex3f(p.x()-dx, p.y()-dy, p.z());
    glVertex3f(p.x()+dx, p.y()-dy, p.z());
    glVertex3f(p.x()+dx, p.y()+dy, p.z());
    glVertex3f(p.x()-dx, p.y()+dy, p.z());
  }
  glEnd();
}


void Scene::_drawFrameAxis() {
  glBegin(GL_LINES);
  QMatrix4x4 mvMatrix = _cameraMatrix * _worldMatrix;
  mvMatrix.scale(0.05); // make it small
  for (auto vertex : _axesLines) {
    const auto translated = _projectionMatrix * mvMatrix * vertex.first;
    glColor3f(vertex.second.red(), vertex.second.green(), vertex.second.blue());
    glVertex3f(translated.x(), translated.y(), translated.z());
  }
  glEnd();
}


void Scene::resizeGL(int w, int h)
{
  _projectionMatrix.setToIdentity();
  _projectionMatrix.perspective(70.0f, GLfloat(w) / h, 0.01f, 100.0f);
}


QVector3D Scene::_pickPointFrom2D(const QPoint& pos) const {

  // do slow linear search through small sample
  // must have is BSP/quadtree for O(logN) searching on large samples
  float maxDistance = 1e-1;
  auto ray = _unproject(pos.x(), pos.y());
  QVector3D closest;
  for (size_t i = 0; i < _pointsCount; i++) {
    const GLfloat *p = &_pointsData[i*4];
    QVector3D point(p[0], p[1], p[2]);

    float distance = (point - ray).length();
    if (distance < maxDistance) {
      closest = point;
      maxDistance = distance;
    }
  }
  return closest;
}


void Scene::mousePressEvent(QMouseEvent *event)
{
  _prevMousePosition = event->pos();

  if (event->button() == Qt::LeftButton && _pickpointEnabled)
  {
    const QVector3D closest = _pickPointFrom2D(event->pos());
    if (closest != QVector3D()) {
      if (_pickedPoints.size() == 2) {
        // clear previous pair
        _pickedPoints.clear();
      }

      _pickedPoints << closest;
      emit pickpointsChanged(_pickedPoints);
      update();
    }
  }
}


void Scene::mouseMoveEvent(QMouseEvent *event)
{  
  const int dx = event->x() - _prevMousePosition.x();
  const int dy = event->y() - _prevMousePosition.y();
  const bool panningMode = (event->modifiers() & Qt::ShiftModifier);
  _prevMousePosition = event->pos();

  if (event->buttons() & Qt::LeftButton) {

    if (panningMode) {
      if (dx > 0) {
        _currentCamera->right();
      }
      if (dx < 0) {
        _currentCamera->left();
      }
      if (dy > 0) {
        _currentCamera->down();
      }
      if (dy < 0) {
        _currentCamera->up();
      }
    } else {
      _currentCamera->rotate(dy, dx, 0);
    }
  }

  if (_pickpointEnabled) {
    _highlitedPoint = _pickPointFrom2D(event->pos());
    update();
  }
}




void Scene::setPointSize(size_t size) {
  assert(size > 0);
  _pointSize = size;
  update();
}


void Scene::setColorAxisMode(colorAxisMode value) {
  _colorMode = value;
  update();
}


void Scene::attachCamera(QSharedPointer<Camera> camera) {
  if (_currentCamera) {
    disconnect(_currentCamera.data(), &Camera::changed, this, &Scene::_onCameraChanged);
  }
  _currentCamera = camera;
  connect(camera.data(), &Camera::changed, this, &Scene::_onCameraChanged);
}


void Scene::_onCameraChanged(const CameraState&) {
  update();
}


void Scene::setPickpointEnabled(bool enabled) {
  _pickpointEnabled = enabled;
  if (!enabled) {
    _highlitedPoint = QVector3D();
    update();
  }
}


void Scene::clearPickedpoints() {
  _pickedPoints.clear();
  emit pickpointsChanged(_pickedPoints);
  update();
}


QVector3D Scene::_unproject(int x, int y) const {
  // with Qt5.5 we can make use of new QVector3D::unproject()

  const QMatrix4x4 mvMatrix = _projectionMatrix * _cameraMatrix * _worldMatrix;
  const QMatrix4x4 inverted = mvMatrix.inverted();

  // normalized device coordinates
  double ndcX = 2*(double)x / width() - 1;
  double ndcY = 2*(double)(height() - y) / height() - 1;
  QVector4D nearPoint4 = inverted * QVector4D(ndcX, ndcY, 1, 1);
  QVector4D farPoint4 = inverted * QVector4D(ndcX, ndcY, -1, 1);
  if (nearPoint4.w() == 0.0) {
    return QVector3D();
  }

  double w = 1.0/nearPoint4.w();
  QVector3D nearPoint = QVector3D(nearPoint4);
  nearPoint *= w;

  w = 1.0/farPoint4.w();
  QVector3D farPoint = QVector3D(farPoint4);
  farPoint *= w;

  QVector3D direction = farPoint - nearPoint;
  if (direction.z() == 0.0) {
    return QVector3D();
  }

  double t = -nearPoint.z() / direction.z();
  return nearPoint + direction * t;
}
