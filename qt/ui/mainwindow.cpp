#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "ArrayUtil.hpp"
#include "Array3D.h"
#include "ConfigFile.hpp"
#include "FileUtil.hpp"
#include "octglwidget.h"
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QImageReader>
#include <QImageWriter>
#include <QMessageBox>
#include <QMimeData>
#include <QStandardPaths>
#include <QThread>
#include <QThreadPool>
#include <algorithm>
#include <iostream>
#include <vector>
#include <filesystem>
#include <functional>

namespace fs = std::experimental::filesystem;

void saveVolume(std::string outdir, int scanCount, const std::vector<uint8_t> & vol, const int * volSize,
    int zCrop0, int zCrop1);

void loadVolume(std::string filename, std::vector<uint8_t> & vol, int * volSize);

double transformA, transformB, rotateAngle;

std::string viewModeString[2] = { "scan", "gradient" };

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  imagingThread(new ImagingThread()),
  scanCount(0), 
  viewMode(VIEW_SCAN),
  captureMode(0), saveCropZ(0)
{
  
  ui->setupUi(this);
  ui->imageViewer->setBackgroundRole(QPalette::Base);
  ui->actionOpen->setShortcut(QKeySequence::Open);
  setAcceptDrops(true);

  connect(ui->actionReloadConfig, SIGNAL(triggered()), this, SLOT(reloadConfig()));
  connect(ui->imageIndexSlider, SIGNAL(valueChanged(int)), this, SLOT(selectImage(int)));
  connect(ui->volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(selectVolumeSlice(int)));
  connect(ui->viewModeButton, &QPushButton::clicked, this, &MainWindow::toggleViewMode);
  connect(ui->actionLoad_Images, &QAction::triggered, this, &MainWindow::loadImages);
  connect(ui->actionLoadBin, &QAction::triggered, this, &MainWindow::loadBin);
  connect(ui->actionLoadVol, &QAction::triggered, this, &MainWindow::loadVol);
  connect(ui->actionStartRender, &QAction::triggered, this, &MainWindow::startGL);

  imagingThread->main = this;
  imagingThread->glViewer = ui->glViewer;
  connect(imagingThread, &ImagingThread::selectImage, this, &MainWindow::selectImage);
  connect(imagingThread, &ImagingThread::refreshRecon, this, &MainWindow::refreshSurface);
  ui->glViewer->setConfigFile(&conf);
  ui->viewModeLabel->setText(QString::fromStdString(viewModeString[0]));
}

MainWindow::~MainWindow()
{
  delete ui;
}

bool MainWindow::loadConfigFile(const std::string & filename)
{
  configFilename = filename;
  int retval = conf.load(filename);
	if (conf.hasOpt("scanCount")) {
		scanCount = conf.getInt("scanCount");
	}

  conf.getInt("saveCropZ", saveCropZ);
  conf.getInt("captureMode", captureMode);
  std::vector<float> transform = conf.getFloatVector("transform");
  if (transform.size() == 3) {
    transformA = transform[0];
    transformB = transform[1];
    rotateAngle = transform[2];
  }
  return retval==0;
}

void MainWindow::refreshSurface()
{
}

void MainWindow::loadImages()
{
  if (!conf.hasOpt("imageDir")) {
    qDebug() << "Need specify imageDir in config.txt.\n";
    return;
  }
}

void MainWindow::loadBin() {
}

void MainWindow::loadVolStr(const std::string & filename) {
    FileUtilIn in;
    int volSize[3];
    in.open(filename, std::ifstream::binary);
    in.in.read((char*)(&volSize[0]), sizeof(int));
    in.in.read((char*)(&volSize[1]), sizeof(int));
    in.in.read((char*)(&volSize[2]), sizeof(int));
    size_t nVox = volSize[0] * volSize[1] * (size_t)volSize[2];
    bool status = vol.allocate(volSize[0], volSize[1], volSize[2]);
    if (!status) {
        std::cout << "Error: wrong file or volume is too large to be read.\n";
        std::cout << "Volume size "<<volSize[0]<<" "<<volSize[1]<<" "<<volSize[2]<<"\n";
        return;
    }

    in.in.read((char*)vol.data.data(), nVox);
    in.close();

    std::cout << "Volume size " << volSize[0] << " " << volSize[1] << " " << volSize[2] << "\n";
    ui->imageIndexSlider->setMinimum(0);
    ui->imageIndexSlider->setMaximum((int)volSize[1] - 1);
    selectImage(0);
}

void MainWindow::loadVol() {
    QString fileName = QFileDialog::getOpenFileName(this, ("Open File"),
        "/home", ("Binary volume (*.bin)"));
    if (!fileName.isNull()) {
        qDebug() << "selected file path : " << fileName.toUtf8();
    }
    std::string f = fileName.toStdString();
    loadVolStr(f);
}

void MainWindow::reloadConfig() {
  loadConfigFile(configFilename);
}

void MainWindow::selectImage(int index)
{
    index = std::max(0, index);
    index = std::min((int)vol.size[1] - 1, index);
        
    QImage qimage(vol.size[0], vol.size[2], QImage::Format_ARGB32);
    uchar* bits = qimage.bits();
    for (int row = 0; row < qimage.height(); row++) {
        for (int col = 0; col < qimage.width(); col++) {
            unsigned char val = vol(col, index, row);
            bits[qimage.bytesPerLine() * row + 4 * col] = val;
            bits[qimage.bytesPerLine() * row + 4 * col + 1] = val;
            bits[qimage.bytesPerLine() * row + 4 * col + 2] = val;
            bits[qimage.bytesPerLine() * row + 4 * col + 3] = 255;
        }
    }

    QPixmap pixmap = QPixmap::fromImage(qimage);

    ui->imageViewer->setFixedSize(pixmap.size());
    ui->imageViewer->setPixmap(pixmap);
}

void MainWindow::selectRawImage(int index)
{
	selectImage(index);
}

void MainWindow::toggleViewMode() {
    switch (viewMode) {
    case VIEW_SCAN:
        viewMode = VIEW_GRAD;
        break;
    case VIEW_GRAD:
        viewMode = VIEW_SCAN;
        break;
    }
    ui->viewModeLabel->setText(QString::fromStdString(viewModeString[viewMode]));
}

void MainWindow::selectVolumeSlice(int index)
{
}

void MainWindow::closeEvent(QCloseEvent * )
{
	imagingThread->wait();
}

void loadVolume(std::string filename, std::vector<uint8_t> & vol, int * volSize) {
    FileUtilIn in;
    in.open(filename, std::ifstream::binary);
    in.in.read((char*)(&volSize[0]), sizeof(int));
    in.in.read((char*)(&volSize[1]), sizeof(int));
    in.in.read((char*)(&volSize[2]), sizeof(int));
    size_t nVox = volSize[0] * volSize[1] * (size_t)volSize[2];
    vol.resize(nVox);
    in.in.read((char*)vol.data(), nVox);
    in.close();
}

void saveVolume(std::string outdir, int scanCount, const std::vector<uint8_t> & vol, const int * volSize,
    int zCrop0, int zCrop1) {
    int zSize = zCrop1 - zCrop0;

    //write z offset to a separate text file.
    std::string zfile = outdir + "/zoffset.txt";
    FileUtilOut zOffsetOut;
    zOffsetOut.open(zfile, std::ofstream::app);
    zOffsetOut.out << scanCount << " " << zCrop0 << "\n";
    zOffsetOut.close();

    //save volume as 3d array. three 4-byte integers followed by a list of uint8_t
    size_t outSize = volSize[0] * volSize[1] * (size_t)zSize;
    std::vector<uint8_t> outVol(outSize);
    for (int i = 0; i < volSize[0]; i++) {
        for (int j = 0; j < volSize[1]; j++) {
            for (int k = zCrop0; k < zCrop1; k++) {
                outVol[i*volSize[1] * zSize + j * zSize + k - zCrop0] = vol[i*volSize[1] * volSize[2] + j * volSize[2] + k];
            }
        }
    }
    std::string filename = outdir + "/" + std::to_string(scanCount) + ".bin";
    FileUtilOut out;
    out.open(filename, std::ofstream::binary);
    out.out.write((const char *)volSize[0], sizeof(int));
    out.out.write((const char *)volSize[1], sizeof(int));
    out.out.write((const char *)(&zSize), sizeof(int));
    out.out.write((const char *)outVol.data(), outVol.size());
    out.close();
}

class UpdateHeightFieldTask:public QRunnable
{
public:
	UpdateHeightFieldTask(MainWindow * m, OCTGLWidget * g) :main(m), glViewer(g){}
	OCTGLWidget * glViewer;
	MainWindow * main;
	void run() {
	}
};

void ImagingThread::run()
{
	UpdateHeightFieldTask * hfTask = new UpdateHeightFieldTask(main, glViewer);
	QThreadPool::globalInstance()->start(hfTask);

	while (main!= nullptr ) {
		QThread::msleep(100);
	}
	emit resultReady();
}

void MainWindow::startGL()
{
	ui->glViewer->startRender();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();

    // check for our needed mime type, here a file or a list of files
    if (mimeData->hasUrls())
    {
        QStringList pathList;
        QList<QUrl> urlList = mimeData->urls();

        // extract the local paths of the files
        if (urlList.size() > 0) {
            QString filename = (urlList.at(0).toLocalFile());
            std::string f = filename.toStdString();
            loadVolStr(f);
        }
        event->acceptProposedAction();
    }
}