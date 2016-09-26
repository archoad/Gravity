/*universe3d
Copyright (C) 2013 Michel Dubois

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <png.h>

#include <GL/freeglut.h>

#define WINDOW_TITLE_PREFIX "Universe simulation"
#define couleur(param) printf("\033[%sm",param)
#define BUFSIZE 512
#define MAXPLANET 500000

static short winSizeW = 920,
	winSizeH = 690,
	frame = 0,
	currentTime = 0,
	timebase = 0,
	fullScreen = 0,
	trace = 0,
	allTraces = 0,
	rotate = 0,
	axe = 1,
	concentration = 12,
	dt = 5; // in milliseconds

static int textList = 0,
	cpt = 0,
	background = 0,
	pathLength = 0,
	sampleSize = 1000;

static float fps = 0.0,
	rotx = -80.0,
	roty = 0.0,
	rotz = 20.0,
	xx = 0.0,
	yy = 5.0,
	zoom = 300.0,
	prevx = 0.0,
	prevy = 0.0;

typedef struct _vector {
	double x, y, z;
} vector;

typedef struct _planet {
	vector pos;
	vector color;
	vector velocity;
	vector force;
	vector *path;
	double radius;
	double mass;
	int id;
	short selected;
	short displayed;
} planet;


static planet planetsList[MAXPLANET];

static double maxWeight = 2.0e9,
	minWeight = 1.0e2,
	density = 1.0e8,
	pi = 3.14159265358979323846,
	g = 6.67428e-11;




void usage(void) {
	couleur("31");
	printf("Michel Dubois -- universe3d -- (c) 2013\n\n");
	couleur("0");
	printf("Syntaxe: universe3d <background color>\n");
	printf("\t<background color> -> 'white' or 'black'\n");
}


void help(void) {
	couleur("31");
	printf("Michel Dubois -- universe3d -- (c) 2013\n\n");
	couleur("0");
	printf("Key usage:\n");
	printf("\t'ESC' key to quit\n");
	printf("\t'UP', 'DOWN', 'LEFT' and 'RIGHT' keys to rotate manually\n");
	printf("\t'r' to rotate continuously\n");
	printf("\t'x' and 'X' to move to right and left\n");
	printf("\t'y' and 'Y' to move to top and bottom\n");
	printf("\t'z' and 'Z' to zoom in or out\n");
	printf("\t'f' to switch to full screen\n");
	printf("\t'p' to take a screenshot\n");
	printf("\t'd' to display axe or not\n");
	printf("\t't' to display selected planet trace or not\n");
	printf("\t'a' to display trace of all planets\n");
	printf("Mouse usage:\n");
	printf("\t'LEFT CLICK' to to select a planet\n");
	printf("\n");
}


void takeScreenshot(char *filename) {
	FILE *fp = fopen(filename, "wb");
	int width = glutGet(GLUT_WINDOW_WIDTH);
	int height = glutGet(GLUT_WINDOW_HEIGHT);
	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info = png_create_info_struct(png);
	unsigned char *buffer = calloc((width * height * 3), sizeof(unsigned char));
	int i;

	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *)buffer);
	png_init_io(png, fp);
	png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png, info);
	for (i=0; i<height; i++) {
		png_write_row(png, &(buffer[3*width*((height-1) - i)]));
	}
	png_write_end(png, NULL);
	png_destroy_write_struct(&png, &info);
	free(buffer);
	fclose(fp);
	printf("INFO: Save screenshot on %s (%d x %d)\n", filename, width, height);
}


char* displayPlanet(planet p, int simple) {
	char *text = NULL;
	text = calloc(120, sizeof(text));
	if (simple) {
		sprintf(text, "[%d] coord: (%.2f, %.2f, %.2f) mass: %.6f radius: %.6f", p.id, p.pos.x, p.pos.y, p.pos.z, p.mass, p.radius);
	} else {
		sprintf(text, "[%d] \t coord: (%.2f, %.2f, %.2f) \t color: (%.2f, %.2f, %.2f) \t velocity: (%.2f, %.2f, %.2f) \t mass: %.6f \t radius: %.6f\n", p.id, p.pos.x, p.pos.y, p.pos.z, p.color.x, p.color.y, p.color.z, p.velocity.x, p.velocity.y, p.velocity.z, p.mass, p.radius);
	}
	return(text);
}


void drawPlanet(planet p, int name) {
	glPushMatrix();
	glTranslatef(p.pos.x, p.pos.y, p.pos.z);
	if (p.selected) {
		glColor3f(1.0, 0.0, 0.0);
		glutWireCube(p.radius * 2.0);
	}
	glColor3f(p.color.x, p.color.y, p.color.z);
	glLoadName(name);
	//glPointSize(5*p.radius);
	//glBegin(GL_POINTS);
		//glVertex3f(p.pos.x, p.pos.y, p.pos.z);
	//glEnd();
	glutSolidSphere(p.radius, 6, 6);
	glPopMatrix();
}


void drawPath(planet p) {
	int pos = 0;
	glPointSize(0.5f);
	glColor3f(p.color.x, p.color.y, p.color.z);
	for (pos=0; pos<pathLength; pos++) {
		glBegin(GL_POINTS);
			glVertex3f(p.path[pos].x, p.path[pos].y, p.path[pos].z);
		glEnd();
	}
}


void drawString(float x, float y, float z, char *text) {
	unsigned i = 0;
	glPushMatrix();
	glLineWidth(1.0);
	if (background){ // White background
		glColor3f(0.0, 0.0, 0.0);
	} else { // Black background
		glColor3f(1.0, 1.0, 1.0);
	}
	glTranslatef(x, y, z);
	glScalef(0.008, 0.008, 0.008);
	for(i=0; i < strlen(text); i++) {
		glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, (int)text[i]);
	}
	glPopMatrix();
}


void drawText(void) {
	int i = 0;
	char text1[50], text2[70], text3[120];
	sprintf(text1, "Nbr of planets: %d", sampleSize);
	sprintf(text2, "dt: %1.3f, FPS: %4.2f", (dt/1000.0), fps);
	for (i=0; i<sampleSize; i++) {
		if (planetsList[i].selected) {
			sprintf(text3, "%s", displayPlanet(planetsList[i], 1));
		}
	}
	textList = glGenLists(1);
	glNewList(textList, GL_COMPILE);
	drawString(-40.0, -36.0, -100.0, text1);
	drawString(-40.0, -38.0, -100.0, text2);
	drawString(-40.0, -40.0, -100.0, text3);
	glEndList();
}


void drawAxes(void) {
	glPushMatrix();
	glLineWidth(1.0);
	glTranslatef(0.0, 0.0, 0.0);
	glColor3f(1.0, 1.0, 1.0);
	glutWireCube(100.0/2.0);
	glColor3f(0.6, 0.6, 0.6);
	glutWireCube(100.0*2.0);
	glPopMatrix();
}


void display(void) {
	int i=0;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	drawText();
	glCallList(textList);

	glPushMatrix();
	glTranslatef(xx, yy, -zoom);
	glRotatef(rotx, 1.0, 0.0, 0.0);
	glRotatef(roty, 0.0, 1.0, 0.0);
	glRotatef(rotz, 0.0, 0.0, 1.0);

	GLfloat ambient1[] = {0.15f, 0.15f, 0.15f, 1.0f};
	GLfloat diffuse1[] = {0.8f, 0.8f, 0.8f, 1.0f};
	GLfloat specular1[] = {1.0f, 1.0f, 1.0f, 1.0f};
	GLfloat position1[] = {0.0f, 0.0f, 20.0f, 1.0f};
	glLightfv(GL_LIGHT1, GL_AMBIENT, ambient1);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse1);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, specular1);
	glLightfv(GL_LIGHT1, GL_POSITION, position1);
	glEnable(GL_LIGHT1);

	if (axe) { drawAxes(); }
	for (i=0; i<sampleSize; i++) {
		if (planetsList[i].displayed) {
			drawPlanet(planetsList[i], i);
		}
		if (trace) {
			if (planetsList[i].selected) {
				drawPath(planetsList[i]);
			}
		}
		if (allTraces) {
			drawPath(planetsList[i]);
		}
	}
	glPopMatrix();

	glutSwapBuffers();
	glutPostRedisplay();
}


void processSelectedObject(GLint hitsNumber, GLuint *selectBuffer) {
	int objectName = 0;
	if (hitsNumber == 1) {
		objectName = selectBuffer[3];
		planetsList[objectName].selected = !planetsList[objectName].selected;
		printf("INFO: Touched -> %s", displayPlanet(planetsList[objectName], 0));
	}
}


void selectObject(x, y) {
	GLuint selectBuffer[BUFSIZE];
	GLint hitsNumber;
	GLint viewPort[4];

	glGetIntegerv(GL_VIEWPORT, viewPort);
	glSelectBuffer(BUFSIZE, selectBuffer);
	(void) glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(0);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glGetIntegerv(GL_VIEWPORT, viewPort);
	gluPickMatrix(x, y, 1.0, 1.0, viewPort);
	gluPerspective(45.0, 1.0, 1.0, 1000.0);
	glMatrixMode(GL_MODELVIEW);
	glutSwapBuffers();
	display();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	hitsNumber = glRenderMode(GL_RENDER);
	glFlush();
	glutPostRedisplay();
	processSelectedObject(hitsNumber, selectBuffer);
}


void onReshape(int width, int height) {
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, width/height, 1.0, 1000.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


void onSpecial(int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_UP:
			rotx += 5.0;
			printf("INFO: x = %f\n", rotx);
			break;
		case GLUT_KEY_DOWN:
			rotx -= 5.0;
			printf("INFO: x = %f\n", rotx);
			break;
		case GLUT_KEY_LEFT:
			rotz += 5.0;
			printf("INFO: z = %f\n", rotz);
			break;
		case GLUT_KEY_RIGHT:
			rotz -= 5.0;
			printf("INFO: z = %f\n", rotz);
			break;
		default:
			printf("x %d, y %d\n", x, y);
			break;
	}
	glutPostRedisplay();
}


void onMotion(int x, int y) {
	if (prevx) {
		xx += ((x - prevx)/10.0);
		printf("INFO: x = %f\n", xx);
	}
	if (prevy) {
		yy -= ((y - prevy)/10.0);
		printf("INFO: y = %f\n", yy);
	}
	prevx = x;
	prevy = y;
	glutPostRedisplay();
}


void onIdle(void) {
	frame += 1;
	currentTime = glutGet(GLUT_ELAPSED_TIME);
	if (currentTime - timebase >= 1000.0){
		fps = frame*1000.0 / (currentTime-timebase);
		timebase = currentTime;
		frame = 0;
	}
	glutPostRedisplay();
}


void onMouse(int button, int state, int x, int y) {
	switch (button) {
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_DOWN) {
				printf("INFO: left button, x %d, y %d\n", x, y);
				selectObject(x, winSizeH-y);
			}
			break;
		case GLUT_RIGHT_BUTTON:
			if (state == GLUT_DOWN) {
				printf("INFO: right button, x %d, y %d\n", x, y);
			}
			break;
	}
}


void onKeyboard(unsigned char key, int x, int y) {
	char *name = malloc(20 * sizeof(char));
	int i = 0;
	switch (key) {
		case 27: // Escape
			printf("x %d, y %d\n", x, y);
			printf("INFO: exit loop\n");
			glutLeaveMainLoop();
			break;
		case 'x':
			xx += 1.0;
			printf("INFO: x = %f\n", xx);
			break;
		case 'X':
			xx -= 1.0;
			printf("INFO: x = %f\n", xx);
			break;
		case 'y':
			yy += 1.0;
			printf("INFO: y = %f\n", yy);
			break;
		case 'Y':
			yy -= 1.0;
			printf("INFO: y = %f\n", yy);
			break;
		case 'f':
			fullScreen = !fullScreen;
			if (fullScreen) {
				glutFullScreen();
			} else {
				glutReshapeWindow(winSizeW, winSizeH);
				glutPositionWindow(100,100);
				printf("INFO: fullscreen %d\n", fullScreen);
			}
			break;
		case 'd':
			axe = !axe;
			printf("INFO: axe = %d\n", axe);
			break;
		case 'r':
			rotate = !rotate;
			printf("INFO: rotate %d\n", rotate);
			break;
		case 't':
			trace = !trace;
			printf("INFO: trace = %d\n", trace);
			break;
		case 'a':
			allTraces = !allTraces;
			for (i=0; i<sampleSize; i++) {
				planetsList[i].displayed = !allTraces;
			}
			printf("INFO: all traces = %d\n", allTraces);
			break;
		case 'z':
			zoom -= 5.0;
			if (zoom < 5.0) {
				zoom = 5.0;
			}
			printf("INFO: zoom = %f\n", zoom);
			break;
		case 'Z':
			zoom += 5.0;
			printf("INFO: zoom = %f\n", zoom);
			break;
		case 'p':
			printf("INFO: take a screenshot\n");
			sprintf(name, "capture_%.3d.png", cpt);
			takeScreenshot(name);
			cpt += 1;
			break;
		default:
			break;
	}
	free(name);
	glutPostRedisplay();
}


void onTimer(int event) {
	switch (event) {
		case 0:
			break;
		default:
			break;
	}
	if (rotate) {
		rotz -= 0.2;
	} else {
		rotz += 0.0;
	}
	if (rotz > 360) rotz = 360;
	glutPostRedisplay();
	glutTimerFunc(dt, onTimer, 0);
}


void update(int value) {
	/* source: https://en.wikipedia.org/wiki/Kepler%27s_laws_of_planetary_motion
		paragraph: Newton's law of gravitation
	*/

	int p1=0, p2=0;
	double dx=0, dy=0, dz=0, d=0, f=0;
	double radiusSum=0;
	vector a;

	prevx = 0.0;
	prevy = 0.0;
	pathLength ++;
	for (p1=0; p1<value; p1++) {
		for (p2=0; p2<sampleSize; p2++) {
			if (planetsList[p1].id != planetsList[p2].id) {
				// computes distance between p1 and p2
				dx = planetsList[p2].pos.x - planetsList[p1].pos.x;
				dy = planetsList[p2].pos.y - planetsList[p1].pos.y;
				dz = planetsList[p2].pos.z - planetsList[p1].pos.z;
				d = sqrt((dx*dx) + (dy*dy) + (dz*dz));
				radiusSum = planetsList[p1].radius + planetsList[p2].radius;
				if (d < radiusSum) { d = radiusSum; }

				// computes force attraction
				f = g * planetsList[p2].mass / (d*d);
				// compute acceleration
				a.x = f * dx/d;
				a.y = f * dy/d;
				a.z = f * dz/d;

				planetsList[p1].velocity.x += a.x;
				planetsList[p1].velocity.y += a.y;
				planetsList[p1].velocity.z += a.z;
			}
		}
		planetsList[p1].pos.x += planetsList[p1].velocity.x/100.0;
		planetsList[p1].pos.y += planetsList[p1].velocity.y/100.0;
		planetsList[p1].pos.z += planetsList[p1].velocity.z/100.0;

		planetsList[p1].path = realloc(planetsList[p1].path, (pathLength+1)*sizeof(vector));
		planetsList[p1].path[pathLength] = planetsList[p1].pos;
	}
	glutPostRedisplay();
	glutTimerFunc(dt*4, update, sampleSize);
}


void init(void) {
	if (background){ // White background
		glClearColor(1.0, 1.0, 1.0, 1.0);
	} else { // Black background
		glClearColor(0.1, 0.1, 0.1, 1.0);
	}

	glEnable(GL_LIGHTING);

	GLfloat ambient[] = {0.05f, 0.05f, 0.05f, 1.0f};
	GLfloat diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
	GLfloat specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
	GLfloat position[] = {0.0f, 0.0f, 0.0f, 1.0f};
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, specular);
	glLightfv(GL_LIGHT0, GL_POSITION, position);
	glEnable(GL_LIGHT0);

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	GLfloat matAmbient[] = {0.3f, 0.3f, 0.3f, 1.0f};
	GLfloat matDiffuse[] = {0.6f, 0.6f, 0.6f, 1.0f};
	GLfloat matSpecular[] = {0.8f, 0.8f, 0.8f, 1.0f};
	GLfloat matShininess[] = {128.0f};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmbient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShininess);

	GLfloat baseAmbient[] = {0.5f, 0.5f, 0.5f, 0.5f};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, baseAmbient);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

	// points smoothing
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

	//needed for transparency
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glShadeModel(GL_SMOOTH); // smooth shading
	glEnable(GL_NORMALIZE); // recalc normals for non-uniform scaling
	glEnable(GL_AUTO_NORMAL);

	glEnable(GL_CULL_FACE); // do not render back-faces, faster
}


void glmain(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(winSizeW, winSizeH);
	glutInitWindowPosition(120, 10);
	glutCreateWindow(WINDOW_TITLE_PREFIX);
	glutDisplayFunc(display);
	glutReshapeFunc(onReshape);
	glutSpecialFunc(onSpecial);
	glutMotionFunc(onMotion);
	glutIdleFunc(onIdle);
	glutMouseFunc(onMouse);
	glutKeyboardFunc(onKeyboard);
	glutTimerFunc(dt, onTimer, 0);
	glutTimerFunc(dt*4, update, sampleSize);
	init();
	fprintf(stdout, "INFO: OpenGL Version: %s\n", glGetString(GL_VERSION));
	fprintf(stdout, "INFO: Screen size (%d, %d)\n", glutGet(GLUT_SCREEN_WIDTH), glutGet(GLUT_SCREEN_HEIGHT));
	fprintf(stdout, "INFO: Nbr elts: %d\n", sampleSize);
	glutMainLoop();
	fprintf(stdout, "INFO: Freeing memory\n");
	glDeleteLists(textList, 1);
}


double generateIntRandom(void) {
	double result = 0;
	int negativ = 0;

	negativ = rand() % 2;
	result = (double)(rand() % concentration);
	if (negativ) { result *= -1; }
	return(result);
}


float generateFloatRandom(void) {
	float result;
	result = (float)rand() / (float)(RAND_MAX - 1);
	return(result);
}


double generateRangeRandom(double min, double max) {
	double result = 0;
	result = min + (double)rand() / ((double)RAND_MAX / (max - min + 1) + 1);
	return(result);
}


void populatePlanets(void) {
	int i = 0;
	double theta = sqrt(2) / 2.0;
	for (i=0; i<sampleSize; i++) {
		planetsList[i].id = i;
		planetsList[i].selected = 0;
		planetsList[i].displayed = !allTraces;
		planetsList[i].color.x = generateFloatRandom();
		planetsList[i].color.y = generateFloatRandom();
		planetsList[i].color.z = generateFloatRandom();
		planetsList[i].pos.x = generateIntRandom();
		planetsList[i].pos.y = generateIntRandom();
		planetsList[i].pos.z = generateIntRandom();
		planetsList[i].velocity.x = planetsList[i].pos.x * cos(theta) - planetsList[i].pos.y * sin(theta);
		planetsList[i].velocity.y = planetsList[i].pos.x * sin(theta) + planetsList[i].pos.y * cos(theta);
		planetsList[i].velocity.z = generateFloatRandom();
		planetsList[i].path = calloc(1, sizeof(vector));
		planetsList[i].path[pathLength] = planetsList[i].pos;
		planetsList[i].mass = generateRangeRandom(minWeight, maxWeight);
		planetsList[i].radius = pow(((3.0 * planetsList[i].mass) / (4.0 * pi * density)), (1.0/3.0));
	}
}


int main(int argc, char *argv[]) {
	switch (argc) {
		case 2:
			if (!strncmp(argv[1], "white", 5)) {
				background = 1;
			}
			help();
			srand(time(NULL));
			populatePlanets();
			glmain(argc, argv);
			exit(EXIT_SUCCESS);
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
			break;
	}
}
