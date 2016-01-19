#pragma once

#include <QWidget>
#include <QVector3D>
#include <QSharedPointer>
#include <QLabel>

#include "camera.h"

// declare but not include to hide scene interface
class Scene;

class Viewer : public QWidget
{
  Q_OBJECT

public:

  Viewer(const QString& filePath);


protected:
  void wheelEvent(QWheelEvent *);
  void keyPressEvent(QKeyEvent *);


private slots:
  void _updatePointSize(int);
  void _updateMeasureInfo(const QVector<QVector3D>& points);


private:

  Scene* _scene;
  QSharedPointer<Camera> _camera;
  QLabel* _lblColorBy;
  QLabel* _lblDistanceInfo;

};
