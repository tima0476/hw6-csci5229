/*
 * Timothy Mason, homework 5: Lighting
 * CSCI 5229  Computer Graphics - University of Colorado Boulder
 *
 * rocket.c
 *
 * Module for drawing a cartoon rocket.
 * ToDo:  Generalize this for drawing any arbitrary cylindrical surface
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

//  OpenGL with prototypes for glext
#define GL_GLEXT_PROTOTYPES
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "CSCIx229.h"
#include "rocket.h"

// The following shapes were manually traced and digitized on graph paper.
static duopoint rocket_profile[] = {
   {0   ,  32   },
   {1   ,  31.4 },
   {2   ,  30.6 },
   {3.1 ,  29.5 },
   {4   ,  28.4 },
   {4.9 ,  27.3 },
   {5.6 ,  26.1 },
   {6.2 ,  25   },
   {6.7 ,  24   },
   {7.1 ,  23   },
   {7.5 ,  22   },
   {7.9 ,  21   },
   {8.2 ,  20   },
   {8.5 ,  19   },
   {8.7 ,  18   },
   {8.85,  16.9 },
   {8.9 ,  16   },
   {8.9 ,  15   },
   {8.9 ,  14   },
   {8.9 ,  13   },
   {8.85,  12   },
   {8.8 ,  11   },
   {8.7 ,  10   },
   {8.5 ,  9    },
   {8.2 ,  8    },
   {7.9 ,  7    },
   {7.5 ,  5.9  },
   {7.1 ,  5    },
   {6.6 ,  4    },
   {6.1 ,  3    },
   {5.5 ,  2    },
   {4.8 ,  1    },
   {3.8 ,  0    }
};
#define ROCKET_POINT_COUNT	(sizeof(rocket_profile) / sizeof(rocket_profile[0]))

static char rlp_valid = 0;    // Non-zero if the rocket_lat_pct array has been computed
static double rocket_lat_pct[ROCKET_POINT_COUNT];  // Array of latitude percentages (how far is a point between the tip and the tail)
static double finmin = __DBL_MAX__;
static double finmax = - __DBL_MAX__;

static duopoint rocket_fin[] = {
   {6,	   17.1},
   {7,	   17.1},
   {8.1,	   17},
   {9.2,	   16.8},
   {10,	   16.5},
   {10.9,	16},
   {11.8,	15},
   {12.2,	14},
   {12.6,	13},
   {12.9,	12},
   {13.1,	11},
   {13.4,	10},
   {13.6,	9},
   {13.8,	8},
   {13.9,	7},
   {14,	   6},
   {14.1,	5},
   {14.2,	4},
   {14.3,	3},
   {14.3,	2},
   {14.3,	1},
   {14.3,	0},
   {13.8,	0},
   {13.6,	1},
   {13.3,	2},
   {13.05,	3},
   {12.8,	4},
   {12.6,	5},
   {12.35,	6},
   {12.2,	6.7},
   {12,	   7.5},
   {11.8,	8},
   {11.6,	8.6},
   {11.4,	9},
   {11.2,	9.6},
   {10.9,	10},
   {10.4,	10.4},
   {9.9,	   10.6},
   {9.2,	   10.8},
   {8.6,	   10.9},
   {8,	   11},
   {7.3,	   11.1},
   {6.6,	   11.15},
   {6,	   11.2},
};
#define ROCKET_FIN_POINT_COUNT	(sizeof(rocket_fin) / sizeof(rocket_fin[0]))


/*
 * Draw vertex in cylindrical coordinates (r, theta, z)
 */
void cylVertex(double r, double th, double z)
{
   glVertex3d(r*Cos(th), r*Sin(th), z);
}

/*
 * Draw normal in cylindrical coordinates (r, theta, z)
 */
void cylNormal(double r, double th, double z)
{
   glNormal3d(r*Cos(th), r*Sin(th), z);
}

/*
 * Draw a radially symmetric solid.  Automatically calculate and add surface normals for lighting
 *
 *    profile: pointer to an array of x,y coordinates representing the surface profile of the solid
 *    size: The number of points in the profile array
 *    bz,by,bz: 3D coordinates of the base of the solid
 *    rx,ry,rz: 3D vector for rotation of the solid.
 *    ph:  Angle to rotate the solid around (rx,ry,rz)
 *    s: the scale of the solid
 *    d: The angular increment for each slice of the radially symmetric solid
 *    tex: The texture to apply
 *    basetex: The texture for the base
 */
void lathe(dpp profile, int size, double bx, double by, double bz, double rx, double ry, double rz, double ph, double s, double d, unsigned int tex, unsigned int basetex)
{
   double th;
   int i;
   double nlr, nlz;     // Temporary storage for computing surface normals of the pair of vertices on the left (lower index)
   double nrr, nrz;     // Temporary storage for computing surface normals of the pair of vertices on the right (higher index)

   // Set texture
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, tex);

   // Save transformation
   glPushMatrix();

   // Offset and scale
   glTranslated(bx,by,bz);
   glRotated(ph, rx, ry, rz);
   glScaled(s,s,s);

   glColor3f(1.0, 1.0, 1.0);     // Everything is white.  Colors now imparted by textures

   // Latitude bands
   for (i=1; i<size; i++)
   {
      glBegin(GL_QUADS);
      for (th=0.0; th<=360.0; th+=d)
      {
         //
         // Calculate the radius and z components of the surface normals.  For the first and last segments in the profile,
         // compute normals from adjacent vertices.  For all other (middle) segments, compute from vertices that are two
         // clicks away, thereby getting an average
         //
         nlr = (i>1 ? profile[i-2].z : profile[i-1].z) - profile[i].z;
         nlz = profile[i].r - (i>1 ? profile[i-2].r : profile[i-1].r);

         nrr = profile[i-1].z - (i+1<size ? profile[i+1].z : profile[i].z);      // The way the outer loop is constructed, there is always a contribution...
         nrz = (i+1<size ? profile[i+1].r : profile[i].r) - profile[i-1].r;      // ...from the segment to the left of the right point

         // if there is a point to the right of this one, then add in it's contribution to the surface normal
         cylNormal(profile[i-1].z - profile[i].z, th+d/2.0, profile[i].r - profile[i-1].r);

         //
         // Draw the quad in counter-clockwise order
         //
         cylNormal(nlr, th, nlz);  
         glTexCoord2d(th/360.0, rocket_lat_pct[i-1]);    
         cylVertex(profile[i-1].r, th,   profile[i-1].z);
         
         cylNormal(nrr, th, nrz);      
         glTexCoord2d(th/360.0, rocket_lat_pct[i]);    
         cylVertex(profile[i  ].r, th,   profile[i  ].z);
         
         cylNormal(nrr, th+d, nrz);    
         glTexCoord2d((th+d)/360.0, rocket_lat_pct[i]);    
         cylVertex(profile[i  ].r, th+d, profile[i  ].z);
         
         cylNormal(nlr, th+d, nlz);    
         glTexCoord2d((th+d)/360.0, rocket_lat_pct[i-1]);    
         cylVertex(profile[i-1].r, th+d, profile[i-1].z);
      }
      glEnd();
   }

   // Assume no top cap is necessary (ie., the first point has r coordinate == 0)

   // Bottom cap
   glDisable(GL_TEXTURE_2D);
   
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, basetex);
   if (profile[size-1].r != 0.0)
   {
      // Draw a triangle fan from the origin to the final circle.
      glBegin(GL_TRIANGLE_FAN);
      
      glColor3f(1, 1, 1);
      glTexCoord2d(0.5,0.5);  // The first point is the center of the circle
      glNormal3d(0,0,1);      // Surface normal is straight down the z-axis
      glVertex3d(0, profile[size-1].z, 0);

      for (th = 0; th <= 360; th += d)
      {
         glTexCoord2d(0.5+0.5*Cos(th), 0.5+0.5*Sin(th));
         cylVertex(profile[size-1].r, th, profile[size-1].z);
      }
      glEnd();
   }

   // undo transformations
   glPopMatrix();
   glDisable(GL_TEXTURE_2D);
}

/*
 * Draw rocket fins equidistant around the rotation
 *
 *    bz,by,bz: 3D coordinates of the base of the rocket
 *    rx,ry,rz: 3D vector for rotation of the rocket.
 *    ph:  Angle to rotate the rocket
 *    s: the scale of the rocket
 *    fc: the number of fins on the rocket
 *    tex: The texture to apply
 */
void draw_fins(double bx, double by, double bz, double rx, double ry, double rz, double ph, double s, int fc, unsigned int tex)
{
   int dth = 360/fc;
   int th,i;

   // Set texture
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, tex);

   // Save transformation
   glPushMatrix();

   // Offset and scale
   glTranslated(bx,by,bz);
   glRotated(ph, rx, ry, rz);
   glScaled(s,s,s);

   // Draw rocket fins, spaced equally around the cylinder   
   for (th=0; th<=360; th += dth)
   {
      glBegin(GL_QUADS);      // The fin shape is non-convex, so can't use a simple polygon
      glColor3f(1.0, 1.0, 1.0);       // Everything is now white to accomodate textures

      // The rocket fin is a flat plane, so all vertices have the same surface normal
      cylNormal(1, th+90, 0);

      // The rocket fin is non-convex, so it cannot be drawn as a single polygon.  This loop will draw it
      // instead as a strip of quads, making the assumption that the first point in the array is adjacent to
      // the last point, the 2nd point is adjacent to the 2nd-to-last point, and so on.
      for (i=0; i<(ROCKET_FIN_POINT_COUNT/2 - 2); i++)
      {
         // Draw each quad CCW so the double-sided lighting will work
         glTexCoord2d((rocket_fin[i  ].r - finmin)/(finmax-finmin), 
               (rocket_fin[i  ].z - finmin)/(finmax-finmin));
         cylVertex(rocket_fin[i  ].r, th, rocket_fin[i  ].z);
         
         glTexCoord2d((rocket_fin[i+1].r - finmin)/(finmax-finmin), 
               (rocket_fin[i+1].z - finmin)/(finmax-finmin));
         cylVertex(rocket_fin[i+1].r, th, rocket_fin[i+1].z);
         
         glTexCoord2d((rocket_fin[ROCKET_FIN_POINT_COUNT-i-2].r - finmin)/(finmax-finmin), 
               (rocket_fin[ROCKET_FIN_POINT_COUNT-i-2].z - finmin)/(finmax-finmin));
         cylVertex(rocket_fin[ROCKET_FIN_POINT_COUNT-i-2].r, th, 
               rocket_fin[ROCKET_FIN_POINT_COUNT-i-2].z);
         
         glTexCoord2d((rocket_fin[ROCKET_FIN_POINT_COUNT-i-1].r - finmin)/(finmax-finmin), 
               (rocket_fin[ROCKET_FIN_POINT_COUNT-i-1].z - finmin)/(finmax-finmin));
         cylVertex(rocket_fin[ROCKET_FIN_POINT_COUNT-i-1].r, th, 
               rocket_fin[ROCKET_FIN_POINT_COUNT-i-1].z);
      }

      glEnd();
   }

   // undo transformations
   glPopMatrix();   
   glDisable(GL_TEXTURE_2D);
}

/*
 * Draw a cartoon rocket ship
 *
 *    bz,by,bz: 3D coordinates of the base of the rocket
 *    rx,ry,rz: 3D vector for rotation of the rocket.
 *    ph:  Angle to rotate the rocket
 *    s: the scale of the rocket
 *    fc: how many fins the rocket gets
 *    d: The angular increment for each slice of the rocket
 *    tex: The texture to apply
 * 	basetex: The texture for the base
 */
void rocket(double bx, double by, double bz, double rx, double ry, double rz, double ph, double s, int fc, double d, unsigned int tex, 
		unsigned int fintex, unsigned int basetex)
{
   // If this is the first time called, then populate the latitudinal progression array.  This array will be used for texture 
   // mapping so that the texture will not be distorted, even if the rocket skin control points are not evenly spaced.
   if (!rlp_valid) {
      rocket_lat_pct[0] = 0.0;
      double sum = 0.0;
      for (int i = 1; i < ROCKET_POINT_COUNT; i++)
      {
         double dist = sqrt( pow(rocket_profile[i].r-rocket_profile[i-1].r, 2.0) + pow(rocket_profile[i].z-rocket_profile[i-1].z, 2.0));
         sum += dist;
         rocket_lat_pct[i] = sum;  // Initially store accumulated distance.
      }

      // Now the total distance is known; normalize the pct traveled array
      for (int i = 0; i < ROCKET_POINT_COUNT; i++)
         rocket_lat_pct[i] /= sum;
      rlp_valid = 1;

      // While we're here, scan the rocket fin points for the min and max values (used for mapping the texture)
      for (int i = 0; i < ROCKET_FIN_POINT_COUNT; i++)
      {
         // Find the coordinates of the smallest square which fully contains all of the points in the fin.
         finmin = MIN( finmin, rocket_fin[i].r );
         finmin = MIN( finmin, rocket_fin[i].z );
         finmax = MAX( finmax, rocket_fin[i].r );
         finmax = MAX( finmax, rocket_fin[i].z );
      }
   }

   
   // Draw the main rocket cylinder
   lathe(rocket_profile, ROCKET_POINT_COUNT, bx, by, bz, rx, ry, rz, ph, s, d, tex, basetex);

   // Now add some fins
   draw_fins(bx, by, bz, rx, ry, rz, ph, s, fc, fintex);
}
