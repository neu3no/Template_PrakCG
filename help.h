// *** Help-Module, PrakCG Template

#ifdef __EXPORT_HELP

char *spalte1[]={
	"Maus",
	"",
	"linke Taste      Kamerabewegung",
	"mittlere Taste   Zoom",
	"rechte Taste     Kontextmenü",

  NULL };
char *spalte2[]={
  "Tastatur:",
  "",
  "f,F   - Framerate (An/Aus)",
  "l,L   - Licht global (An/Aus)",
  "h,H   - Hilfe (An/Aus)",
  "w,W   - WireFrame (An/Aus)",
  "k,K   - Koordinatensystem (An/Aus)",
   "","","","",
   "ESC   - Beenden", 

  NULL };
#endif
  
class cg_help {
  bool  showhelp,showfps,wireframe,koordsystem;
  int   frames;
  float fps,bg_size,shadow;
  char  *title;
  void draw_background();
  void printtext (float x, float y, char *text, void *font=GLUT_BITMAP_HELVETICA_18);
  void printtext (float x, float y, char *text, float r, float g, float b, void *font=GLUT_BITMAP_HELVETICA_18);
  void printtext_shadow (float x, float y, char *text, float r, float g, float b, void *font=GLUT_BITMAP_HELVETICA_18);
  void printfps (float x, float y, void *font=GLUT_BITMAP_HELVETICA_18);
public:
  cg_help();
  void  toggle(void);
  void  togglefps(void);
  void  set_title(char *t); 
  void  set_wireframe(bool wf);
  bool  is_wireframe(void);
  void  toggle_koordsystem(void);
  bool  is_koordsystem(void);
  float get_fps(void);
  void  draw(void);
  void  draw_koordsystem (GLfloat xmin, GLfloat xmax, GLfloat ymin, GLfloat ymax, GLfloat zmin, GLfloat zmax);
};

