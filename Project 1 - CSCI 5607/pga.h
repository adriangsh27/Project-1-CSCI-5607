//CSCI 5611 PGA Library
//This header defines the following 2D PGA operations:
// -wedge product, dot product, vee product, dual, reverse
//over the following elements:
// -points, directions, lines, motors, multivectors

#ifndef PGA_H
#define PGA_H

#include "multivector.h"
#include "primitives.h"

//--------
// Operations that act on various primitives
//--------

//Operations on Directions
inline Point2D Dir2D::operator+(Point2D rhs){
  return MultiVector(*this) + rhs;
}

inline Dir2D Dir2D::operator+(Dir2D rhs){
  return MultiVector(*this) + rhs;
}

inline Dir2D Dir2D::operator-(Dir2D rhs){
  return MultiVector(*this) - rhs;
}

//Operations on Points
inline Dir2D Point2D::operator-(Point2D rhs){
  return MultiVector(*this) - rhs;
}

inline Point2D Point2D::operator+(Dir2D rhs){
  return MultiVector(*this) + rhs;
}

inline Point2D Point2D::operator+(Point2D rhs){
  return MultiVector(*this) + rhs;
}

//Operations on Homogeneous Points
inline HomogeneousPoint2D HomogeneousPoint2D::operator-(HomogeneousPoint2D rhs){
  return MultiVector(*this) - rhs;
}

inline HomogeneousPoint2D HomogeneousPoint2D::operator+(HomogeneousPoint2D rhs){
  return MultiVector(*this) + rhs;
}

//Wedge Product
//Note: We could have a wedge between two lines return a Point2D to be faster, but that fails when the lines are parallel [performance]
inline HomogeneousPoint2D wedge(Line2D lhs, Line2D rhs){
  return MultiVector(lhs).wedge(rhs);
}


//Vee/joint Operators 
inline Line2D vee(Point2D lhs, Point2D rhs){
  return MultiVector(lhs).vee(rhs);
}

//Quiz: Why no vee() that takes in 4 points?
inline float vee(Point2D a, Point2D b, Point2D c){
  return (MultiVector(a).vee(b).vee(c)).s;
}

inline Line2D vee(Point2D lhs, Dir2D rhs){
  return MultiVector(lhs).vee(rhs);
}

inline float vee(Line2D lhs, Point2D rhs){
  return (MultiVector(lhs).vee(rhs)).s;
}

inline float vee(Point2D lhs, Line2D rhs){
  return (MultiVector(lhs).vee(rhs)).s;
}

inline float vee(Line2D lhs, Dir2D rhs){
  return (MultiVector(lhs).vee(rhs)).s;
}

inline float vee(Dir2D lhs, Line2D rhs){
  return (MultiVector(lhs).vee(rhs)).s;
}

inline float vee(HomogeneousPoint2D a, HomogeneousPoint2D b, HomogeneousPoint2D c){
  return (MultiVector(a).vee(b).vee(c)).s;
}

inline Line2D vee(HomogeneousPoint2D lhs, HomogeneousPoint2D rhs){
  return MultiVector(lhs).vee(rhs);
}

inline float vee(Line2D lhs, HomogeneousPoint2D rhs){
  return (MultiVector(lhs).vee(rhs)).s;
}

inline float vee(HomogeneousPoint2D lhs, Line2D rhs){
  return (MultiVector(lhs).vee(rhs)).s;
}


//Dot product
inline float dot(Line2D lhs, Line2D rhs){
  return (MultiVector(lhs).dot(rhs)).s;
}

inline Line2D dot(Point2D lhs, Line2D rhs){
  return MultiVector(lhs).dot(rhs);
}

inline Line2D dot(Line2D lhs, Point2D rhs){
  return MultiVector(lhs).dot(rhs);
}

inline Line2D dot(HomogeneousPoint2D lhs, Line2D rhs){
  return MultiVector(lhs).dot(rhs);
}

inline Line2D dot(Line2D lhs, HomogeneousPoint2D rhs){
  return MultiVector(lhs).dot(rhs);
}

//--------
// Constructors to turn MultiVectors in points, directions, lines, and motors
//--------

inline Point2D::Point2D(MultiVector mv){
  x = mv.yw/mv.xy;
  y = mv.wx/mv.xy;
}

inline Dir2D::Dir2D(MultiVector mv){
  x = mv.yw;
  y = mv.wx;
}

inline HomogeneousPoint2D::HomogeneousPoint2D(MultiVector mv){
  x = mv.yw;
  y = mv.wx;
  w = mv.xy;
}

inline Motor2D::Motor2D(MultiVector mv){
  s = mv.s;
  yw = mv.yw;
  wx = mv.wx;
  xy = mv.xy;
}

inline Line2D::Line2D(MultiVector mv){
  x = mv.x;
  y = mv.y;
  w = mv.w;
}


//--------
// Helper Functions
//--------
inline float clamp(float f, float min, float max) {
  const float t = f < min ? min : f;
  return f > max ? max : f;
}

inline int sign(float f){
  if (f >= 0) return 1;
  return -1;
}

#endif