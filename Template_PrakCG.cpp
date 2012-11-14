#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#ifdef __APPLE__
  #include <GLUT/glut.h>
#else
  #include <GL/glut.h>
  #include <GL/gl.h>
#endif

#ifdef _WIN32 || _WIN64
	#include "glext.h"
#endif

#include "help.h"
#include "wavefront.h"


#ifndef	M_PI				// Pi
#define M_PI 3.14159265358979323846
#endif
#ifndef SGN					// Vorzeichen bestimmen
#define SGN(y) (((y) < 0) ? -1 : ((y) > 0))
#endif
#ifndef MIN					// Minimum bestimmen
#define MIN(a,b) ((a) > (b))? (b) : (a)
#endif
#ifndef MAX					// Maximum bestimmen
#define MAX(a,b) ((a) > (b))? (a) : (b)
#endif

/////////////////////////////////////////////////////////////////////////////////
//	Fenster Initialisierung
/////////////////////////////////////////////////////////////////////////////////
//! Die Startposition des Fensters (linke, obere Ecke)
#define WIN_POS_X	0
#define WIN_POS_Y	0

//! Breite des Fensters, Hoehe des Fensters
#define WIN_WIDTH	1280
#define WIN_HEIGHT	1024

/////////////////////////////////////////////////////////////////////////////////
//	OpenGL Darstellungsmodus
/////////////////////////////////////////////////////////////////////////////////
//! Default OpenGL Modus:  RGBA mit double Buffering und depth buffer (Z-Buffer)
//! Verwendung der Stencil und Accumulation Buffers, falls erforderlich
#if defined NEED_STENCIL && defined NEED_ACCUM
#define USED_MODUS	GLUT_RGBA |  GLUT_DEPTH | GLUT_DOUBLE | GLUT_STENCIL | GLUT_ACCUM
#elif defined NEED_STENCIL
#define USED_MODUS	GLUT_RGBA |  GLUT_DEPTH | GLUT_DOUBLE | GLUT_STENCIL
#elif defined NEED_ACCUM
#define USED_MODUS	GLUT_RGBA |  GLUT_DEPTH | GLUT_DOUBLE | GLUT_ACCUM
#else
#define USED_MODUS	GLUT_RGBA |  GLUT_DEPTH | GLUT_DOUBLE
#endif

/////////////////////////////////////////////////////////////////////////////////
//	Menuelemente des Kontextmenues (rechte Maustaste)
/////////////////////////////////////////////////////////////////////////////////
#define MENU_TEXT_WIREFRAME		"Wireframe on/off"
#define MENU_TEXT_SHADE			"Lighting/Shading"
#define MENU_TEXT_NO_NORMALS		"Lighting: set No Normals"
#define MENU_TEXT_PER_SURFACE_NORMALS "Lighting: set normals per surface"
#define MENU_TEXT_PER_VERTEX_NORMALS "Lighting: set normals per vertex"
#define MENU_TEXT_EXIT			"Exit"

enum MENU_IDs {
	ID_MENU_WIREFRAME=1,
	ID_MENU_SHADE,
	ID_MENU_NO_NORMALS,
	ID_MENU_PER_SURFACE_NORMALS,
	ID_MENU_PER_VERTEX_NORMALS,
	ID_MENU_EXIT
};


/////////////////////////////////////////////////////////////////////////////////
//	Konstanten und Variablen
/////////////////////////////////////////////////////////////////////////////////
#define PROG_NAME	"PrakCG-Template"

int MainWin;		// Identifier vom Hauptfenster
int MainMenu;		// Identifier vom Hauptmenu

//! Struktur fuer Bearbeitung der Mausevents
struct MouseStruct {
	int LastState;
	int OldX;
	int OldY;
	int ScreenX;
	int ScreenY;
	int MoveX;
	int MoveY;
	double Radius;
} globMouse;

//! Struktur für den Status einzelner Tasten (0: Up, 1: Down)
struct Keys {
  int keys[256];
  Keys() { // Initialisierung
	  for(int i=0; i<256; i++) keys[i]=0;
  }
  void KeyDown(unsigned char k) { // Taste k wurde gedrückt
	  keys[k]=1;
  }
  void KeyUp  (unsigned char k) { // Taste k wurde losgelassen
	  keys[k]=0;
  }
  int KeyState(unsigned char k) { // Abfrage Status Taste k
	  return keys[k];
  }
} globKeys;

/////////////////////////////////////////////////////////////////////////////////
//	CALLBACK Funktionen
/////////////////////////////////////////////////////////////////////////////////
void MenuFunc (int);				// Menue an Maustaste

void ReshapeFunc (int,int);			// Neuzeichnen der Szene
void MouseFunc (int,int,int,int);	// Maustasten
void MouseMove (int,int);			// Mausbewegung
void KeyboardDownFunc (unsigned char,int,int);	// Tastendruck
void KeyboardUpFunc (unsigned char,int,int);	// Taste loslassen
void SpecialKeyboardFunc (int,int,int);	// Sonderzeichen
void IdleFunc (void);				// Idle

/////////////////////////////////////////////////////////////////////////////////
//	Modellierungsfunktionen
/////////////////////////////////////////////////////////////////////////////////
// Material setzen (nur fuer Beleuchtungsmodus)
void SetMaterial (GLenum face, GLfloat amb[4], GLfloat diff[4], GLfloat spec[4], GLfloat shine, GLfloat emis[4]);
// Parameter einer Lichtquelle setzen (nur fuer Beleuchtungsmodus)
void SetLightColors (GLenum lightid, GLfloat amb[4], GLfloat diff[4], GLfloat spec[4]);
// Lichtquellen setzen
void SetLights(void); 
// Kamera platzieren, siehe Maus-Callbacks
void SetCamera(void);				

void DisplayFunc (void);			// GL-Displayfunktion
void Draw_Scene(void);				// Zeichnet die Szene im Weltkoordinatensystem
void Process(void);					// Animationsfunktion

/////////////////////////////////////////////////////////////////////////////////
//	Globale Variablen und Instanzen
/////////////////////////////////////////////////////////////////////////////////
float g_campos[3];			// Globale Kamerapostion (Weltkoordinaten)
GLenum pmode = GL_FILL;		// Zeichenmodus für Flächen {GL_LINE || GL_FILL}
bool lmode = GL_TRUE;    	// Beleuchtung an / aus
bool cullface = false;		// Backface-Culling an / aus
cg_help myhelp;				// Hilfe-Instanz
object3D *g_obj;			// allgemeines Wavefront-Objekt für VBO-Übung

int normal_mode = 0;		// Zeichenmodus: no normals, per surface, per vertex

/////////////////////////////////////////////////////////////////////////////////
//	Resize CALLBACK Funktion - wird aufgerufen, wenn sich die 
//	Fenstergroesse aendert
//		w,h: neue Breite und Hoehe des Fensters
/////////////////////////////////////////////////////////////////////////////////
void ReshapeFunc (int w, int h) {
	glMatrixMode (GL_PROJECTION);
		glLoadIdentity ();
		gluPerspective (45, (double)w/(double)h, 1, 2000);
		glViewport (0, 0, w, h);
	glMatrixMode (GL_MODELVIEW);	// Sicherheit halber 
	glLoadIdentity ();				// Modelierungsmatrix einstellen
	globMouse.ScreenX = w;			
	globMouse.ScreenY = h;
}

/////////////////////////////////////////////////////////////////////////////////
//	Maus Button CALLBACK Funktion
//		button - Welche Taste betaetigt bzw. losgelassen wurde
//		state  - Status der State (GL_DOWN, GL_UP)
//		x, y   - Fensterkoordinaten des Mauszeigers
/////////////////////////////////////////////////////////////////////////////////
void MouseFunc (int button,int state,int x,int y) {
	globMouse.LastState = (state == GLUT_DOWN) ? button : -1;
	switch (button) {
		// Linke Taste: Kamerabewegung
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_DOWN) {
				globMouse.LastState = state;
				globMouse.OldX = x;
				globMouse.OldY = y;
				}
			if (state == GLUT_UP)
				globMouse.LastState = -1;
			break;
		// Mittlere Taste: Zoom
		case GLUT_MIDDLE_BUTTON:
			if (state == GLUT_DOWN)
				globMouse.OldY = y;
			break;
		// Rechte Taste: Mit Kontextmenue belegt
		case GLUT_RIGHT_BUTTON:
			break;
		}
		glutPostRedisplay();
}

/////////////////////////////////////////////////////////////////////////////////
//	Maus Movement CALLBACK Funktion, wenn das Mausbutton gedrueckt
//		wurde. Fuer reine Mausbewegung muss man GlutPassiveMotionFunc
//		einstellen.
/////////////////////////////////////////////////////////////////////////////////
void MouseMove (int x,int y) {
	switch (globMouse.LastState) {
		case GLUT_LEFT_BUTTON:
			globMouse.MoveX += x - globMouse.OldX;
			globMouse.MoveY += y - globMouse.OldY;
			globMouse.OldX = x;
			globMouse.OldY = y;
			glutPostRedisplay ();
			break;
		case GLUT_MIDDLE_BUTTON:
			globMouse.Radius += y - globMouse.OldY;
			globMouse.Radius = MAX (globMouse.Radius,1);
			globMouse.OldX = x;
			globMouse.OldY = y;
			glutPostRedisplay ();
			break;
		case GLUT_RIGHT_BUTTON:
			break;
		}			
}

/////////////////////////////////////////////////////////////////////////////////
//	CALLBACK Funktion fuer Idle: wird aufgerufen, Standard-Callback
/////////////////////////////////////////////////////////////////////////////////
void IdleFunc () {
	glutPostRedisplay();
}

/////////////////////////////////////////////////////////////////////////////////
//	Menu CALLBACK Funktion
/////////////////////////////////////////////////////////////////////////////////
void MenuFunc (int Item) {
	switch (Item) {
		case ID_MENU_EXIT:
			exit (0);
			break;
		case ID_MENU_WIREFRAME:
			if (pmode == GL_FILL) 
				pmode = GL_LINE;
			else 
				pmode = GL_FILL;
			break;
		case ID_MENU_SHADE:
			if (lmode == GL_TRUE) 
				lmode = GL_FALSE;
			else 
				lmode = GL_TRUE;
			break;
		case ID_MENU_NO_NORMALS:
			  normal_mode = 0;
			break;
		case ID_MENU_PER_SURFACE_NORMALS:
			  normal_mode = 1;
			break;
		case ID_MENU_PER_VERTEX_NORMALS:
			  normal_mode = 2;
			break;
		}
}


/////////////////////////////////////////////////////////////////////////////////
//	Tastatur CALLBACK Funktion
//		key: Wert der Taste die gedrueckt wurde
//		x,y: Position des Mauskursors auf dem Viewport
/////////////////////////////////////////////////////////////////////////////////
void SpecialKeyboardFunc (int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_UP:
			break;
		case GLUT_KEY_DOWN:
			break;
		case GLUT_KEY_LEFT:
			break;
		case GLUT_KEY_RIGHT:
			break;
		case GLUT_KEY_F1:
			break;
		}
	glutPostRedisplay ();
}

/////////////////////////////////////////////////////////////////////////////////
//	Tastatur CALLBACK Funktion (Taste gedrückt)
//		key: Wert der Taste die gedrueckt wurde
//		x,y: Position des Mauskursors auf dem Viewport
/////////////////////////////////////////////////////////////////////////////////
void KeyboardDownFunc (unsigned char key, int x, int y) {
	globKeys.KeyDown(key);
  // Tasten auswerten, bei denen lediglich das einmalige Betätigen eine Aktion auslösen soll
  // Achtung: das geht so nur mit glutIgnoreKeyRepeat(true) (siehe main())
	switch (key) {
		  case 27: exit (0); // Escape
		  case 'f':
		  case 'F': myhelp.togglefps();  break;		// Framecounter on/off	
		  case 'h':
		  case 'H': myhelp.toggle(); break;			// Hilfetext on/off
		  case 'k':
		  case 'K':	myhelp.toggle_koordsystem(); break;	// Koordinatensystem on/off
		  case 'w': 
		  case 'W': if (pmode == GL_FILL) pmode = GL_LINE; // Wireframe on/off
					else pmode = GL_FILL;
					break;
		  case 'l': 
		  case 'L': if (lmode == GL_TRUE) lmode = GL_FALSE;	// Beleuchtung on/off
					else lmode = GL_TRUE;
					break;
		  case 'c': 
		  case 'C': if (cullface == GL_TRUE) cullface = GL_FALSE; // Backfaceculling on/off
					else cullface = GL_TRUE;
					break;

		  case 'i':
		  case 'I':	globMouse.MoveX = 0;	// Initialisierung der Kamera
					globMouse.MoveY = 0;
					glutPostRedisplay ();
					break;
				}
}

/////////////////////////////////////////////////////////////////////////////////
//	Tastatur CALLBACK Funktion (Taste losgelassen)
//		key: Wert der Taste die losgelassen wurde
//		x,y: Position des Mauskursors auf dem Viewport
/////////////////////////////////////////////////////////////////////////////////
void KeyboardUpFunc (unsigned char key, int x, int y) {
	globKeys.KeyUp(key);
}

/////////////////////////////////////////////////////////////////////////////////
//	Anfang des OpenGL Programmes
/////////////////////////////////////////////////////////////////////////////////
int main (int argc,char **argv) {
	
	// Fensterinitialisierung
	glutInit (&argc,argv);
	glutInitWindowSize (WIN_WIDTH,WIN_HEIGHT);
	glutInitWindowPosition (WIN_POS_X,WIN_POS_Y);
	glutInitDisplayMode (USED_MODUS);
	MainWin = glutCreateWindow (PROG_NAME);

	// OpenGL Initialisierungen
	glEnable (GL_DEPTH_TEST);	// Z-Buffer aktivieren

	// Menue erzeugen
	MainMenu = glutCreateMenu (MenuFunc);
	glutAddMenuEntry (MENU_TEXT_WIREFRAME,ID_MENU_WIREFRAME);
	glutAddMenuEntry (MENU_TEXT_SHADE,ID_MENU_SHADE);
	glutAddMenuEntry (MENU_TEXT_NO_NORMALS,ID_MENU_NO_NORMALS);
	glutAddMenuEntry (MENU_TEXT_PER_SURFACE_NORMALS,ID_MENU_PER_SURFACE_NORMALS);
	glutAddMenuEntry (MENU_TEXT_PER_VERTEX_NORMALS,ID_MENU_PER_VERTEX_NORMALS);
	glutAddMenuEntry (MENU_TEXT_EXIT,ID_MENU_EXIT);
	glutAttachMenu (GLUT_RIGHT_BUTTON);		// Menue haengt an der rechten Maustaste

	// Callbackfunktionen binden
	glutDisplayFunc (DisplayFunc);
	glutReshapeFunc (ReshapeFunc);
	glutMouseFunc (MouseFunc);
	glutMotionFunc (MouseMove);
	glutKeyboardFunc  (KeyboardDownFunc);
	glutKeyboardUpFunc(KeyboardUpFunc);
	glutSpecialFunc (SpecialKeyboardFunc);
	glutIdleFunc (IdleFunc);
	glutIgnoreKeyRepeat(GL_TRUE);

	// Initiale Mauseigenschaften
	globMouse.LastState = -1;
	globMouse.MoveX 	= 60;
	globMouse.MoveY 	= 40;
	globMouse.Radius 	= 15;
#ifdef _WIN32 || _WIN64
	init_extensions();	// OpenGL Extensions laden
#endif 
	g_obj=loadobject(".\\scene.obj", false);	// Eine Wavefront-Szene laden

	// Die Hauptschleife
	glutMainLoop ();	
	return 0;
}



/////////////////////////////////////////////////////////////////////////////////
//	Modellierungsfunktionen
/////////////////////////////////////////////////////////////////////////////////

void SetCamera(void) {
	// Ansichtstransformationen setzen,
	// SetCamera() zum Beginn der Zeichenfunktion aufrufen
	double x,y,z, The,Phi;
	Phi = (double)globMouse.MoveX / (double)globMouse.ScreenX * M_PI * 2.0 + M_PI*0.5;
	The = (double)globMouse.MoveY / (double)globMouse.ScreenY * M_PI * 2.0;
	x = globMouse.Radius * cos (Phi) * cos (The);
	y = globMouse.Radius * sin (The);
	z = globMouse.Radius * sin (Phi) * cos (The);
	int Oben = (The <= 0.5 * M_PI || The > 1.5 * M_PI) * 2 - 1;
	gluLookAt (x,y,z, 0,0,0, 0, Oben, 0);
    g_campos[0]=x; g_campos[1]=y; g_campos[2]=z;
}

void SetMaterial (GLenum face, GLfloat amb[4], GLfloat diff[4], GLfloat spec[4], GLfloat shine, GLfloat emis[4])
	// Aktualisierung des OpenGL Materials 
{
  glMaterialfv (face, GL_AMBIENT, amb);
  glMaterialfv (face, GL_DIFFUSE, diff);
  glMaterialfv (face, GL_SPECULAR, spec);
  glMaterialf  (face, GL_SHININESS, shine);
  glMaterialfv (face, GL_EMISSION, emis);
}

void SetLightColors (GLenum lightid, GLfloat amb[4], GLfloat diff[4], GLfloat spec[4])
	// Aktualisierung der OpenGL Lichtquelle "lightid" (Farbwerte) 
{
  glLightfv (lightid, GL_AMBIENT, amb);
  glLightfv (lightid, GL_DIFFUSE, diff);
  glLightfv (lightid, GL_SPECULAR, spec);
  glLightf  (lightid, GL_CONSTANT_ATTENUATION,  1);
  glLightf  (lightid, GL_LINEAR_ATTENUATION,    0);
  glLightf  (lightid, GL_QUADRATIC_ATTENUATION, 0);
}

// schwarz und weiss
GLfloat g_black[4]={0,0,0,1};
GLfloat g_white[4]={1,1,1,1};

// Parameter eines globalen Lichts
GLint g_localviewer=GL_FALSE;
float g_amb[4] ={0.2f, 0.2f, 0.2f, 1.0f};
float l_diff[4]={1.0f, 1.0f, 1.0f, 1.0f};
float l_spec[4]={1.0f, 1.0f, 1.0f, 1.0f};
float l_pos[4] ={-1.0f, 1.0f, 1.0f, 0.0f}; // paralles Licht
//float l_pos[4] ={-5.0f, 5.0f, 5.0f, 1.0f}; // Punktlicht

// Parameter eines globalen Materials
float m_amb[4] ={1.0f, 0.0f, 1.0f, 1.0f};
float m_diff[4]={1.0f, 0.0f, 1.0f, 1.0f};
float m_spec[4]={1.0f, 1.0f, 1.0f, 1.0f};
float m_shine  =50.0f;
float m_emis[4]={0.0f, 0.0f, 0.0f, 1.0f};



void SetLights(void) {
// Standard-Lichtszene setzen  
  GLenum lightid=GL_LIGHT0;	  // Lichtquelle
  glLightfv (lightid, GL_POSITION, l_pos);

  GLfloat nodir[3]={0,0,0};	 
  glLightfv (lightid, GL_SPOT_DIRECTION, nodir);
  glLightf  (lightid, GL_SPOT_CUTOFF, 180.0f);
  glLightf  (lightid, GL_SPOT_EXPONENT, 0.0);

  SetLightColors (lightid, g_black, l_diff, l_spec);
  glEnable(lightid);

  glEnable(GL_LIGHTING);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, g_localviewer);
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, g_amb);
}

void DisplayFunc (void) {
    // Szene zeichnen: CLEAR, SETCAMERA, DRAW_SCENE
	
	// Back-Buffer neu initialisieren
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity (); 

	// Kamera setzen (spherische Mausnavigation)
	SetCamera();

	// Koordinatensystem zeichnen
	myhelp.draw_koordsystem (-8, 10, -8, 10, -8, 10);

	// Zeichenmodus einstellen (Wireframe on/off)
	glPolygonMode(GL_FRONT_AND_BACK, pmode);
	
	// Backface Culling on/off);
	glFrontFace(GL_CCW); glCullFace(GL_BACK);
	if (cullface) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);

	// Farbmodus oder Beleuchtungsmodus ?
	glEnable(GL_NORMALIZE);
	if (lmode == GL_TRUE) { // Beleuchtung aktivieren
		float m_amb[4] = {0.2, 0.2, 0.2, 1.0};
		float m_diff[4] = {0.2, 0.2, 0.6, 1.0};
		float m_spec[4] = {0.6, 0.6, 0.6, 1.0};
		float m_shine = 100.0;
		float m_emiss[4] = {0.0, 0.0, 0.0, 1.0};

		glEnable(GL_LIGHTING);

		SetLights();
		
		glEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT, GL_DIFFUSE); 
		SetMaterial(GL_FRONT_AND_BACK, m_amb, m_diff, m_spec, m_shine, m_emiss);
	}
	else { // Zeichnen im Farbmodus
		float m_col[4] = {0.2, 0.2, 0.6, 1.0};
		glDisable(GL_LIGHTING);
		glColor4fv(m_col);
	}
	
	// Geometrie zeichnen /////////////////!!!!!!!!!!!!!!!!!!!!!!!!///////////////////////
	Draw_Scene();

	// Hilfetext zeichnen
	myhelp.draw();

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); glDisable(GL_CULL_FACE);

	glFlush ();				// Daten an Server (fuer die Darstellung) schicken
	glutSwapBuffers();		// Buffers wechseln
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ÜBUNG 3 - Truck /////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
inline float* normieren (float v[3]) {
  float l=(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
  if(l==0) return v;
  l = 1.0f/sqrt(l);
  v[0]*=l; v[1]*=l; v[2]*=l;
  return v;
}

#define SLICES 20			// für Zylindermantel, Boden und Deckel
#define STACKS 1			// für Zylindermantel
#define LOOPS 1				// für Deckel

void myCylinder(void) {
	// Zeichnet einen Einheitszylinder 
	// d = 1, h = 1, RINGS, SLICES
	// Lage: Basiskreis in X-Y-Ebene, Höhe entlang der +Z Achse

	glPushMatrix();
		GLUquadricObj *q = gluNewQuadric();
			gluCylinder(q,0.5, 0.5, 1, SLICES, STACKS);
			glPushMatrix();
				glTranslatef(0,0,1);
				gluDisk(q, 0, 0.5, SLICES, LOOPS);
			glPopMatrix();
			glPushMatrix();
				glRotatef(180,0,1,0);
				//gluQuadricOrientation(q, GLU_INSIDE);
				gluDisk(q, 0, 0.5, SLICES, LOOPS);
			glPopMatrix();
		gluDeleteQuadric(q);
	glPopMatrix();

};

void myBox(void) {
	// zeichnet einen Einheitswürfel, Kantenlänge 1 Einheit, "kleinste Ecke" im Ursprung, 
	// Ausdehnung entlang der X, Y und Z-Achse
	glPushMatrix();
		glTranslatef(0.5, 0.5, 0.5);
		glutSolidCube(1);
	glPopMatrix();
};


// Eigenschaften der Truck-Geometrie:
float axis_width   = 7.0f;    // Breite der Achse
float wheel_radius = 2.0f;     // Radius der Räder
float front_axis   = 15.0f;    // Abstand vordere Achse zum Nullpunkt (auf Z)
float rear_axis1   = 2.5f;     // Abstand hintere Achse zum Nullpunkt (auf Z)
float rear_axis2   =-2.5f;     // Abstand hintere Achse zum Nullpunkt (auf Z)

// Struktur für den Truck
struct Truck {
  float speed;    // Geschwindigkeit [m/s]
  float steering; // Lenkeinschlag   [°]
  float pos[3];   // Position        [m]
  float rot[3];   // Rotation        [°]
  float wheelrot; // Rollposition der Räder [°]
  float tilt;	  // Kippwinkel
  Truck() 
  {
    wheelrot=0;
    speed=0; steering=0; 
    pos[0]=pos[1]=pos[2]=0;
    rot[0]=rot[1]=rot[2]=0;
	tilt = 0;
  }
} myTruck;

void myAxle(void) {
	// Achse zeichnen 
	// Lage auf der X-Achse, Mittelpunkt im Ursprung
	// Länge 6 Einheiten, Durchmesser 0.5 in X, 1 in Y-Richtung
	// Farbe (0.5, 0.5, 0.9);
	glPushMatrix();
		glRotatef(90, 0, 1, 0);
		glScalef(0.5,1,axis_width);
		glTranslatef(0, 0, -0.5);	
		glColor3f(0.5, 0.5, 0.9); 
		myCylinder();
	glPopMatrix();
};

void myWheel(void) {
	// Zeichnet ein Rad 
	// Durchmesser Rad = 4, Radkappe = 3
	// Höhe Rad = 2, Radkappe = 0.2 (Radkappe liegt von 1<=Z<=1.2);
	// Farbe Rad = 30% Grau, Radkappe = Gelblich (0.8, 0.8, 0.2);
	
	glPushMatrix();
		glScalef(2*wheel_radius,2*wheel_radius,wheel_radius);
		glColor3f(0.3, 0.3, 0.3);
		myCylinder();
	glPopMatrix();
	
	glPushMatrix();
		glTranslatef(0,0,2);
		glScalef(3, 3, 0.1);
		glColor3f(0.8, 0.8, 0.2);
		myCylinder();
	glPopMatrix();
}

void myFrontAxle (float wheelrot, float steering) {
	// die Vorderachse
 
	myAxle();

	// rechtes Rad bei WKS: X = 3 anfügen 	
	glPushMatrix();
		glRotatef(90, 0, 1, 0);
		glTranslatef(0,0,axis_width/2.0);
		glRotatef(steering, 0,1,0);
		glRotatef(wheelrot, 0,0,1);
		myWheel();
	glPopMatrix();

	// linkes Rad bei WKS: X = 3 anfügen 	
	glPushMatrix();
		glRotatef(-90, 0, 1, 0);
		glTranslatef(0,0,axis_width/2.0);
		glRotatef(steering, 0,1,0);
		glRotatef(wheelrot, 0,0,-1);
		myWheel();
	glPopMatrix();

	
}; 

void myRearAxle(float wheelrot) {
	// eine komplette Hinterachse

	myAxle();

	// rechtes Rad bei WKS: X = 3 anfügen 	
	glPushMatrix();
		glRotatef(90, 0, 1, 0);
		glTranslatef(0,0,axis_width/2.0);
		glRotatef(wheelrot, 0,0,1);
		myWheel();
	glPopMatrix();

	// linkes Rad bei WKS: X = 3 anfügen 	
	glPushMatrix();
		glRotatef(-90, 0, 1, 0);
		glTranslatef(0,0,axis_width/2.0);
		glRotatef(wheelrot, 0,0,-1);
		myWheel();
	glPopMatrix();

};

void Draw_Truck(Truck myTruck) {

	
		// Fahrwerk mit Rahmen
		{
			glColor3f(0.8, 0.8, 0.5);
			glPushMatrix();					    // Rahmen
				glTranslatef(-axis_width/2.0 + 1, 0, -7);
				glScalef(axis_width - 2, 2, 25);
				myBox();
			glPopMatrix();
			
			glPushMatrix();						 // Vorderachse
				glTranslatef(0,0,front_axis);	
				myFrontAxle(myTruck.wheelrot, myTruck.steering);
			glPopMatrix();

			glPushMatrix();						// Mittlere Achse
				glTranslatef(0,0,rear_axis1);
				myRearAxle(myTruck.wheelrot);
			glPopMatrix();
			
			glPushMatrix();						// Hintere Achse
				glTranslatef(0,0,rear_axis2);
				myRearAxle(myTruck.wheelrot);
			glPopMatrix();
		}

		/////////////////////////////////////////////////////////////////////////////////
		// TODO: Hier die Fahrzeugaufbauten durch Aufruf der Funktion myBox() ergänzen //
		/////////////////////////////////////////////////////////////////////////////////

		

}

void Draw_Ground(void) {  
	// Zeichnet den Boden bei Y=0

	GLUquadricObj *q = gluNewQuadric();
		glColor3f(0.2, 0.2, 0.3);

		glPushMatrix();
			glRotatef(90,1,0,0);
			gluDisk(q, 0, 300, 100, 50);
		glPopMatrix();

	gluDeleteQuadric(q);

};


void Draw_Scene (void) {
	// Zeichnet die komplette Szene mit Boden und Truck

	Process();

	Draw_Ground();						// der Boden

	glTranslatef(0, wheel_radius, 0);	// den Truck "auf die Straße stellen"
	

	// TODO: Den Truck laut seinen Animationsparametern positionieren und ausrichten
	// myTruck.pos[], myTruck.rot[];
	Draw_Truck(myTruck);				

}



void Process(void)
	// Berechnung der Animationsparameter aus alten Parametern und der Reaktion auf Eingaben
{
#pragma region PROCESS_INPUT
  // -------------------------------  INPUT  --------------------------------------------
  // Input-Verarbeitung: (5/2/1/3-Steuerung, 9,6 für den Kipper)

  // Geschwindigkeit: Tasten 5 und 2
  myTruck.speed *= 0.995f;								// Reibung
  if(globKeys.KeyState('5'))  myTruck.speed += 0.25f;	// Beschleunigen
  if(globKeys.KeyState('2'))  myTruck.speed -= 0.25f;	// Bremsen
  if(globKeys.KeyState(' '))  myTruck.speed *= 0.9f;	// Handbremse
 
  // Lenkeinschlag: Tasten 1 und 3
  if(fabs(myTruck.speed)>0.1f)                           // Lenkrückstellung sollte abhängig von
    myTruck.steering *= 0.95f;                           // der (absoluten) Geschwindigkeit sein
  if(globKeys.KeyState('1'))     myTruck.steering += 2.0f;  // Links lenken
  if(globKeys.KeyState('3'))     myTruck.steering -= 2.0f;  // Rechts lenken

  // TODO Kippen: Tasten 6 und 9
  // 
  // if (???) myTruck.tilt = ????


  // Begrenzungen der Animationsparameter:
  if(myTruck.speed> 30.0f)       myTruck.speed= 30.0f;   // Maximal 30m/s vorwärts
  if(myTruck.speed< -5.0f)       myTruck.speed= -5.0f;   // Maximal 5m/s rückwärts
  if(myTruck.steering> 30.0f)    myTruck.steering= 30.0f;   // Maximal 65° Lenkeinschlag
  if(myTruck.steering<-30.0f)    myTruck.steering=-30.0f;   // Maximal -65° Lenkeinschlag
  // TODO Begrenzung des Kippers
  // if (myTruck.tilt ???) ...

#pragma endregion
#pragma region PROCESS_TRUCK
  // -------------------------------  TRUCK bewegen --------------------------------------------
  // Geschwindigkeit in m/frame, berechnet aus speed in m/s
  float speed_perframe = myTruck.speed/60.0f;

  // TODO: Berechnung des Drehwinkels für die Räder: myTruck.wheelrot
  // Wie weit haben sich die Räder weitergedreht?
  // Die Geschwindigkeit beträgt speed_perframe in m/frame, der Radius der Räder ist wheel_radius
  // mytruck.wheelrot = .... ???


  // Truck-Position und Rotation neu berechnen: Rotation ist nur um die Y-Achse interessant
  float alpha     = (myTruck.rot[1]+myTruck.steering)*M_PI/180.0f;
  float alpha_old =  myTruck.rot[1]                  *M_PI/180.0f;
  float vec[3]     = {sin(alpha),    0,cos(alpha)};     // neuer Bewegungsvektor der Vorderachse
  float vec_old[3] = {sin(alpha_old),0,cos(alpha_old)}; // alter Richtungsvektor
 
  float front_point_old[3] = {vec_old[0]*front_axis,    // Mittelpunkt der Vorderachse
                              vec_old[1]*front_axis,
                              vec_old[2]*front_axis};
  float front_point_new[3] = {front_point_old[0]+speed_perframe*vec[0], // neuer Mittelpunkt der Vorderachse
                              front_point_old[1]+speed_perframe*vec[1],
                              front_point_old[2]+speed_perframe*vec[2]};

  // Nun Mittelpunkt der Hinterachse (hier (0,0,0)!) neu berechnen. 
  // Dieser entspricht dann der gesuchten Fahrzeugbewegung.
  // Er liegt auf dem Vektor V, der vom alten Mittelpunkt(0,0,0) zum neuen Vorderachsenpunkt zeigt

  float V[3] = {front_point_new[0],
                front_point_new[1],
                front_point_new[2]};
  normieren(V);

  float pos[3] = {front_point_new[0]-V[0]*front_axis,
                  front_point_new[1]-V[1]*front_axis,
                  front_point_new[2]-V[2]*front_axis};

  float alpha_new = asin(V[0])*180.0f/M_PI; 
  if(V[2]<0) alpha_new = 180.0f-alpha_new;

  while(alpha_new>360.0f) alpha_new-=360.0f;
  while(alpha_new<0.0f)   alpha_new+=360.0f;
  myTruck.rot[1]=alpha_new;

  myTruck.pos[0] += pos[0];
  myTruck.pos[1] += pos[1];
  myTruck.pos[2] += pos[2];
#pragma endregion
}