#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>
#include <QDesktopWidget>
#include <QApplication>
#include <QMessageBox>
#include <QComboBox>
#include <QTimer>
#include <QGroupBox>
#include <QCheckBox>
#include <QSlider>

#include "camera.h"
#include "scene.h"
#include "viewer.h"

#include <cassert>



QSlider* createAnglecontrolSlider()
{
  QSlider* slider = new QSlider(Qt::Horizontal);
  slider->setRange(0, 360 * Camera::RK);
  slider->setSingleStep(Camera::RK);
  slider->setPageStep(10 * Camera::RK);
  return slider;
}


Viewer::Viewer(const QString& filePath)
{
  // accept keyboard input
  setFocusPolicy(Qt::StrongFocus);
  setFocus();

  //
  // make and connect scene widget
  //
  _scene = new Scene(filePath);
  connect(_scene, &Scene::pickpointsChanged, this, &Viewer::_updateMeasureInfo);

  //
  // make shared camera
  //
  _camera = QSharedPointer<Camera>(new Camera());
  _scene->attachCamera(_camera);

  //
  // make camera controls
  //
  auto xSlider = createAnglecontrolSlider();
  auto ySlider = createAnglecontrolSlider();
  auto zSlider = createAnglecontrolSlider();
  connect(xSlider, SIGNAL(valueChanged(int)), _camera.data(), SLOT(setXRotation(int)));
  connect(_camera.data(), SIGNAL(xRotationChanged(int)), xSlider, SLOT(setValue(int)));
  connect(ySlider, SIGNAL(valueChanged(int)), _camera.data(), SLOT(setYRotation(int)));
  connect(_camera.data(), SIGNAL(yRotationChanged(int)), ySlider, SLOT(setValue(int)));
  connect(zSlider, SIGNAL(valueChanged(int)), _camera.data(), SLOT(setZRotation(int)));
  connect(_camera.data(), SIGNAL(zRotationChanged(int)), zSlider, SLOT(setValue(int)));

  //
  // make 'point size' contoller
  //
  auto pointSizeSlider = new QSlider(Qt::Horizontal);
  pointSizeSlider->setRange(1, 20);
  pointSizeSlider->setTickPosition(QSlider::TicksRight);
  pointSizeSlider->setTickInterval(1);
  pointSizeSlider->setPageStep(1);
  connect(pointSizeSlider, &QSlider::valueChanged, this, &Viewer::_updatePointSize);

  //
  // make 'color by' control label and combobox
  //
  _lblColorBy = new QLabel();
  auto cbColorMode = new QComboBox();
  cbColorMode->addItems(QStringList()<<"color by Z axis"<<"color by row");
  connect(cbColorMode, static_cast<void(QComboBox::*)( int ) >(&QComboBox::currentIndexChanged), [=](const int newValue) {
    _scene->setColorAxisMode(newValue == 0 ? Scene::COLOR_BY_Z : Scene::COLOR_BY_ROW);
  });

  //
  // make 'clipping planes' controllers
  //
  const float CP_SLIDER_RANGE = 2000;
  auto fcpLabel = new QLabel();
  auto frontClippingPlaneSlider = new QSlider(Qt::Horizontal);
  frontClippingPlaneSlider->setRange(0, CP_SLIDER_RANGE);
  frontClippingPlaneSlider->setSingleStep(1);
  connect(frontClippingPlaneSlider, &QSlider::valueChanged, [=](int newValue) {
    const float v = float(newValue) / CP_SLIDER_RANGE;
    _camera->setFrontCPDistance(-v);
    fcpLabel->setText(tr("Front clipping plane Z: %1").arg(v));
  });
  frontClippingPlaneSlider->setValue(1);

  auto farcpLabel = new QLabel();
  auto farClippingPlaneSlider = new QSlider(Qt::Horizontal);
  farClippingPlaneSlider->setRange(0, CP_SLIDER_RANGE);
  farClippingPlaneSlider->setSingleStep(1);
  //farClippingPlaneSlider->setPageStep(10);
  connect(farClippingPlaneSlider, &QSlider::valueChanged, [=](int newValue) {
    const float v = float(newValue) / CP_SLIDER_RANGE;
    _camera->setRearCPDistance(v);
    farcpLabel->setText(tr("Far clipping plane Z: %1").arg(v));
  });
  farClippingPlaneSlider->setValue(CP_SLIDER_RANGE);

  //
  // compose 'Measuring tool' group
  //
  auto gbMeasuringTool = new QGroupBox(tr("Measuring tool"));
  auto mtLayout = new QVBoxLayout();
  gbMeasuringTool->setLayout(mtLayout);
  _lblDistanceInfo = new QLabel();

  auto cbActiveMT = new QCheckBox(tr("Active"));
  cbActiveMT->setChecked(false);
  connect(cbActiveMT, &QCheckBox::stateChanged, [=](int state) {
    _scene->setPickpointEnabled(state == Qt::Checked);
  });

  auto btnClearMT = new QPushButton(tr("Clear"));
  btnClearMT->setMaximumWidth(100);
  connect(btnClearMT, &QPushButton::pressed, [=]() {
    _scene->clearPickedpoints();
  });
  mtLayout->addWidget(cbActiveMT);
  mtLayout->addWidget(btnClearMT);
  mtLayout->addWidget(_lblDistanceInfo);

  //
  // compose control panel
  //
  QWidget* cpWidget = new QWidget();
  QVBoxLayout* controlPanel = new QVBoxLayout();
  cpWidget->setMaximumWidth(300);
  cpWidget->setLayout(controlPanel);
  controlPanel->addWidget(_lblColorBy);
  controlPanel->addWidget(pointSizeSlider);
  controlPanel->addSpacing(20);
  controlPanel->addWidget(cbColorMode);
  controlPanel->addSpacing(40);
  controlPanel->addWidget(new QLabel(tr("Camera angles")));
  controlPanel->addWidget(xSlider);
  controlPanel->addWidget(ySlider);
  controlPanel->addWidget(zSlider);
  controlPanel->addSpacing(20);
  controlPanel->addWidget(fcpLabel);
  controlPanel->addWidget(frontClippingPlaneSlider);
  controlPanel->addSpacing(20);
  controlPanel->addWidget(farcpLabel);
  controlPanel->addWidget(farClippingPlaneSlider);
  controlPanel->addSpacing(20);
  controlPanel->addWidget(gbMeasuringTool);
  controlPanel->addStretch(2);

  //
  // compose main layout
  //
  QHBoxLayout *mainLayout = new QHBoxLayout;
  mainLayout->addWidget(_scene);
  mainLayout->addWidget(cpWidget);
  setLayout(mainLayout);

  //
  // initial state of scene and controls
  //
  xSlider->setValue(0);
  ySlider->setValue(0);
  zSlider->setValue(0);

  _updatePointSize(1);
  cbColorMode->setCurrentIndex(0);

  _camera->setPosition(QVector3D(0, -0.1, -0.2));
  _camera->rotate(0, 50, 0);
  _scene->setColorAxisMode(Scene::COLOR_BY_Z);
  _scene->setPickpointEnabled(false);
}


void Viewer::wheelEvent(QWheelEvent* e) {
  if (e->angleDelta().y() > 0) {
    _camera->forward();
  } else {
    _camera->backward();
  }
}


void Viewer::keyPressEvent(QKeyEvent* keyEvent) {
  switch ( keyEvent->key() )
  {
    case Qt::Key_Escape:
      QApplication::instance()->quit();  // DEV MODE
      break;

    case Qt::Key_Left:
    case Qt::Key_A:
      _camera->left();
      break;

    case Qt::Key_Right:
    case Qt::Key_D:
      _camera->right();
      break;

    case Qt::Key_Up:
    case Qt::Key_W:
      _camera->forward();
      break;

    case Qt::Key_Down:
    case Qt::Key_S:
      _camera->backward();
      break;

    case Qt::Key_Space:
    case Qt::Key_Q:
      _camera->up();
      break;

    case Qt::Key_C:
    case Qt::Key_Z:
      _camera->down();
      break;

    default:
      QWidget::keyPressEvent(keyEvent);
  }
}


void Viewer::_updatePointSize(int value) {
  _scene->setPointSize(value);
  _lblColorBy->setText(QString("Point size: %1").arg(value));
}


void Viewer::_updateMeasureInfo(const QVector<QVector3D>& points) {
  QString text;
  if (!points.empty()) {
    const QVector3D& p = points[0];
    text += tr("(%1,  %2,  %3)\n").arg(p.x()).arg(p.y()).arg(p.z());
  }

  if (points.size() == 2) {
    const QVector3D& p = points[1];
    text += tr("(%1,  %2,  %3)\n").arg(p.x()).arg(p.y()).arg(p.z());

    float distance = points[0].distanceToPoint(points[1]);
    text += tr("Distance:  %1").arg(distance);
  }
  _lblDistanceInfo->setText(text);
}
