// The main ray tracer.

#pragma warning (disable: 4786)

#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"

#include "parser/Tokenizer.h"
#include "parser/Parser.h"

#include "ui/TraceUI.h"
#include <cmath>
#include <algorithm>

extern TraceUI* traceUI;

#include <iostream>
#include <fstream>

using namespace std;

// Use this variable to decide if you want to print out
// debugging messages.  Gets set in the "trace single ray" mode
// in TraceGLWindow, for example.
bool debugMode = false;

// Trace a top-level ray through normalized window coordinates (x,y)
// through the projection plane, and out into the scene.  All we do is
// enter the main ray-tracing method, getting things started by plugging
// in an initial ray weight of (0.0,0.0,0.0) and an initial recursion depth of 0.
Vec3d RayTracer::trace( double x, double y )
{
  // Clear out the ray cache in the scene for debugging purposes,
  scene->intersectCache.clear();

  ray r( Vec3d(0,0,0), Vec3d(0,0,0), ray::VISIBILITY );

  scene->getCamera().rayThrough( x,y,r );
  Vec3d ret = traceRay( r, Vec3d(1.0,1.0,1.0), 0 );
  ret.clamp();
  return ret;
}

// Do recursive ray tracing!  You'll want to insert a lot of code here
// (or places called from here) to handle reflection, refraction, etc etc.
Vec3d RayTracer::traceRay( const ray& r, const Vec3d& thresh, int depth )
{
  isect i;
  isect j;
  Vec3d color;
   /*             if(r.type() == ray::VISIBILITY)
    cout << " visibility rayll!!" << depth << endl;
                if(r.type() == ray::REFLECTION)
    cout << " reflective rayll!!" << depth << endl;
                if(r.type() == ray::REFRACTION)
    cout << " REFRACTIVE RAYYYYl!!" << depth << endl;*/
  if( scene->intersect( r, i ) ) {
    // YOUR CODE HERE

    // An intersection occured!  We've got work to do.  For now,
    // this code gets the material for the surface that was intersected,
    // and asks that material to provide a color for the ray.  

    // This is a great place to insert code for recursive ray tracing.
    // Instead of just returning the result of shade(), add some
    // more steps: add in the contributions from reflected and refracted
    // rays.
    const Material& m = i.getMaterial();
    if(depth <= traceUI->getDepth()) {
      Vec3d position = r.at(i.t);
      Vec3d normal = i.N;
      normal.normalize();
      Vec3d incoming = r.getPosition() - position;
      incoming.normalize();
      Vec3d reflectionD = normal*(2*(incoming*normal)) - incoming;
      reflectionD.normalize();
      ray reflective(position, reflectionD, ray::REFLECTION );
      color += prod((traceRay(reflective, Vec3d(1.0,1.0,1.0), depth + 1)),m.kr(i));

      double n1;
      double n2;
      double mIndex = m.index(i);

      n1 = 1.0;
      n2 = mIndex;
      double ratio = n1/n2;
      if(r.type() == ray::REFRACTION)
        ratio = n2/n1;
      double ratio2 = ratio*ratio;
      
      double theta = (incoming*(-1))*normal;
      double internal = 1 - ratio2*(1 - (theta*theta));
      double angle = sqrt(internal);
      Vec3d refraction = (-1)*incoming*ratio + normal*(ratio*theta - angle);
      refraction.normalize();
      ray refractive( position, refraction, ray::REFRACTION );
      ray refractive2( position, refraction, ray::VISIBILITY );
      scene->intersect( refractive, j );
      Vec3d pos = refractive.getPosition();
      if(internal >= 0){
        if(r.type() != ray::REFRACTION)
          color += prod((traceRay(refractive, Vec3d(1.0,1.0,1.0), depth+1)),m.kt(i));
        else
          color += prod((traceRay(refractive2, Vec3d(1.0,1.0,1.0), depth+1)),m.kt(i));
      }
      color += m.shade(scene, r, i);
    }
    else{
    color = Vec3d( 0.0, 0.0, 0.0 );
    }
    return color;

  } else {
    // No intersection.  This ray travels to infinity, so we color
    // it according to the background color, which in this (simple) case
    // is just black.

    return Vec3d( 0.0, 0.0, 0.0 );
  }
}

RayTracer::RayTracer()
    : scene( 0 ), buffer( 0 ), buffer_width( 256 ), buffer_height( 256 ), m_bBufferReady( false )
{

}


RayTracer::~RayTracer()
{
  delete scene;
  delete [] buffer;
}

void RayTracer::getBuffer( unsigned char *&buf, int &w, int &h )
{
  buf = buffer;
  w = buffer_width;
  h = buffer_height;
}

double RayTracer::aspectRatio()
{
  return sceneLoaded() ? scene->getCamera().getAspectRatio() : 1;
}

bool RayTracer::loadScene( char* fn )
{
  ifstream ifs( fn );
  if( !ifs ) {
    string msg( "Error: couldn't read scene file " );
    msg.append( fn );
    traceUI->alert( msg );
    return false;
  }
	
  // Strip off filename, leaving only the path:
  string path( fn );
  if( path.find_last_of( "\\/" ) == string::npos )
    path = ".";
  else
    path = path.substr(0, path.find_last_of( "\\/" ));

  // Call this with 'true' for debug output from the tokenizer
  Tokenizer tokenizer( ifs, false );
  Parser parser( tokenizer, path );
  try {
    delete scene;
    scene = 0;
    scene = parser.parseScene();
  } 
  catch( SyntaxErrorException& pe ) {
    traceUI->alert( pe.formattedMessage() );
    return false;
  }
  catch( ParserException& pe ) {
    string msg( "Parser: fatal exception " );
    msg.append( pe.message() );
    traceUI->alert( msg );
    return false;
  }
  catch( TextureMapException e ) {
    string msg( "Texture mapping exception: " );
    msg.append( e.message() );
    traceUI->alert( msg );
    return false;
  }


  if( ! sceneLoaded() )
    return false;

  return true;
}

void RayTracer::traceSetup( int w, int h )
{
  if( buffer_width != w || buffer_height != h )
  {
    buffer_width = w;
    buffer_height = h;

    bufferSize = buffer_width * buffer_height * 3;
    delete [] buffer;
    buffer = new unsigned char[ bufferSize ];


  }
  memset( buffer, 0, w*h*3 );
  m_bBufferReady = true;
}

void RayTracer::tracePixel( int i, int j )
{
  Vec3d col;

  if( ! sceneLoaded() )
    return;

  double x = double(i)/double(buffer_width);
  double y = double(j)/double(buffer_height);


  col = trace( x,y );

  unsigned char *pixel = buffer + ( i + j * buffer_width ) * 3;

  pixel[0] = (int)( 255.0 * col[0]);
  pixel[1] = (int)( 255.0 * col[1]);
  pixel[2] = (int)( 255.0 * col[2]);
}


