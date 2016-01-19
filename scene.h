#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QVector3D>
#include <QSharedPointer>

#include <camera.h>
#include <vector>


class Scene : public QOpenGLWidget, protected QOpenGLFunctions
{
  Q_OBJECT

public:
  enum colorAxisMode {COLOR_BY_ROW, COLOR_BY_Z};

  Scene(const QString& plyFilePath, QWidget* parent = 0);
  ~Scene();


public slots:
  void setPointSize(size_t size);
  void setColorAxisMode(colorAxisMode value);
  void attachCamera(QSharedPointer<Camera> camera);
  void setPickpointEnabled(bool enabled);
  void clearPickedpoints();


signals:
  void pickpointsChanged(const QVector<QVector3D> points);


protected:
  void initializeGL() Q_DECL_OVERRIDE;
  void paintGL() Q_DECL_OVERRIDE;
  void resizeGL(int width, int height) Q_DECL_OVERRIDE;

  void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
  void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;


private slots:
  void _onCameraChanged(const CameraState& state);

private:
  void _loadPLY(const QString& plyFilePath);
  void _cleanup();
  void _drawFrameAxis();
  QVector3D _unproject(int x, int y) const;
  QVector3D _pickPointFrom2D(const QPoint& pos) const;
  void _drawMarkerBox(const QVector3D& point, const QColor& color);

  float _pointSize;
  colorAxisMode _colorMode;
  std::vector<std::pair<QVector3D, QColor> > _axesLines;

  QPoint _prevMousePosition;
  QOpenGLVertexArrayObject _vao;
  QOpenGLBuffer _vertexBuffer;
  QScopedPointer<QOpenGLShaderProgram> _shaders;

  QMatrix4x4 _projectionMatrix;
  QMatrix4x4 _cameraMatrix;
  QMatrix4x4 _worldMatrix;

  QVector<float> _pointsData;
  size_t _pointsCount;
  QVector3D _pointsBoundMin;
  QVector3D _pointsBoundMax;
  QVector3D _ray;

  QSharedPointer<Camera> _currentCamera;

  bool _pickpointEnabled;
  QVector<QVector3D> _pickedPoints;
  QVector3D _highlitedPoint;
};
