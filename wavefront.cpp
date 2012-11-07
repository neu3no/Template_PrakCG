#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <math.h>
#ifdef __APPLE__
  #include <GLUT/glut.h>
#else
  #include <GL/glut.h>
  #include <GL/gl.h>
#endif
#define EXPORT_EXT
#include "wavefront.h"

/* wavefront */

// extensions laden wird nur unter windows benötigt
#ifdef _WIN32 || _WIN64
int init_extensions(void) {
	glGenBuffers = (PFNGLGENBUFFERSARBPROC) wglGetProcAddress("glGenBuffers");
	glBindBuffer = (PFNGLBINDBUFFERARBPROC) wglGetProcAddress("glBindBuffer");
	glBufferData = (PFNGLBUFFERDATAARBPROC) wglGetProcAddress("glBufferData");
	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC) wglGetProcAddress("glDeleteBuffers");

	if (glGenBuffers) return (1);
	else return (0);
} 
#endif

inline float* normieren (float v[3]) {
  float l=1.0f/sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
  v[0]*=l; v[1]*=l; v[2]*=l;
  return v;
}


void cross_product(float *n, float *a, float *b) {
	n[0] = a[1]*b[2] - a[2]*b[1];
	n[1] = a[2]*b[0] - a[0]*b[2];
	n[2] = a[0]*b[1] - a[1]*b[0];
};

void set_obj_material (object3D *obj, float red, float green, float blue, float alpha, float spec, float shine, float emis) {
  if (!obj) return;
  obj->color[0]=red; obj->color[1]=green; obj->color[2]=blue; obj->color[3]=alpha; 
  obj->amb[0]=0.1; obj->amb[1]=0.1; obj->amb[2]=0.1; obj->amb[3]=alpha; 
  obj->diff[0]=red; obj->diff[1]=green; obj->diff[2]=blue; obj->diff[3]=alpha; 
  obj->spec[0]=spec; obj->spec[1]=spec; obj->spec[2]=spec; obj->spec[3]=alpha; 
  obj->shine=shine; 
  // Emission = r,g,b * emis
  obj->emis[0]=red*emis; obj->emis[1]=blue*emis, obj->emis[2]=green*emis, obj->emis[3]=alpha*emis; 
}
void set_obj_pos (object3D *obj, float x, float y, float z) {
  if (!obj) return;
  obj->pos[0]=x; obj->pos[1]=y; obj->pos[2]=z; 
}

object3D* loadobject (char *filename, bool use_vbos, float red, float green, float blue, float x, float y, float z) {
  if (!filename) return NULL;
  FILE *f=fopen(filename,"r");
  if (!f) return NULL;
  /*
	  fseek(f,0,SEEK_END);
	  int l=ftell(f);
	  rewind (f);
	  char *buf=(char*)malloc (l+1);
	  l=fread(buf,1,l,f);
  */
  int npoints=0,ntris=0;
  char s[200];

  // Pass 1: Anzahl der Dreiecke in der Datei zählen
  while (!feof(f)) {
	fscanf(f,"%s",&s);
	if (s[1]==0) {				// Zeichenkette besteht nur aus 1 Zeichen
	  if (s[0]=='v') ++npoints; // das Zeichen ist 'v'
	  else if (s[0]=='f') {		// oder 'f'
		++ntris;				// mindestens 1 Dreieck
		int p[3];				// das Dreieck mit 3 Punkten
		fscanf(f,"%d %d %d",&p[0],&p[1],&p[2]);	// Dreieck lesen
		while (fscanf(f,"%d",p)==1)	// noch ein weiterer Dezimalwert in der Zeile??
		  ++ntris;				// JA: noch ein Dreieck 
	  }
	}
  }

  // Pass 2: Objekt laden
  struct object3D *obj=NULL;
  if (npoints > 0 && ntris > 0) {
	obj=(object3D*)malloc(sizeof(object3D));   // Objekt anlegen
	obj->points=(GLfloat*)malloc(npoints*3*sizeof(GLfloat));	// Punktarray anlegen
	obj->tris  =(GLint*)malloc(ntris*3*sizeof(GLint));			// Index-Array anlegen
	npoints=0; ntris=0;			// Initialisierung
	obj->normal_mode = 0;		// vorerst keine Normalen
	obj->vbo_geladen = 0;		// vorerst keine VBOs
	
	rewind(f);
    while (!feof(f)) {
	  fscanf(f,"%s",&s);		// siehe oben
	  if (s[1]==0) {			// 1 Zeichen gelesen
		if (s[0]=='v') {		// Vertex - Zeile
		  float *p=&obj->points[3*npoints];		// pointer auf Vertex setzen
		  fscanf(f,"%f %f %f",&p[0],&p[1],&p[2]);	// Werte lesen
		  ++npoints;							// nächster Vertex
		} else
		if (s[0]=='f') {		// Flächen - Zeile
		  int *p=&obj->tris[3*ntris];	// Pointer auf akt. Dreieck setzen
          fscanf(f,"%u %u %u",&p[0],&p[1],&p[2]);	// Dreieck lesen
		  --p[0]; --p[1]; --p[2];		// Indizes dekrementieren (1->0)	
		  ++ntris;				// nächstes Dreieck
		  int d;	
		  while (fscanf(f,"%u",&d)==1) { // noch ein Dezimalwert in der Zeile
		    int *q=&obj->tris[3*ntris];	 // Pointer setzen
			q[0]=p[0];					 // Dreieck laden: p0 = letztes Dreieck.p0
			q[1]=p[2];					 //                p1 = letztes Dreieck.p1
			q[2]=d-1;					 //                p2 = neuer Punktindex
			p=q;						 // Dreieck weiterschalten
	        ++ntris;	
		  }
		}
	  }
	}
	obj->numpoints=npoints;
	obj->numtris=ntris;
  }
  fclose(f);

  if (!obj) return NULL;

  // calculate surface normals (one normal per surface)
  // let's start with surface normals
  obj->f_normals  = (float*)malloc(3*ntris*sizeof(float));
  for (int i=0; i<ntris; i++) {
	  // calculate edges per triangle

	  int *act_tri = &(obj->tris[3*i]);
	  float *p0 = &(obj->points[3*act_tri[0]]);
	  float *p1 = &(obj->points[3*act_tri[1]]);
	  float *p2 = &(obj->points[3*act_tri[2]]);

	  float v1[3];
	  float v2[3];
	  
	  v1[0] = p1[0] - p0[0];
	  v1[1] = p1[1] - p0[1];
	  v1[2] = p1[2] - p0[2];

	  v2[0] = p2[0] - p0[0];
	  v2[1] = p2[1] - p0[1];
	  v2[2] = p2[2] - p0[2];

	  float *act_n = &(obj->f_normals[3*i]);

	  cross_product(act_n, v1, v2);
	  normieren(act_n);
  }

  // now let's continue with interpolated normals per vertex
  obj->p_normals = (float*)malloc(3*npoints*sizeof(float));
  memset(obj->p_normals,0,3*npoints*sizeof(float));
  for (int i=0; i<npoints; i++) {
	  // for each point accumulate the face normals of faces the act. point is member of
      float *act_n = &(obj->p_normals[3*i]);
	  for (int j=0; j<ntris; j++) {
		  //loop over tris and check if p is member of act triangle
		  int *act_tri = &(obj->tris[3*j]);
		  if ((act_tri[0] == i) || (act_tri[1] == i) || (act_tri[2] == i)) {
			  // add face normal
			  float *act_fn = &(obj->f_normals[3*j]);
			  act_n[0] += act_fn[0];
			  act_n[1] += act_fn[1];
			  act_n[2] += act_fn[2];
		  }
	  }
	  normieren(act_n);
  }


    //////////////////////////////////////////////////
	// Objekt geladen: Erzeugen und Laden nun die VBOs
    //////////////////////////////////////////////////
  
  if (use_vbos) {
    glGenBuffers(4, obj->vbos);
	if (obj->vbos[0]) {      // kein Fehler beim VBO-Generieren

		glEnableClientState(GL_VERTEX_ARRAY);
		//glEnableClientState(GL_COLOR_ARRAY);
		
			glBindBuffer(GL_ARRAY_BUFFER, obj->vbos[0]);		// Points (Geometry)
			glBufferData(GL_ARRAY_BUFFER,
						 obj->numpoints * 3 * sizeof(GLfloat),
						 obj->points,
						 GL_STREAM_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->vbos[1]); // Tris (Indexes)
			glBufferData(GL_ELEMENT_ARRAY_BUFFER,
						 obj->numtris * 3 * sizeof(GLuint),
						 obj->tris,
						 GL_STREAM_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, obj->vbos[2]);		// f_normals (Geometry)
			glBufferData(GL_ARRAY_BUFFER,
						 obj->numtris * 3 * sizeof(GLfloat),
						 obj->f_normals,
						 GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, obj->vbos[3]);		// p_normals (Geometry)
			glBufferData(GL_ARRAY_BUFFER,
						 obj->numpoints * 3 * sizeof(GLfloat),
						 obj->p_normals,
						 GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, 0);					// unbind Arrays
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);	

		//glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);

		obj->vbo_geladen = 1;
	}
	else exit(1);
  }

  set_obj_material (obj, red, green, blue, 1.0f, 0.0f, 128.0f, 0.0f);
  set_obj_pos(obj, x, y, z);

  return obj;
}

void freeobject (object3D *obj) {
  if (obj->points) free(obj->points);
  if (obj->tris) free(obj->tris);
  if (obj->f_normals) free(obj->f_normals);
  free(obj);
}

//
// Zeichnet ein Wavefront-Object *obj
//

void drawobject (object3D *obj) {
	int *p = NULL;  
	float *n = NULL;

	if (obj) {


	 if (!obj->vbo_geladen) {  // haben keine VBOs

		  glBegin (GL_TRIANGLES);
		  for (int i=0; i < obj->numtris; ++i) {
			  switch (obj->normal_mode) {
				  case 0:
						//set no normals
						p=&obj->tris[3*i];
						glVertex3fv (&obj->points[3*p[0]]);
						glVertex3fv (&obj->points[3*p[1]]);
						glVertex3fv (&obj->points[3*p[2]]);
					break;
				  case 1:	//set surface normal
						n = &(obj->f_normals[3*i]);
						glNormal3fv(n);
						p=&obj->tris[3*i];
						glVertex3fv (&obj->points[3*p[0]]);
						glVertex3fv (&obj->points[3*p[1]]);
						glVertex3fv (&obj->points[3*p[2]]);
					break;
				  case 2:
						p=&obj->tris[3*i];
						// set vertex normals
						glNormal3fv (&obj->p_normals[3*p[0]]);
						glVertex3fv (&obj->points[3*p[0]]);
						glNormal3fv (&obj->p_normals[3*p[1]]);
						glVertex3fv (&obj->points[3*p[1]]);
						glNormal3fv (&obj->p_normals[3*p[2]]);
						glVertex3fv (&obj->points[3*p[2]]);
					break;
			  }
		  }
		  glEnd();
	 }
	 else {
							// haben VBOs
	    glEnableClientState(GL_VERTEX_ARRAY);

		if (obj->normal_mode == 2) { // Normalen per Vertex
			glEnableClientState(GL_NORMAL_ARRAY);
			glBindBuffer(GL_ARRAY_BUFFER, obj->vbos[3]);			// Normals per Vertex
			glNormalPointer(GL_FLOAT, 0, 0);
		}

		glBindBuffer(GL_ARRAY_BUFFER, obj->vbos[0]);			// Points
		glVertexPointer(3, GL_FLOAT, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->vbos[1]);	// Indexes
		glDrawElements(GL_TRIANGLES, 3 * obj->numtris, GL_UNSIGNED_INT, 0);	

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		if (obj->normal_mode == 2) { // Normalen per Vertex
			glDisableClientState(GL_NORMAL_ARRAY);
		}

		glDisableClientState(GL_VERTEX_ARRAY);

	}
	}
}