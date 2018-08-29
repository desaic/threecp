#include "glcamera.h"

inline float clampf(float v, float lb, float ub)
{
	float ret = v;
	if (v < lb) {
		v = lb;
		return ret;
	}
	if (v > ub) {
		v = ub;
	}
	return ret;
}

GLCamera::GLCamera():eye(QVector3D(0, 0, -2)),
at(QVector3D(0, 0, 0)),
up(QVector3D(0, 1, 0)),
fov(45),
aspRatio(1.0f),
zNear(0.01f),
zFar(10.0f),
angleX(0)
{
	update();
}

//updates view matrix vMat and projection matrix projMat.
void GLCamera::update()
{
	at[1] = eye[1];
	QVector3D z = at - eye;
	z.normalize();
	at = eye + z;
	QVector3D y = up.normalized();
	QVector3D x = QVector3D::crossProduct(y, z);
	x.normalize();
	QMatrix4x4 rotMat;
	rotMat.setToIdentity();
	rotMat.rotate(angleX, x);
	z = (rotMat*QVector4D(z, 0)).toVector3D();
	y = QVector3D::crossProduct(z, x);
	

	vMat.setToIdentity();
	vMat.lookAt(eye, eye+z, up);
	projMat.setToIdentity();
	//fov in degrees.
	projMat.perspective(fov, aspRatio, zNear, zFar);
	
}

//rotate view around current x axis.
void GLCamera::rotX(float degrees)
{
	angleX += degrees;
	angleX = clampf(angleX, -89, 89);
}

//rotate view around constant y axis.
void GLCamera::rotY(float degrees)
{
	QMatrix4x4 rot;
	rot.setToIdentity();
	up.normalize();
	rot.rotate(degrees, up);
	QVector4D dir(at - eye, 0);
	dir = rot * dir;
	at = eye + dir.toVector3D();
}

//move camera relative.
void GLCamera::moveRel(float fx, float fy, float fz)
{
	
	at[1] = eye[1];
	QVector3D z = at - eye;
	z.normalize();
	at = eye + z;
	QVector3D y = up.normalized();
	QVector3D x = QVector3D::crossProduct(y, z);
	x.normalize();

	QVector3D newEye = eye + up*fy + fx * x + fz * z;
	moveAbs(newEye[0], newEye[1], newEye[2]);
}

//move camera absolute.
void GLCamera::moveAbs(float x, float y, float z)
{
	QVector3D dir = at - eye;
	eye = QVector3D(x, y, z);
	at = eye + dir;
}

void GLCamera::reset()
{
	eye = QVector3D(0.8f, 0.6, -0.5);
	at = QVector3D(0.6, 0.6, 0);
	up = QVector3D(0, 1, 0);
	angleX = 30;	
	update();
}
