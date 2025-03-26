#include "PointTrigDist.h"

void ComputeTrigFrame(const float* trig, const Vec3f& n, TrigFrame& frame)
{
  frame.x = Vec3f(trig[3] - trig[0], trig[4] - trig[1], trig[5] - trig[2]);
  float xlen = frame.x.norm();
  if (xlen > 0) {
    frame.x = (1.0f / xlen)*frame.x;
  } 
  frame.z = n;
  frame.y = frame.z.cross(frame.x);

  frame.v1x = xlen;
  Vec3f v2 = Vec3f(trig[6] - trig[0], trig[7] - trig[1], trig[8] - trig[2]);
  frame.v2x = v2.dot(frame.x);
  frame.v2y = v2.dot(frame.y);
}

TrigDistInfo PointTrigDist2D(float px, float py, float t1x, float t2x,
                             float t2y)
{
  float eps = 1e-12;
  TrigDistInfo info;  
  if (py <= 0) {
    //V0, V1, or E01
    if (px<=0) {
      info.type = TrigPointType::V0;
      info.sqrDist = px * px + py * py;
      info.bary = Vec3f(1, 0, 0);
      return info;
    }
    if (px > t1x) {
      info.type = TrigPointType::V1;
      info.sqrDist = (px - t1x) * (px - t1x) + py * py;
      info.bary = Vec3f(0, 1, 0);
      return info;
    }
    info.type = TrigPointType::E01;
    info.sqrDist = py * py;
    float l1 = px/(eps+t1x);
    info.bary = Vec3f(1-l1, l1, 0);
    return info;
  }

  //test E12
  Vec2f n12(t2y, -t2x+t1x);
  Vec2f pv1(px - t1x, py);
  Vec2f pv2(px - t2x, py-t2y);
  Vec2f e12(t2x - t1x, t2y);
  float edgeDist = pv1.dot(n12);
  
  if (edgeDist >= 0) {
    if (pv1.dot(e12)<=0) {
      info.type = TrigPointType::V1;
      info.sqrDist = (px - t1x) * (px - t1x) + py * py;
      info.bary = Vec3f(0, 1, 0);
      return info;
    }
    if (pv2.dot(e12) >= 0) {
      info.type = TrigPointType::V2;
      info.sqrDist = pv2.norm2();
      info.bary = Vec3f(0, 0, 1);
      return info;
    }
    info.type = TrigPointType::E12;   
    info.sqrDist = edgeDist * edgeDist / (eps+n12.norm2());
    float l2 = pv1.dot(e12) / (eps+n12.norm2());
    info.bary = Vec3f(0, 1 - l2, l2);
    return info;
  }
  //testE02
  Vec2f n20(-t2y, t2x);
  edgeDist = pv2.dot(n20);
  if (edgeDist >= 0) {
    if (pv2.dot(Vec2f(t2x, t2y))>=0) {
      info.type = TrigPointType::V2;
      info.sqrDist = pv2.norm2();
      info.bary = Vec3f(0, 0, 1);
      return info;
    }
    if (px*t2x+ py*t2y <= 0) {
      info.type = TrigPointType::V0;
      info.sqrDist = px*px+py*py;
      info.bary = Vec3f(1, 0, 0);
      return info;
    }
    info.type = TrigPointType::E02;
    info.sqrDist = edgeDist * edgeDist / (eps + n20.norm2());
    //abuse of local variable. n20.norm2() = e20.norm2()=t2x^2+t2y^2.
    float l2 = (px * t2x+py*t2y) / (eps + n20.norm2());
    info.bary = Vec3f( 1 - l2,0, l2);
    return info;
  }
  //interior
  info.type = TrigPointType::FACE;
  info.sqrDist = 0;
  float l2 = py / (eps + t2y);
  float l1 = (px - (l2 * t2x)) / (eps + t1x);
  info.bary = Vec3f(1 - l1 - l2, l1, l2);
  return info;
}