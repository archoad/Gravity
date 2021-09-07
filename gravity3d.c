/*gravity3d
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

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#define WINDOW_TITLE_PREFIX "Gravity simulation"
#define couleur(param) printf("\033[%sm",param)
#define BUFSIZE 512
#define MAXOBJECTS 5000

static short winSizeW = 1200,
	winSizeH = 900,
	frame = 0,
	currentTime = 0,
	timebase = 0,
	fullScreen = 0,
	trace = 0,
	allTraces = 1,
	rotate = 1,
	axe = 1,
	concentration = 80,
	dt = 5; // in milliseconds

static int textList = 0,
	cpt = 0,
	pathLength = 0,
	maxPathLength = 150,
	sampleSize = 1500;

static float fps = 0.0,
	rotx = -80.0,
	roty = 0.0,
	rotz = 20.0,
	xx = 0.0,
	yy = 5.0,
	zoom = 600.0,
	prevx = 0.0,
	prevy = 0.0;

static double minDistance = 2.0,
	maxWeight = 50,
	minWeight = 5,
	density = 1.5,
	pi = 3.14159265358979323846,
	g = -9.8;

typedef struct _vector {
	double x, y, z;
} vector;

typedef struct _objects {
	vector pos;
	vector color;
	vector velocity;
	vector force;
	vector *path;
	double radius;
	double mass;
	int id;
	short selected;
} objects;


static objects objectsList[MAXOBJECTS];




void help(void) {
	couleur("31");
	printf("Michel Dubois -- gravity3d -- (c) 2013\n\n");
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
	printf("\t't' to display selected object trace or not\n");
	printf("\t'a' to display trace of all objects\n");
	printf("Mouse usage:\n");
	printf("\t'LEFT CLICK' to select an object\n");
	printf("\n");
}


double generateFloatRandom(void) {
	double result = 0;
	result = (double)rand() / (double)(RAND_MAX - 1);
	return(result);
}


double generatePosRandom(void) {
	double result = 0;
	int negativ = 0;
	negativ = rand() % 2;
	result = (double)(rand() % concentration);
	if (negativ) { result *= -1; }
	return(result);
}


double generateRangeRandom(double min, double max) {
	double result = 0;
	result = (((double)rand() / (double)RAND_MAX) * (max - min)) + min;
	return(result);
}


vector addVec(vector v1, vector v2) {
	vector r;
	r.x = v1.x + v2.x;
	r.y = v1.y + v2.y;
	r.z = v1.z + v2.z;
	return(r);
}


vector subVec(vector v1, vector v2) {
	vector r;
	r.x = v1.x - v2.x;
	r.y = v1.y - v2.y;
	r.z = v1.z - v2.z;
	return(r);
}


vector mulVecByScalar(vector v, double s) {
	vector r;
	r.x = v.x * s;
	r.y = v.y * s;
	r.z = v.z * s;
	return(r);
}


vector divVecByScalar(vector v, double s) {
	vector r;
	r.x = v.x / s;
	r.y = v.y / s;
	r.z = v.z / s;
	return(r);
}


double magnitude(vector v) {
	// compute length of vector
	double r = 0.0;
	r = (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
	return(sqrt(r));
}


vector normalize(vector v) {
	double mag=0;
	vector tmp;
	tmp.x=0; tmp.y=0; tmp.z=0;
	mag = magnitude(v);
	if (mag>0) {
		tmp = divVecByScalar(v, mag);
	}
	return(tmp);
}


vector limitForce(vector v, double max) {
	vector r;
	double mag = 0;
	r.x = 0; r.y = 0; r.z = 0;
	mag = magnitude(v);
	if (mag > max) {
		r = divVecByScalar(v, mag);
		r = mulVecByScalar(r, max);
	}
	return(r);
}


double distance(objects o1, objects o2) {
	vector r;
	r = subVec(o2.pos, o1.pos);
	return(magnitude(r));
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


char* displayObject(objects o, int simple) {
	char *text = NULL;
	text = calloc(120, sizeof(text));
	if (simple) {
		sprintf(text, "[%d] coord: (%.2f, %.2f, %.2f)", o.id, o.pos.x, o.pos.y, o.pos.z);
	} else {
		sprintf(text, "[%d] coord: (%.2f, %.2f, %.2f) velocity: (%.2f, %.2f, %.2f)\n", o.id, o.pos.x, o.pos.y, o.pos.z, o.velocity.x, o.velocity.y, o.velocity.z);
	}
	return(text);
}


void drawObject(objects o, int name) {
	glPushMatrix();
	glTranslatef(o.pos.x, o.pos.y, o.pos.z);
	if (o.selected) {
		glColor3f(1.0, 0.0, 0.0);
		glutWireCube(o.radius * 2.0);
	}
	glColor3f(o.color.x, o.color.y, o.color.z);
	glLoadName(name);
	glutSolidSphere(o.radius, 12, 12);
	glPopMatrix();
}


void drawPath(objects o) {
	int i = 0;
	glPointSize(0.5f);
	glColor3f(o.color.x, o.color.y, o.color.z);
	for (i=0; i<maxPathLength; i++) {
		glBegin(GL_POINTS);
			glNormal3f(o.path[i].x, o.path[i].y, o.path[i].z);
			glVertex3f(o.path[i].x, o.path[i].y, o.path[i].z);
		glEnd();
	}
}


void drawString(float x, float y, float z, char *text) {
	unsigned i = 0;
	glPushMatrix();
	glLineWidth(1.0);
	glColor3f(1.0, 1.0, 1.0); // Black background
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
	sprintf(text1, "Nbr of objects: %d", sampleSize);
	sprintf(text2, "dt: %1.3f, FPS: %4.2f", (dt/1000.0), fps);
	for (i=0; i<sampleSize; i++) {
		if (objectsList[i].selected) {
			sprintf(text3, "%s", displayObject(objectsList[i], 0));
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
	glColor3f(0.4, 0.4, 0.4);
	glutWireCube(100.0*3.0);
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
		drawObject(objectsList[i], i);
		if (trace) {
			if (objectsList[i].selected) {
				drawPath(objectsList[i]);
			}
		}
		if (allTraces) {
			drawPath(objectsList[i]);
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
		objectsList[objectName].selected = !objectsList[objectName].selected;
		printf("INFO: Touched -> %s", displayObject(objectsList[objectName], 1));
	}
}


void selectObject(int x, int y) {
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
	switch (key) {
		case 27: // Escape
			printf("INFO: exit\n");
			printf("x %d, y %d\n", x, y);
			exit(0);
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


vector meanColor(int o1) {
	int o2 = 0,
		numNeighbors = 0;
	double dist = 0;
	vector sum;
	sum.x=0.0; sum.y=0.0; sum.z=0.0;
	for (o2=0; o2<sampleSize; o2++) {
		dist = distance(objectsList[o1], objectsList[o2]);
		if ((dist>0) & (dist < minDistance)) {
			sum = addVec(sum, objectsList[o2].color);
			numNeighbors++;
		}
	}
	if (numNeighbors) {
		sum = divVecByScalar(sum, numNeighbors);
		sum = normalize(sum);
	} else {
		sum.x = objectsList[o1].color.x;
		sum.y = objectsList[o1].color.y;
		sum.z = objectsList[o1].color.z;
	}
	return(sum);
}


void keepWithinBounds(int i) {
	double highLimit = 150.0,
		lowLimit = -150.0;
	if ((objectsList[i].pos.x >= highLimit) | (objectsList[i].pos.x < lowLimit)) {
		objectsList[i].velocity.x = -1 * objectsList[i].velocity.x;
	}
	if ((objectsList[i].pos.y >= highLimit) | (objectsList[i].pos.y < lowLimit)) {
		objectsList[i].velocity.y = -1 * objectsList[i].velocity.y;
	}
	/*
	if ((objectsList[i].pos.z >= highLimit) | (objectsList[i].pos.z < lowLimit)) {
		objectsList[i].velocity.z = -1 * objectsList[i].velocity.z;
	}
	*/
}


void addEltPath(int o1) {
	int i = 0;
	vector *temp = calloc(maxPathLength, sizeof(vector));

	if (pathLength < maxPathLength) {
		objectsList[o1].path[pathLength] = objectsList[o1].pos;
	} else {
		for (i=1; i<maxPathLength; i++) {
			temp[i-1] = objectsList[o1].path[i];
		}
		temp[maxPathLength-1] = objectsList[o1].pos;
		for (i=0; i<maxPathLength; i++) {
			objectsList[o1].path[i] = temp[i];
		}
	}
}


void update(int value) {
	int i=0;
	double force=0.0, dist=0.0, lowLimit=-150.0;
	vector acc, col, diff, ground;
	acc.x=0.0; acc.y=0.0; acc.z=0.0;
	ground.x = objectsList[i].pos.x;
	ground.y = objectsList[i].pos.y;
	ground.z = lowLimit;
	pathLength ++;

	for (i=0; i<value; i++) {
		col = meanColor(i);
		objectsList[i].color.x = col.x;
		objectsList[i].color.y = col.y;
		objectsList[i].color.z = col.z;

		diff = subVec(ground, objectsList[i].pos);
		dist = magnitude(diff);
		if ((dist > 0) & (objectsList[i].pos.z > lowLimit)) {
			force = (g * objectsList[i].mass) / (dist * dist);
			acc.z = force;
		} else {
			objectsList[i].velocity.z = 0;
			objectsList[i].pos.z = lowLimit;
		}
		objectsList[i].velocity = addVec(objectsList[i].velocity, acc);
		objectsList[i].pos = addVec(objectsList[i].pos, objectsList[i].velocity);
		keepWithinBounds(i);
		addEltPath(i);
	}
	glutPostRedisplay();
	glutTimerFunc(dt, update, sampleSize);
}


void init(void) {
	glClearColor(0.1, 0.1, 0.1, 1.0); // Black background

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

	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glEnable(GL_AUTO_NORMAL);
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);
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
	glutTimerFunc(dt, update, sampleSize);
	init();
	fprintf(stdout, "INFO: OpenGL Version: %s\n", glGetString(GL_VERSION));
	fprintf(stdout, "INFO: Screen size (%d, %d)\n", glutGet(GLUT_SCREEN_WIDTH), glutGet(GLUT_SCREEN_HEIGHT));
	fprintf(stdout, "INFO: Nbr elts: %d\n", sampleSize);
	glutMainLoop();
	fprintf(stdout, "INFO: Freeing memory\n");
	glDeleteLists(textList, 1);
}


void populateObjects(void) {
	int i = 0;
	for (i=0; i<sampleSize; i++) {
		objectsList[i].id = i;
		objectsList[i].selected = 0;
		objectsList[i].color.x = generateFloatRandom();
		objectsList[i].color.y = generateFloatRandom();
		objectsList[i].color.z = generateFloatRandom();
		objectsList[i].pos.x = generatePosRandom();
		objectsList[i].pos.y = generatePosRandom();
		objectsList[i].pos.z = generatePosRandom();
		objectsList[i].velocity.x = generateRangeRandom(-0.4, 0.4);
		objectsList[i].velocity.y = generateRangeRandom(-0.4, 0.4);
		objectsList[i].velocity.z = generateRangeRandom(0.5, 0.8);
		objectsList[i].mass = generateRangeRandom(minWeight, maxWeight);
		objectsList[i].radius = pow(((3.0 * objectsList[i].mass) / (4.0 * pi * density)), (1.0/3.0));
		objectsList[i].path = calloc(maxPathLength, sizeof(vector));
		objectsList[i].path[pathLength] = objectsList[i].pos;
	}
}


int main(int argc, char *argv[]) {
	help();
	srand(time(NULL));
	populateObjects();
	glmain(argc, argv);
	exit(EXIT_SUCCESS);
}
