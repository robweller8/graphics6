#include <cmath>
#include <algorithm>
#include<iostream>
#include "light.h"



using namespace std;

double DirectionalLight::distanceAttenuation( const Vec3d& P ) const
{
  // distance to light is infinite, so f(di) goes to 0.  Return 1.
  return 1.0;
}


Vec3d DirectionalLight::shadowAttenuation( const Vec3d& P ) const
{
//cout << "HERE1!!!" << endl;
  // YOUR CODE HERE:
  // You should implement shadow-handling code here.
  isect i;
  ray r(P, -orientation, ray::SHADOW);
  if(scene->intersect( r, i )){
    return Vec3d(0,0,0);
  } 
  else {
cout << "HERE2!!!" << endl;
    return Vec3d(1,1,1);//i.getMaterial().kt(i);
  }

}

Vec3d DirectionalLight::getColor( const Vec3d& P ) const
{
  // Color doesn't depend on P 
  return color;
}

Vec3d DirectionalLight::getDirection( const Vec3d& P ) const
{
  return -orientation;
}

double PointLight::distanceAttenuation( const Vec3d& P ) const
{

  // YOUR CODE HERE
  // These three values are the a, b, and c in the distance
  // attenuation function (from the slide labelled 
  // "Intensity drop-off with distance"):
  //    f(d) = min( 1, 1/( a + b d + c d^2 ) )
 //     float constantTerm;		// a
 //     float linearTerm;		// b
 //     float quadraticTerm;	// c

  // You'll need to modify this method to attenuate the intensity 
  // of the light based on the distance between the source and the 
  // point P.  For now, we assume no attenuation and just return 1.0

  Vec3d distance = position - P;
  double d = distance.length();
  double intensity = min(1.0, 1.0/(constantTerm + 
                                     linearTerm*d + 
                                       quadraticTerm*pow(d,2)));

  return intensity;

}

Vec3d PointLight::getColor( const Vec3d& P ) const
{
  // Color doesn't depend on P 
  return color;
}

Vec3d PointLight::getDirection( const Vec3d& P ) const
{
  Vec3d ret = position - P;
  ret.normalize();
  return ret;
}


Vec3d PointLight::shadowAttenuation(const Vec3d& P) const
{

  // YOUR CODE HERE:
  // You should implement shadow-handling code here.
  Vec3d direction = getDirection(P);
  ray r(P, direction, ray::SHADOW );
  isect i;
  if(scene->intersect( r, i )){
    return Vec3d(0,0,0);
  } 
  else {
    return Vec3d(1,1,1);//i.getMaterial().kt(i);
  }
}
