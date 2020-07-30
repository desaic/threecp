#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCloseEvent>
#include <QMainWindow>
#include <QImage>
#include <QThread>
#include <array>
#include <thread>
#include "ConfigFile.hpp"
#include "Image.h"
#include "Volume.hpp"
class ImagingThread;
class OCTGLWidget;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);

  bool loadConfigFile(const std::string & filename);
  void selectRawImage(int idx);
  //load volume file specified by std::string filename
  bool loadVolStr(const std::string & filename);
  bool loadFloatImg(const std::string & filename);
  ~MainWindow();

  enum ViewMode{VIEW_SCAN, VIEW_GRAD};

protected:
    //@override
    void dropEvent(QDropEvent * event);
    void dragEnterEvent(QDragEnterEvent *e);

private slots:
  void refreshSurface();
  void loadImages();
  void loadBin();
  void loadVol();
  void reloadConfig();
  void selectImage(int index);
  void selectVolumeSlice(int index);
  void toggleViewMode();
  void startGL();
  void saveSlice();
  
private:
  Ui::MainWindow *ui;
  std::string configFilename;
  ConfigFile conf;

  void closeEvent(QCloseEvent * event);
  ImagingThread * imagingThread;

  ViewMode viewMode;
  //number of scans run.
  int scanCount;
  int captureMode;
  int saveCropZ;
  int sliceCount;
  Volume vol;
  ImageF32 image;

};

class ImagingThread :public QThread {
	Q_OBJECT
		void run() override;
signals:
	void resultReady();
	void refreshRecon();
	void selectImage(int index);
public:
	ImagingThread() :main(0), glViewer(0) {}
	MainWindow * main;
	OCTGLWidget * glViewer;
};

#endif // MAINWINDOW_H
