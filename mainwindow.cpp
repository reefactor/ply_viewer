#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QLabel>
#include <QApplication>
#include <QDesktopWidget>

#include "mainwindow.h"
#include "viewer.h"


const QString TITLE = QObject::tr("Points Cloud Viewer");


MainWindow::MainWindow()
{

  // fit into 80% of a desktop size
  const QRect desk = QApplication::desktop()->availableGeometry(QApplication::desktop()->screenNumber(this));
  resize(desk.width() * .8, desk.height() * .8);

  setWindowTitle(TITLE);
  setMinimumSize(600, 400);

  // create menu
  QMenuBar *menuBar = new QMenuBar;
  setMenuBar(menuBar);
  QMenu *fileMenu = menuBar->addMenu(tr("&File"));
  QAction *openFile = new QAction(tr("&Open"), fileMenu);
  fileMenu->addAction(openFile);
  connect(openFile, &QAction::triggered, this, &MainWindow::_openFileDialog);
  QAction *closeView = new QAction(tr("&Close"), fileMenu);
  fileMenu->addAction(closeView);
  connect(closeView, &QAction::triggered, this, &MainWindow::_closeView);

  // take first command line argument as a path to PLY files
  if (QApplication::arguments().size() > 1) {
    _openView(QApplication::arguments()[1]);
  } else {
    // place some hints on a screen
    auto welcomeHint = new QLabel();
    welcomeHint->setTextFormat(Qt::RichText);
    QString t = "<font color=gray>";
    t += "<h1><u>Welcome</u></h1>";
    t += "<p />";
    t += "<ul>";
    t += "<li>Use menu <b>File</b> -> <b>Open</b> to load PLY file</li>";
    t += "<li>Also first provided command line argument is treated as path to file</li>";
    t += "<li><h2>Navigation hints</h2></li>";
    t += "<ul>";
    t += "<li>Use mouse to rotate camera</li>";
    t += "<li>Use mouse with keyboard <i>SHIFT</i> modifier to pan camera</li>";
    t += "<li>Use mouse scroll to zoom (move camera forward, backward)</li>";
    t += "<li>Use keyboard <i>UP, DOWN, LEFT, RIGHT</i> (or <i>W,S,A,D</i>) to move camera forward, backward, left and right</li>";
    t += "<li>Use keyboard <i>Q, Z</i> (or <i>SPACE, C</i>) to move camera up and down</li>";
    t += "</ul>";
    t += "<ul>";
    t += "</ul>";
    t += "<ul>";
    t += "<font>";
    welcomeHint->setText(t);
    welcomeHint->setMargin(50);
    setCentralWidget(welcomeHint);
  }
}


void MainWindow::_openFileDialog()
{
  const QString filePath = QFileDialog::getOpenFileName(this, tr("Open PLY file"), "", tr("PLY Files (*.ply)"));
  if (!filePath.isEmpty()) {
    _openView(filePath);
  }
}


void MainWindow::_openView(const QString& filePath) {
  _closeView();

  try {
    // create new new
    setCentralWidget(new Viewer(filePath));
    // add source path into title
    setWindowTitle(QString("%1 - %2").arg(filePath).arg(TITLE));
  } catch (const std::exception& e) {
    QMessageBox::warning(this, tr("Cannot open view"), e.what());
  }
}


void MainWindow::_closeView()
{
  if (centralWidget()) {
    // destroy view
    centralWidget()->close();
    takeCentralWidget()->deleteLater();
    // remove source path from title
    setWindowTitle(TITLE);
  }
}

