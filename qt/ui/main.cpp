#include "mainwindow.h"
#include "FileUtil.hpp"
#include <deque>
#include <QApplication>
#include <QDebug>
#include <iostream>
#include <thread>
#include <QSurfaceFormat>


int main(int argc, char *argv[])
{

  QApplication a(argc, argv);
	// Set OpenGL Version information
	// Note: This format must be set before show() is called.
	QSurfaceFormat format;
	format.setRenderableType(QSurfaceFormat::OpenGL);
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setVersion(3, 3);

  QSurfaceFormat::setDefaultFormat(format);

	MainWindow w;
	if(argc>1){
    std::string configFilename (argv[1]);
    qDebug()<<"load config file "<<QString(configFilename.c_str());
    w.loadConfigFile(configFilename);
  }
	w.show();
  return a.exec();
}
