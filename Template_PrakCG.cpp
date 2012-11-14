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
// ÜBUNG 2 - ACHSE /////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
#define SLICES 20			// für Zylindermantel, Boden und Deckel
#define STACKS 1			// für Zylindermantel
#define LOOPS 1				// für Deckel

void myCylinder(void) {
	// Zeichnet einen Einheitszylinder ins WKS
	// d = 1, h = 1, Unterteilung: RINGS, SLICES
	// Lage: Basiskreis in X-Y-Ebene, Höhe entlang der +Z Achse

	GLUquadricObj *q = gluNewQuadric();

		// Mantelfläche
		glColor3f(0.2, 0.2, 0.6);
		gluCylinder(q,0.5, 0.5, 1, SLICES, STACKS);

		// Boden ergänzen --> gluDisk(...)

		// Deckel ergänzen --> gluDisk(...)

	gluDeleteQuadric(q);

};

void myAxle(void) {
	// Achse zeichnen 
	// Lage auf der X-Achse, Mittelpunkt im Ursprung
	// Länge 6 Einheiten, Durchmesser 0.5 in X, 1 in Y-Richtung
	// Farbe (0.5, 0.5, 0.9);

	// Transformationen einstellen


	// myCylinder zeichnen
	   glColor3f(0.5, 0.5, 0.9);
	   myCylinder();

	// Transformationen rückgängig machen

};

void myWheel(void) {
	// Zeichnet ein Rad 
	// Durchmesser Rad = 4, Radkappe = 3
	// Höhe Reifen = 2, Radkappe = 0.2 (Radkappe liegt von 1<=Z<=1.2);
	// Farbe Rad = 30% Grau, Radkappe = Gelblich (0.8, 0.8, 0.2);
	
	// Transformationen einstellen + Reifen zeichnen

	    glColor3f(0.3, 0.3, 0.3);
		myCylinder();

	
	// Transformationen einstellen + Radkappe zeichnen	
		glColor3f(0.8, 0.8, 0.2);
		myCylinder();

	// Transformationen rückgängig machen

}

void Draw_Scene (void) {
  
	// Testen der Funktionen 
	myCylinder();


	// Animationsparameter berechnen
	static float wheelrot = 0;
	wheelrot += 0.5; 
	if (wheelrot > 360.0) wheelrot = 0;

	static float steering = 0.0;
	// Berechnung des Lenkeinschlags


	// Achse komplett zeichnen:

	// Achse platzieren und zeichnen

	   // myAxle();

	// rechtes Rad platzieren, drehen und zeichnen

	 // myWheel();


	// linkes Rad platzieren, drehen und zeichnen

	 // myWheel();


}; // Draw_Scene()
