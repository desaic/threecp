#ifndef GLCAMERA_H
#define GLCAMERA_H

#include <QMatrix4x4>
#include <QVector3D>

//camera coordinate frame and projection matrix.
class GLCamera
{
public:
    GLCamera();
	//at is always at the same y as eye.
	QVector3D eye, at;
	QMatrix4x4 vMat, projMat;
	//field of view radians.
	float fov, aspRatio, zNear, zFar;
	//updates view matrix vMat and projection matrix projMat.
	void update();
	//rotate view around current x axis.
	void rotX(float radians);
	//rotate view around constant y axis.
	void rotY(float radians);
	//move camera relative.
	void moveRel(float x, float y, float z);
	//move camera absolute.
	void moveAbs(float x, float y, float z);

	void reset();
private:
	//up is always (0,1,0).
	QVector3D up;
	//xrot is bounded between (-0.5 pi, 0.5 pi).
	float angleX;
};

#endif // GLCAMERA_H