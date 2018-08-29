#include "octglwidget.h"
#include "ConfigFile.hpp"
#include "FileUtil.hpp"
#include "TrigMesh.hpp"
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>

const int OCTGLWidget::nBuf = 3;

void OCTGLWidget::checkGLError()
{
	GLenum error = GL_NO_ERROR;
	do {
		error = glGetError();
		if (error != GL_NO_ERROR) {
			qDebug() << error << "\n";
		}
		// handle the error
	} while (error != GL_NO_ERROR);

}

OCTGLWidget::OCTGLWidget(QWidget *parent) : QOpenGLWidget(parent),
program(0),
conf(0), texLoc(0), dxLoc(0),
maxX(10),maxY(10),timer(new QTimer()),
running(false)
{
}

OCTGLWidget::~OCTGLWidget()
{
	cleanup();
}

void OCTGLWidget::initializeGL()
{
	if (conf == 0) {
		std::cout << "octglwidget needs config file.\n";
		return;
	}

	connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &OCTGLWidget::cleanup);
	connect(timer, SIGNAL(timeout()), this, SLOT(update()));

	initializeOpenGLFunctions();
	printVersionInformation();
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	lightPos = QVector3D(0.5, 3, 0.5);
}

void OCTGLWidget::updateVertArray(TrigMesh * m, GLVertArray * vertArray)
{
	int dim = 3;
	int nFloat = 3 * dim * (int)m->t.size();
	int nV = 3 * (int)m->t.size();
	vertArray->v.resize(nFloat);
	vertArray->n.resize(nFloat);
	vertArray->c.resize(nFloat);
	vertArray->s.resize(nV);

	//copy per-triangle vertices
	int cnt = 0;
	for (size_t i = 0; i < m->t.size(); i++) {
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < dim; k++) {
				vertArray->v[cnt] = (GLfloat)m->v[m->t[i][j]][k];
				cnt++;
			}
		}
	}

	//copy normals.
	cnt = 0;
	m->compute_norm();
	for (size_t i = 0; i < m->t.size(); i++) {
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < dim; k++) {
				vertArray->n[cnt] = m->n[m->t[i][j]][k];//(GLfloat)normal[k];
				cnt++;
			}
		}
	}

	//copy colors
	GLfloat color[3] = { 0.2f, 0.2f, 0.2f };
	cnt = 0;
	for (size_t i = 0; i < m->t.size(); i++) {
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < dim; k++) {
				vertArray->c[cnt] = color[k];
				cnt++;
			}
		}
	}

	//copy vertex scales.
	//cnt = 0;
	//for (size_t i = 0; i < m->t.size(); i++) {
	//	for (int j = 0; j < 3; j++) {
	//		vertArray->s[cnt] = (GLfloat)1.0f;
	//		cnt++;
	//	}
	//}
}

void OCTGLWidget::copyVertBuffer(GLVertArray * vertArray, ShaderBuffer* buf)
{

	buf->b[0].bind();
	buf->b[0].write(0, (void*)vertArray->v.data(), vertArray->v.size()* sizeof(GLfloat));
	buf->b[0].release();

	buf->b[1].bind();
	buf->b[1].write(0, (void*)vertArray->n.data(), vertArray->n.size() * sizeof(GLfloat));
	buf->b[1].release();

	buf->b[2].bind();
	buf->b[2].write(0, (void*)vertArray->c.data(), vertArray->c.size() * sizeof(GLfloat));
	buf->b[2].release();

	//buf->b[3].bind();
	//buf->b[3].write(0, (void*)vertArray->s.data(), vertArray->s.size() * sizeof(GLfloat));
	//buf->b[3].release();

}

void OCTGLWidget::initTrigBuffers(TrigMesh * m)
{	
	GLVertArray * vertArray = new GLVertArray();
	vertArrays.push_back(vertArray);
	updateVertArray(m, vertArray);

	ShaderBuffer * bufptr = new ShaderBuffer();
	buffers.push_back(bufptr);
	ShaderBuffer & buf = *bufptr;
	
	buf.vao.create();
	buf.vao.bind();
	buf.b.resize(nBuf);
	for (size_t i = 0; i < buf.b.size(); i++) {
		buf.b[i].create();
	}
	int dim = 3;
	int nFloat = 3 * dim * (int)m->t.size();
	int nV = 3 * (int)m->t.size();
	
	buf.b[0].bind();
	buf.b[0].allocate(nFloat * sizeof(GLfloat));
	buf.b[0].setUsagePattern(QOpenGLBuffer::DynamicDraw);
	program->enableAttributeArray(0);
	program->setAttributeBuffer(0, GL_FLOAT, 0, 3);
	buf.b[0].release();

	buf.b[1].bind();
	buf.b[1].allocate(nFloat * sizeof(GLfloat));
	program->enableAttributeArray(1);
	program->setAttributeBuffer(1, GL_FLOAT, 0, 3);
	buf.b[1].release();

	buf.b[2].bind();
	buf.b[2].allocate(nFloat * sizeof(GLfloat));
	program->enableAttributeArray(2);
	program->setAttributeBuffer(2, GL_FLOAT, 0, 3);
	buf.b[2].release();

	//buf.b[3].bind();
	//buf.b[3].allocate(nV * sizeof(GLfloat));
	//program->enableAttributeArray(3);
	//program->setAttributeBuffer(3, GL_FLOAT, 0, 1);
	//buf.b[3].release();

	buf.vao.release();
	
	copyVertBuffer(vertArray, bufptr);
}

void OCTGLWidget::startRender()
{
	if (running) {
		return;
	}
	if (program != 0) {
		return;
	}
	staleMesh = false;
	//this may be initialted from other UI threads.
	makeCurrent();
	program = new QOpenGLShaderProgram();
	std::string vsString, fsString;
	if (conf == 0) {
		std::cout << "gl needs config file.\n";
		return;
	}
	std::string vsFile, fsFile;
	if (conf->hasOpt("vertexShader")) {
		vsFile = conf->dir + "/" + conf->getString("vertexShader");
	}
	vsString = loadTxtFile(vsFile);
	if (vsString.size() < 3) {
		std::cout << "invalid vert shader file " << vsFile << "\n";
	}

	if (conf->hasOpt("fragmentShader")) {
		fsFile = conf->dir + "/" + conf->getString("fragmentShader");
		fsString = loadTxtFile(fsFile);
	}
	if (fsString.size() < 3) {
		std::cout << "invalid frag shader file " << fsFile << "\n";
	}
	std::cout <<"compile shaders " << vsFile << ", " << fsFile << "\n";

	bool success = false;
	success = program->addShaderFromSourceCode(QOpenGLShader::Vertex, vsString.c_str());
	qDebug() <<"compile vs output: "<< program->log();
	success = program->addShaderFromSourceCode(QOpenGLShader::Fragment, fsString.c_str());
	qDebug() << "compile fs output: " << program->log();
	success = program->link();
	qDebug() << "shader link output: " << program->log();
	if (!success) {
		qDebug() << program->log() << "\n";
	}
	program->bind();
	
	mvpLoc = program->uniformLocation("MVP");
	mvitLoc = program->uniformLocation("MVIT");
	dxLoc = program->uniformLocation("dx");
	lightLoc = program->uniformLocation("lightPos");
	
	//load a test mesh for debugging shaders.
	if (conf->hasOpt("testMesh")) {
		std::string testMeshFile = conf->dir + "/" + conf->getString("testMesh");
		FileUtilIn in(testMeshFile);
		if (in.good()) {
			TrigMesh * testMesh = new TrigMesh();
			testMesh->load(in.in);
			trigs.push_back(testMesh);
			initTrigBuffers(testMesh);
		}
	}

	maxX = (int)conf->getInt("meshX");
	maxY = (int)conf->getInt("meshY");
	TrigMesh * plane = new TrigMesh();
	makePlane(plane, maxX, maxY);
	float dx = 0.5 / maxX;
	float dy = 0.5 / maxY;
	//shift x and z by 1/2 dx so that each vertex is at the center of each texture pixel.
	for (size_t i = 0; i < plane->v.size(); i++) {
		plane->v[i][0] += dx;
		plane->v[i][2] += dy;
	}
	trigs.push_back(plane);

	checkGLError();

	initTrigBuffers(plane);
	checkGLError();

	texLoc = program->uniformLocation("texsampler");
	checkGLError();

	qDebug() << "tex loc " << texLoc << "\n";
	heightField.resize(maxX*maxY, 0);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, maxX, maxY, 0, GL_RED, GL_UNSIGNED_BYTE, heightField.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	cam.reset();
	rotation = QVector3D(0, 0, 0);
	program->release();

	timer->start(15);
	doneCurrent();
	running = true;
}

void OCTGLWidget::resizeGL(int w, int h)
{
	cam.aspRatio = w / float(h);
	cam.update();
}

void OCTGLWidget::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	if (program == nullptr) {
		return;
	}
	cam.update();
	QMatrix4x4 model;
	model.setToIdentity();
	model.translate(0.5, 0, 0.5);
	model.rotate(rotation[0], 1, 0, 0);
	model.rotate(rotation[1], 0, 1, 0);
	model.translate(-0.5, 0, -0.5);
	
	QMatrix4x4 mv = cam.vMat * model;
	QMatrix4x4 mvp = cam.projMat * mv;
	QMatrix3x3 mvit = model.normalMatrix();
	
	program->bind();
	glBindTexture(GL_TEXTURE_2D, texture);

	if (staleMesh) {
		checkGLError();
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, maxX, maxY, GL_RED, GL_UNSIGNED_BYTE, heightField.data());
		checkGLError();
		staleMesh = false;
	}

	glActiveTexture(GL_TEXTURE0);
	QVector2D dx(1.0 / maxX, 1.0 / maxY);
	checkGLError();
	program->setUniformValue(texLoc, 0);
	checkGLError();
	program->setUniformValue(lightLoc, lightPos);
	program->setUniformValue(mvpLoc, mvp);
	program->setUniformValue(mvitLoc, mvit);
	program->setUniformValue(dxLoc, dx);
	for (size_t i = 0; i < buffers.size(); i++) {
		buffers[i]->vao.bind();
		checkGLError();
		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(3 * trigs[i]->t.size()));
		
		buffers[i]->vao.release ();
	}

	program->release();

}

void OCTGLWidget::setStale(bool val)
{
	staleMesh = val;
}

void OCTGLWidget::setTexture(const QImage & image)
{
	//if (texLoc != nullptr) {
	//	makeCurrent();

	//	doneCurrent();
	//}
}

void OCTGLWidget::updateHeightField(const std::vector<float> & surf, const uint32_t * surfSize)
{
	if (heightField.size() == 0 || buffers.size() != trigs.size()) {
		return;
	}

	int xsize = std::min(surfSize[0], maxX);
	int ysize = std::min(surfSize[1], maxY);
	for (int i = 0; i < xsize; i++) {
		for (int j = 0; j < ysize; j++) {
			int vidx = i * (maxY) + j;
			int surfIdx = j * surfSize[0] + i;
			heightField[vidx] = 255 - surf[surfIdx]/4;
		}
	}
}

void OCTGLWidget::cleanup()
{
	if (program == nullptr)
		return;
	makeCurrent();

	for (size_t i = 0; i < trigs.size(); i++) {
		delete trigs[i];
	}
	for (size_t i = 0; i < buffers.size(); i++) {
		for (size_t j = 0; j < buffers[i]->b.size(); j++) {
			buffers[i]->b[j].destroy();
		}
		delete buffers[i];
	}
	delete program;
	program = 0;
	doneCurrent();
}

void OCTGLWidget::mousePressEvent(QMouseEvent* event)
{
	lastPos = event->pos();
}

void OCTGLWidget::mouseMoveEvent(QMouseEvent* event)
{
	QPoint newPos = event->pos();
	QPoint dx = newPos - lastPos;
	if (event->buttons() == Qt::LeftButton) {
		rotation[0] += dx.y() * -0.02;
		rotation[1] += dx.x() * 0.02;
		rotation[0] = fmod(rotation[0],  360);
		rotation[1] = fmod(rotation[1], 360);
	}
	else if (event->buttons() == Qt::RightButton) {
		cam.moveRel(0.01*dx.x(), 0.01*dx.y(), 0);
	}
	lastPos = newPos;
	
}

void OCTGLWidget::wheelEvent(QWheelEvent *event)
{
	QPoint numPixels = event->pixelDelta();
	QPoint numDegrees = event->angleDelta() / 8;

	if (!numPixels.isNull()) {
		
	}
	else if (!numDegrees.isNull()) {
		cam.moveRel(0, 0, numDegrees.y()*0.1f/15);
	}

	event->accept();
}

void OCTGLWidget::keyPressEvent(QKeyEvent * event)
{
	switch (event->key()) {
	case Qt::Key_C:
		//reset camera position
		cam.reset();
		rotation = QVector3D(0, 0, 0);
		break;
	default:
		event->ignore();
		break;
	}
}

void OCTGLWidget::printVersionInformation()
{
	QString glType;
	QString glVersion;
	QString glProfile;

	// Get Version Information
	glType = (context()->isOpenGLES()) ? "OpenGL ES" : "OpenGL";
	glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));

	// Get Profile Information
#define CASE(c) case QSurfaceFormat::c: glProfile = #c; break
	switch (format().profile())
	{
		CASE(NoProfile);
		CASE(CoreProfile);
		CASE(CompatibilityProfile);
	}
#undef CASE

	// qPrintable() will print our QString w/o quotes around it.
	qDebug() << qPrintable(glType) << qPrintable(glVersion) << "(" << qPrintable(glProfile) << ")";
}
