#ifndef OCTGLWIDGET_H
#define OCTGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QMatrix4x4>
#include <QTimer>

#include "glcamera.h"

class ConfigFile;
class TrigMesh;
QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram);

struct ShaderBuffer
{
	//vertex array object
	//each object corresponds to a mesh.
	QOpenGLVertexArrayObject vao;
	//vertex and color buffer
	//vertex of each mesh contains 4 attributes corresponding to 
	//variables in vertex shader
	//vertex position vec3, normal  vec3, color vec3, and a scale float.
	std::vector<QOpenGLBuffer> b;
	//index buffer. unused.
	QOpenGLBuffer index;
	ShaderBuffer() :index(QOpenGLBuffer(QOpenGLBuffer::IndexBuffer)) {}
};

struct GLVertArray
{
	//vertex positions
	std::vector<GLfloat> v;
	//normals
	std::vector<GLfloat> n;
	//colors
	std::vector<GLfloat> c;
	//scales
	std::vector<GLfloat> s;
};

class OCTGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    //Q_OBJECT
public:
    explicit OCTGLWidget(QWidget *parent = nullptr);
	~OCTGLWidget();

	void setConfigFile(ConfigFile * c) { conf = c; }
	void startRender( );
	void updateHeightField(const std::vector<float> & surf, const uint32_t * surfSize);
	void setStale(bool val);
	void setTexture(const QImage & image);
signals:

public slots :
		void cleanup();
protected:
	void initializeGL();
	void resizeGL(int w, int h);
	void paintGL();
	void checkGLError();
	void mousePressEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent *event);
	void keyPressEvent(QKeyEvent * event);
	void wheelEvent(QWheelEvent *event);

private:
	//number of opengl vertex buffer objects for each mesh.
	//set to 3 for position , normal, and color.
	static const int nBuf;
	QOpenGLShaderProgram * program;
	int mvpLoc, mvitLoc, lightLoc, dxLoc;
	ConfigFile * conf;
	std::vector<TrigMesh *> trigs;
	std::vector<ShaderBuffer*> buffers;
	std::vector<GLVertArray *> vertArrays;
	
	QVector3D rotation;
	GLCamera cam;
	void initTrigBuffers(TrigMesh * m);
	void updateVertArray(TrigMesh * m, GLVertArray * vertArray);
	void copyVertBuffer(GLVertArray * vertArray, ShaderBuffer* buf);
	void OCTGLWidget::printVersionInformation();
	uint32_t maxX;
	uint32_t maxY;
	std::vector<uint8_t> heightField;
	QPoint lastPos;
	QVector3D lightPos;
	bool staleMesh;

	GLint texLoc;
	GLuint texture;
	//timer for drawing .
	QTimer *timer;
	bool running;
};

#endif // OCTGLWIDGET_H
