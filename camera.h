#pragma once

#include <QObject>
#include <QVector3D>

struct CameraState {
  CameraState(const QVector3D& position_, const QVector3D& rotation_,
              double frontClippingDistance_, double farClippingDistance_)
    : position(position_),
      rotation(rotation_),
      frontClippingDistance(frontClippingDistance_),
      rearClippingDistance(farClippingDistance_)
  {}

  const QVector3D position;
  const QVector3D rotation;
  const double frontClippingDistance;
  const double rearClippingDistance;
};


class Camera : public QObject
{
  Q_OBJECT

public:
  enum RotationSTEP {RK = 1};

  Camera();

  void forward();
  void backward();
  void left();
  void right();
  void up();
  void down();
  void setPosition(const QVector3D& position);

  void rotate(int dx, int dy, int dz);

  void setFrontCPDistance(double distance);
  void setRearCPDistance(double distance);

  CameraState state() const;


signals:
  void changed(const CameraState& newState);
  void xRotationChanged(int angle);
  void yRotationChanged(int angle);
  void zRotationChanged(int angle);


public slots:
  void setXRotation(int angle);
  void setYRotation(int angle);
  void setZRotation(int angle);


private:
  double _frontClippingPlaneDistance;
  double _rearClippingDistance;
  QVector3D _position;
  int _xRotation;
  int _yRotation;
  int _zRotation;

  void _notify() {emit changed(state());}

};

