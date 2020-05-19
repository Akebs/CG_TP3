// CG_TP3.c - Ricardo Jorge Medeiros, a14038. LEI - UALG
// Um programa OpenGL que exemplifica a iluminação, animacao, controlo
// e textura de diferentes ojecto 3Ds.//
// Este codigo esta baseaddo nos exemplos disponiveis no livro
// "OpenGL SuperBible", 2nd Edition, de Richard S. e Wright Jr.

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/freeglut.h>
#include "string.h"
#include <iostream>
#include <stdio.h>
#include <math.h>
#include "OBJ_Loader.h"
#include "WaterMesh.h"
//#include "camera.h"


#define SPACEBAR 32

using std::cout; using std::endl;

enum textures {EMPTYTEX=0,CHECK_WHITE,CHECK_RED, SKY_LEFT, SKY_BACK, SKY_RIGHT, SKY_FRONT, SKY_TOP, SKY_BOTTOM, SEA_BOTTOM, SKY_RAW, SEA_SAND,SEA_WEED,TERRAIN, SCATTERED_SKY};
enum objects {EMPTY=0,BOAT_HULL,BOAT_WINDOW,LANDSCAPE,CONE_1,CONE_2,CONE_3,CONE_4,RECTANGLE};

// Constantes
// Display variables
GLsizei 	screenWidth 	= 800;
GLsizei 	screenHeight 	= 600;
static GLfloat fAspect		= 0.0f;

// state variables
bool FLATREPRESENTATION 	= true;
bool ILLUMINATION 			= true;
bool MSAA 					= true;
bool LABELS					= true;
bool SHADING				= true;
bool LISTENING 				= false;		// FALSE
bool TEXTURED 				= true;
bool SHOWBOAT 				= true;
bool SHOWLANDSCAPE  		= true;
bool SHOWWATER				= true;
bool SHOWEXTRAS 			= true;
bool SHOWSUN 				= true;
bool SHOWSKYBOX				= true;
bool RUNNING				= false;
bool ENDED					= false;
bool RACE_MODE				= true;
bool CONE1COLLIDED 			= false;
bool CONE2COLLIDED 			= false;
bool CONE3COLLIDED 			= false;
bool CONE4COLLIDED 			= false;
bool CUBECOLLIDED			= false;
bool GAMEOVER				= false;


// Game variables
const int	GAME_TIME		= 60000;
const int 	TIMER 			= 1000/30; 	// Timer period for 30 FPS
const int 	SPEED_INCREMENT = 10;
const float SPEED_RATIO 	= 0.1f;		// boat velocity correction coeficient

static int 	scoring[2]		= {0,0};
static int 	gamelevel  		= 0;
// other variables
int		dx 				= 0;
int		dy 				= 0;
int 	dz 				= 0;
int 	starttime 		= 0;
int 	previoustime 	= 0;
float 	realfps 		= 0.0f;
int 	boat_speed 		= 0;		// initial speed
float 	boat_radius 	= 1.0f;
float 	boat_yaw, boat_pitch;

char 	texto [7][120];
unsigned int texname[15]; //the ids for the textures


GLint  	px, py, pz, lastMouseX, lastMouseY , direction, listindex;
GLint omega = 0;
GLvoid *font_style ;



objl::Vector3 sun(40.0, 20.0, ((float)-MAP_SIZE_XX)/2);
static watermesh::Particle boat;



// uses loaded vertex meshes for object screation command lists
void makeObjList ( int obj, objl::Mesh objname){
	glNewList(obj, GL_COMPILE);	{
		glPushMatrix();
			glBegin(GL_TRIANGLES);
			if (LISTENING)
				cout << " " << objname.MeshName << " " << objname.Vertices.size() << " vertices." << endl;
			for (std::vector<objl::Vertex>::size_type j = 0; j < objname.Vertices.size(); j++)		{
				glVertex3f(  objname.Vertices[j].Position.X , objname.Vertices[j].Position.Y ,  objname.Vertices[j].Position.Z ) ;
				glNormal3f(	objname.Vertices[j].Normal.X ,  objname.Vertices[j].Normal.Y ,objname.Vertices[j].Normal.Z );
				glTexCoord2f(objname.Vertices[j].TextureCoordinate.X , objname.Vertices[j].TextureCoordinate.Y );
			}
			glEnd();
			glPopMatrix();
		}
	glEndList();
}

// Auxiliary method to convert degrees to compass cardinal references
const char* degToCompass(float deg){
	char intstr [3];
	int val = int((deg/22.5)+.5);
	itoa(val,intstr,10);
	const char* myarray[16] = { "N ","NNW","NW ","WNW","W  ","WSW","SW ","SSW","S ","SSE","SE ","ESE","E  ","ENE","NE ","NNE"};
	return myarray[(val % 16)];
}


// Auxiliary function to raster text
void drawText(GLint x, GLint y, char* text, void* font_style) {
	// screen position to position text
    glRasterPos2i(x, y);
    // bitmap drawing of text
	while(*text)
        glutBitmapCharacter(font_style, *text++);
}


/*
Transparency - Ability to pass light through the surface.
Ambience - Light reflected and scattered by other objects.
Diffusion - Light scattered equally in all directions on the surface.
Specularity - Ability to reflect light from a surface.
Shininess 	- A glossy, highly reflective surface.
Emissivity 	- Ability to project light from the surface.
*/
// lightning parameters settings
//    y !
//      !___x
//		/
//	  z/
void lightning(void)
{
	// background color sunset blue sky
	GLfloat luzAmbiente[4]={1.0,0.98,0.98,1.0};
	GLfloat luzDifusa[4]={0.8,0.8,0.8,1.0};
	GLfloat luzEspecular[4]={0.8, 0.8, 0.8, 1.0};
	// light position  x,y,z, {0 (directional) ,1 (position)}
	GLfloat lightpos[4]={sun.X, sun.Y, sun.Z, 1.0};
	//set the lighting model parameterste
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, luzAmbiente);
	// Define os parâmetros da luz de número 0
	glLightfv(GL_LIGHT0, GL_AMBIENT, luzAmbiente);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, luzDifusa );
	glLightfv(GL_LIGHT0, GL_SPECULAR, luzEspecular );
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos );
}


// loading a 2D texture from file
unsigned int loadTexture(const char* filename)
{
	unsigned int textureID;						//the id for the texture
	int width, height, numComponents;
	glGenTextures(1,&textureID);				//generate a unique texture ID
	if (LISTENING) cout << "name " << filename << ", ID: " << textureID << endl;
	unsigned char* data = stbi_load(filename, &width, &height, &numComponents, 0);
	if (LISTENING) cout <<" width: "<< width  << ", height: "<<  height << ", numComponents:" << numComponents << endl;
	if (data)  {
		GLenum format;
		if (numComponents == 1)
			format = GL_RED;
		else if (numComponents == 3)
			format = GL_RGB;
		else if (numComponents == 4)
			format = GL_RGBA;
		glBindTexture(GL_TEXTURE_2D,textureID);	//and use the texture, we have just generated
		// Sets the wrap parameter for texture coordinate s to be clamped to the range,
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE); //GL_CLAMP
		// Sets the wrap parameter for texture coordinate t to be clamped to the range,
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
		//  when the pixel being textured maps to an area less than or equal to one texture element, returns the weighted average of the four texture elements that are closest to the center of the pixel being textured.
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);//GL_LINEAR
		//same if the image bigger
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		// specifies a two-dimensional texture image (target, level, internalformat, width, height, border, format, type, pixels)
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		stbi_image_free(data);
	}
	else	{
		cout << "Texture failed to load at path: " << filename << endl;
	    stbi_image_free(data);
	}
	if (LISTENING) cout << filename<< " texture done" << endl;
	return textureID;	//and we return the id
}

//load textures,
void textures(void){
	texname[CHECK_WHITE]=loadTexture("resources/CHECK_WHITE.png");
	texname[CHECK_RED]	=loadTexture("resources/CHECK_RED.png");
	texname[SKY_LEFT]	=loadTexture("resources/SKY_LEFT.png");
	texname[SKY_BACK]	=loadTexture("resources/SKY_BACK.png");
	texname[SKY_RIGHT]	=loadTexture("resources/SKY_RIGHT.png");
	texname[SKY_FRONT]	=loadTexture("resources/SKY_FRONT.png");
	texname[SKY_TOP]	=loadTexture("resources/SKY_TOP.png");
	texname[SKY_BOTTOM]	=loadTexture("resources/SKY_BOTTOM.png");
	texname[SKY_RAW]	=loadTexture("resources/SKY_RAW.png");
	texname[SEA_BOTTOM]	=loadTexture("resources/SEA_BOTTOM.png");
	texname[SEA_SAND]	=loadTexture("resources/SEA_SAND.png");
	texname[SEA_WEED]	=loadTexture("resources/SEA_WEED.png");
	texname[TERRAIN]	=loadTexture("resources/TERRAIN.png");
	texname[SCATTERED_SKY]	=loadTexture("resources/SCATTERED_SKY.png");
	if (LISTENING)
		cout <<  " textures loaded" << endl;
}


//delete all textures (to avoid memory leaks)
void killTextures() {
	glDeleteTextures(8,&texname[0]);
}

// check boat collisions
bool hasCollided (Vector3 boat_pos){
	Vector3 cone1_pos = Vector3( 20.0f,	0.0f, 44.0f);
	Vector3 cone2_pos = Vector3( 13.0f, 0.0f, 11.8f);
	Vector3 cone3_pos = Vector3(-20.6f, 0.0f,-10.0f);
	Vector3 cone4_pos = Vector3(-27.0f, 0.0f, -7.0f);
	Vector3 cube_pos  = Vector3(  7.1f,	0.0f,-30.0f);
	// cone1 collision
	float other_radius = 0.5f;
	float a_to_b = (cone1_pos - boat_pos).length();
	if ( a_to_b < boat_radius + other_radius){
		if (LISTENING && !CONE1COLLIDED )
			cout << "CONE 1 COLLIDED" << endl;
		CONE1COLLIDED = true;

		return true;
	}
	// cone1 collision
	a_to_b = (cone2_pos - boat_pos).length();
	if ( a_to_b < boat_radius + other_radius){
		if (LISTENING && !CONE2COLLIDED )
			cout << "CONE 2 COLLIDED" << endl;
		CONE2COLLIDED = true;
		return true;
	}
	// cone2 collision
	a_to_b = (cone3_pos - boat_pos).length();
	if ( a_to_b < boat_radius + other_radius){
		if (LISTENING && !CONE3COLLIDED )
			cout << "CONE 3 COLLIDED" << endl;
		CONE3COLLIDED = true;
		return true;
	}
	// cone3 collision
	a_to_b = (cone4_pos - boat_pos).length();
	if ( a_to_b <boat_radius + other_radius){
		if (LISTENING && !CONE4COLLIDED )
			cout << "CONE 4 COLLIDED" << endl;
		CONE4COLLIDED = true;
		return true;
	}
	// cube collision
	Vector3 left  = Vector3(cube_pos.X - 1, 0.0f, cube_pos.Z);
	Vector3 right = Vector3(cube_pos.X + 1, 0.0f, cube_pos.Z);
	a_to_b = (left - boat_pos).length();
	if ( a_to_b < boat_radius + other_radius){
		if (LISTENING && !CUBECOLLIDED )
			cout << "CUBE COLLIDED" << endl;
		CUBECOLLIDED = true;
		return true;
	}
	a_to_b = (right - boat_pos).length();
	if ( a_to_b < boat_radius + other_radius){
		if (LISTENING && !CUBECOLLIDED )
			cout << "CUBECOLLIDED" << endl;
		CUBECOLLIDED = true;
		return true;
	}
	return false;
}


// load objects from meshes2.obj file to buffers
void loadObjects (void) {
	// create loader object
	objl::Loader loader;
	// load meshes from obj file
	loader.LoadFile("src/meshes2.obj");
	for (int i= 0 ; i < (int) loader.LoadedMeshes.size(); i++) {
	//	if (LISTENING)		// print file objects names
			cout << " objects found in file : " + loader.LoadedMeshes[i].MeshName << endl;
		objl::Mesh mesh = loader.LoadedMeshes[i];
		// make new command list for objects creation
		makeObjList(i+1,mesh);
		if (LISTENING)
			cout << i+1 << ": " <<  mesh.MeshName << " list loaded" << endl;
	}
	if (LISTENING)
		cout <<  "lists loading finished" << endl;
}


int * levelScore(){
	static int scoring[2];
	if (gamelevel == 1){
		scoring[0] = CUBECOLLIDED;
		scoring[1] = 1;
		GAMEOVER = (CONE1COLLIDED || CONE2COLLIDED || CONE3COLLIDED || CONE4COLLIDED);
	}
	return scoring;
}

void resetGame(){
	boat.position = Vector3(0.0f,0.0f,0.0f);
	boat.velocity = Vector3(1.0f,0.0f,0.0f);

	boat_yaw = 0.0f;
	boat_pitch = 0.0f;
	boat_speed = 0.0f;
	ENDED = false;
	CUBECOLLIDED  = false;
	CONE1COLLIDED = false;
	CONE2COLLIDED = false;
	CONE3COLLIDED = false;
	CONE4COLLIDED = false;
	scoring[0] = 0;
	scoring[1] = 0;
	gamelevel = 1;
	watermesh::resetMesh();
	starttime = glutGet( GLUT_ELAPSED_TIME );
 	glutPostRedisplay();
}


void updateLabels () {
	// TEXTO 0
	sprintf(texto[0], "POS(%.1f,%.1f) DIR: %.1f %s speed: %d kn ",
			boat.position.X, boat.position.Z,	boat_yaw, degToCompass(boat_yaw), boat_speed);

	// TEXTO 1
	sprintf(texto[1], "CAM_POS (%.1f, %.1f, %.1f) ZOOM %.1f° (%.1fx) ANGLE (%d°,%d°) FPS: (%.1f) ",
	(float)camera.Position.X, (float)camera.Position.Y, (float)camera.Position.Z, camera.Zoom,128.0f/camera.Zoom, (int)camera.Yaw, (int)camera.Pitch, (float)realfps);

	// TEXTO 2
	char str[80];
	strcpy (str,"DISPLAY: ");
	if (FLATREPRESENTATION)	  	strcat (str," FLAT ");
	if (ILLUMINATION) 			strcat (str," ILLUMINATION ");
	if (SHADING) 				strcat (str," SHADING ");
	if (MSAA) 					strcat (str," MSAA ");
	if (TEXTURED) 				strcat (str," TEXTURED ");
	sprintf(texto[2], str);

	// TEXTO 3
	strcpy (str,"OBJECTS : ");
	if (SHOWBOAT)	  	strcat (str,"BOAT		 ");
	if (SHOWLANDSCAPE) 	strcat (str,"LANDSCAPE   ");
	if (SHOWWATER) 		strcat (str,"WATER       ");
	if (SHOWEXTRAS) 	strcat (str,"EXTRAS      ");
	if (SHOWSUN) 		strcat (str,"SUN         ");
	if (SHOWSKYBOX) 	strcat (str,"SKYBOX      ");
	sprintf(texto[3], str);

	// TEXTO 4
	strcpy (str,"COLLISIONS : ");
	if (CONE1COLLIDED)	  	strcat (str,"CONE1_COLLIDED	 ");
	if (CONE2COLLIDED) 		strcat (str,"CONE2_COLLIDED  ");
	if (CONE3COLLIDED) 		strcat (str,"CONE3_COLLIDED  ");
	if (CONE4COLLIDED) 		strcat (str,"CONE4_COLLIDED  ");
	if (CUBECOLLIDED) 		strcat (str,"CUBE_COLLIDED   ");
	sprintf(texto[4], str);

	// TEXTO 5
	if (RACE_MODE){
		float elapsedTime =  (float)(glutGet( GLUT_ELAPSED_TIME ) - starttime)/1000.0f;
		if (RUNNING) {
			if (ENDED) {
						sprintf(texto[5], "GAME OVER  %d/%d points ", scoring[0], scoring[1]);
						sprintf(texto[6], "Press P to play again");
			}
			else {
				sprintf(texto[5], "Get to checkers box, avoiding the mines");
				sprintf(texto[6], "%.1f s           Points: %d", elapsedTime, scoring[0]);
			}
		}
		else {
				sprintf(texto[6], "Press P to start");
		}
	}
}


void update (float timeframe) {
	// check game time
	if (RACE_MODE) {
		if (!ENDED) {
			scoring[0] = levelScore()[0];
			scoring[1] = levelScore()[1];
			int nowtime = glutGet( GLUT_ELAPSED_TIME );
			if ((nowtime - starttime) > GAME_TIME) {
				boat_speed = 0;
				ENDED = true;
				RUNNING = false;
			}
		}
	}
	// store current boat position
	Vector3 old_boatpos = Vector3 (boat.position.X,boat.position.Y,boat.position.Z);
	// update boat position
	boat.position += (boat.velocity * boat_speed * timeframe * SPEED_RATIO);
	// check collisions
	if (hasCollided(boat.position))
		cout << "collided" << endl;
	// update particle mesh reference offset
	watermesh::updateMeshOffset(old_boatpos, boat.position);
	// get bow_position
	Vector3 bow_pos = boat.position + (boat.velocity.normalized() * 0.8f);
	// restrict boat within world limits
	if (fabs(bow_pos.X) + 0.5f  >= MAP_SIZE_XX/2 || fabs(bow_pos.Z ) + 0.5f >= MAP_SIZE_ZZ/2) {
		boat_speed = 0;
	}
	else {
		// update water map displacement
		Vector3 offsetbow_pos =  Vector3( (float)MAP_SIZE_XX /2 - bow_pos.X, 0.0f, (float)MAP_SIZE_ZZ/2 - bow_pos.Z);			// TODO MAP_XX
		// find closest particle
		Point2i nearestpoint ((int) round(offsetbow_pos.X ), (int) round(offsetbow_pos.Z ));
		// change particle Y coordinate
		float fRowWeight = 0.5f - fabs( offsetbow_pos.X - nearestpoint.x );
		float fColWeight = 0.5f - fabs( offsetbow_pos.Z - nearestpoint.y );
		// calculate vertical displacement
		watermesh::mesh[nearestpoint.x][nearestpoint.y].position.Y = -SPLASH_FORCE * fRowWeight * fColWeight;
		// fix point
		watermesh::mesh[nearestpoint.x][nearestpoint.y].fixed = true;
		// update particles
		watermesh::updateForces();
		watermesh::applyForces( timeframe );
		watermesh::genNormals();
		watermesh::genTexCoordinates();
	//	if (boat.position.X > 10 && boat.position.Z > 10) {
	//	for (int r = 0; r < MAP_SIZE_ZZ ; ++r) {
	//		for (int c = 0; c < MAP_SIZE_XX ; ++c) {
	//			cout << "update position C:R : (" << c << ":"<< r <<  ") .position: " << watermesh::mesh[r][c].position.X << " " << watermesh::mesh[r][c].position.Y << " " << watermesh::mesh[r][c].position.Z <<endl;
	//		}
	//	}
	}
	updateLabels();
 	glutPostRedisplay();
	}


static void timer (int value) {
	if (previoustime == 0) {
		previoustime = glutGet( GLUT_ELAPSED_TIME );
	}
    int nowTime = glutGet( GLUT_ELAPSED_TIME );
    float elapsedTime = (nowTime - previoustime) / 1000.0f;
    previoustime = nowTime;
    realfps = 1 / elapsedTime;
	omega += (300 * SPEED_RATIO);
	if (omega >= 360)
		omega -= 360;
    update(elapsedTime );
    glutTimerFunc( TIMER, timer, 0 );// Reactivate update timer
	if (LISTENING)		cout << " timer" << endl;
}


void makeSkyBox(){
	glBindTexture(GL_TEXTURE_2D,texname[SKY_BACK]);
	glBegin(GL_QUADS);		//back face
		glNormal3f(0.0,0.0,1.0);
		glTexCoord2f(0,0);	//use the correct texture coordinate
		glVertex3f(MAP_SIZE_XX/2,MAP_SIZE_YY/2,MAP_SIZE_XX/2);	//and a vertex
		glTexCoord2f(1,0);	//and repeat it...
		glVertex3f(-MAP_SIZE_XX/2,MAP_SIZE_YY/2,MAP_SIZE_XX/2);
		glTexCoord2f(1,1);			glVertex3f(-MAP_SIZE_XX/2,-MAP_SIZE_YY/2,MAP_SIZE_XX/2);
		glTexCoord2f(0,1);			glVertex3f(MAP_SIZE_XX/2,-MAP_SIZE_YY/2,MAP_SIZE_XX/2);
	glEnd();
	glBindTexture(GL_TEXTURE_2D,texname[SKY_LEFT]);
	glBegin(GL_QUADS);		//left face
		glNormal3f(1.0,0.0,0.0);
		glTexCoord2f(0,0);			glVertex3f(-MAP_SIZE_XX/2,MAP_SIZE_YY/2,MAP_SIZE_XX/2);
		glTexCoord2f(1,0);			glVertex3f(-MAP_SIZE_XX/2,MAP_SIZE_YY/2,-MAP_SIZE_XX/2);
		glTexCoord2f(1,1);			glVertex3f(-MAP_SIZE_XX/2,-MAP_SIZE_YY/2,-MAP_SIZE_XX/2);
		glTexCoord2f(0,1);			glVertex3f(-MAP_SIZE_XX/2,-MAP_SIZE_YY/2,MAP_SIZE_XX/2);
	glEnd();
	glBindTexture(GL_TEXTURE_2D,texname[SKY_FRONT]);
	glBegin(GL_QUADS);		//front face
		glNormal3f(0.0,0.0,-1.0);
		glTexCoord2f(1,0);			glVertex3f(MAP_SIZE_XX/2,MAP_SIZE_YY/2,-MAP_SIZE_XX/2);
		glTexCoord2f(0,0);			glVertex3f(-MAP_SIZE_XX/2,MAP_SIZE_YY/2,-MAP_SIZE_XX/2);
		glTexCoord2f(0,1);			glVertex3f(-MAP_SIZE_XX/2,-MAP_SIZE_YY/2,-MAP_SIZE_XX/2);
		glTexCoord2f(1,1);			glVertex3f(MAP_SIZE_XX/2,-MAP_SIZE_YY/2,-MAP_SIZE_XX/2);
	glEnd();
	glBindTexture(GL_TEXTURE_2D,texname[SKY_RIGHT]);
	glBegin(GL_QUADS);		//right face
		glNormal3f(-1.0,0.0,0.0);
		glTexCoord2f(0,0);			glVertex3f(MAP_SIZE_XX/2,MAP_SIZE_YY/2,-MAP_SIZE_XX/2);
		glTexCoord2f(1,0);			glVertex3f(MAP_SIZE_XX/2,MAP_SIZE_YY/2,MAP_SIZE_XX/2);
		glTexCoord2f(1,1);			glVertex3f(MAP_SIZE_XX/2,-MAP_SIZE_YY/2,MAP_SIZE_XX/2);
		glTexCoord2f(0,1);			glVertex3f(MAP_SIZE_XX/2,-MAP_SIZE_YY/2,-MAP_SIZE_XX/2);
	glEnd();
	glBindTexture(GL_TEXTURE_2D,texname[SKY_TOP]);
	glBegin(GL_QUADS);		//top face
		glNormal3f(0.0,-1.0,0.0);
		glTexCoord2f(1,0);			glVertex3f(MAP_SIZE_XX/2,MAP_SIZE_YY/2,MAP_SIZE_XX/2);
		glTexCoord2f(0,0);			glVertex3f(-MAP_SIZE_XX/2,MAP_SIZE_YY/2,MAP_SIZE_XX/2);
		glTexCoord2f(0,1);			glVertex3f(-MAP_SIZE_XX/2,MAP_SIZE_YY/2,-MAP_SIZE_XX/2);
		glTexCoord2f(1,1);			glVertex3f(MAP_SIZE_XX/2,MAP_SIZE_YY/2,-MAP_SIZE_XX/2);
	glEnd();
	glBindTexture(GL_TEXTURE_2D,texname[SKY_BOTTOM]);
	glBegin(GL_QUADS);		//bottom face
		glNormal3f(0.0,1.0,0.0);
		glTexCoord2f(1,1);			glVertex3f(MAP_SIZE_XX/2,-MAP_SIZE_YY/2,MAP_SIZE_XX/2);
		glTexCoord2f(0,1);			glVertex3f(-MAP_SIZE_XX/2,-MAP_SIZE_YY/2,MAP_SIZE_XX/2);
		glTexCoord2f(0,0);			glVertex3f(-MAP_SIZE_XX/2,-MAP_SIZE_YY/2,-MAP_SIZE_XX/2);
		glTexCoord2f(1,0);			glVertex3f(MAP_SIZE_XX/2,-MAP_SIZE_YY/2,-MAP_SIZE_XX/2);
	glEnd();
}


void drawLabels() {
    // backup current model-view matrix
	if (LABELS){
		glPushMatrix();                    		// save current modelview matrix
			glLoadIdentity();                   // reset modelview matrix
			glMatrixMode(GL_PROJECTION);		// switch to projection matrix
			glPushMatrix();						// save current projection matrix
				glLoadIdentity();				// reset projection matrix
				gluOrtho2D(0, screenWidth, 0, screenHeight);// set to orthogonal projection
				glDisable(GL_LIGHTING);
				glColor3ub(56, 46, 46);
				glDisable(GL_LIGHTING);			//turn off lighting, when making the skybox
				glDisable(GL_DEPTH_TEST);		//turn off depth texting
				glDisable (GL_BLEND);
				glEnable(GL_TEXTURE_2D);		//and turn on texturing
				glPolygonMode(GL_FRONT, GL_FILL);
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
				// Sets the wrap parameter for texture coordinate t to be clamped to the range,
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				glBindTexture(GL_TEXTURE_2D,texname[SEA_SAND]);	//use the texture we want
				glBegin(GL_QUADS);		//back face
					glNormal3f(0.0,0.0,1.0);
					glTexCoord2f(0,0);			glVertex3f(0.0f, 0.0f, -1.0f);	//and a vertex
					glTexCoord2f(1,0);			glVertex3f(screenWidth, 0.0, -1.0f);
					glTexCoord2f(1,1);			glVertex3f(screenWidth, 70.0, -1.0f);
					glTexCoord2f(0,1);			glVertex3f(0.0f, 70.0f, -1.0f);
				glEnd();
				glBindTexture(GL_TEXTURE_2D,0);
				glColor3ub(255, 255, 255);
				drawText( 5,screenHeight-16, texto[0], GLUT_BITMAP_9_BY_15);
				drawText( 5,screenHeight-32, texto[1], GLUT_BITMAP_HELVETICA_12);
				drawText( 5,screenHeight-48, texto[2], GLUT_BITMAP_HELVETICA_12);
				drawText( 5,screenHeight-64, texto[3], GLUT_BITMAP_HELVETICA_12);
				drawText( 5,screenHeight-80, texto[4], GLUT_BITMAP_HELVETICA_12);

				int strsize = glutBitmapLength ( GLUT_BITMAP_TIMES_ROMAN_24,(unsigned char*) texto[5]);
				glColor3ub(245, 205, 240);
				drawText( int((screenWidth -strsize)/2) , 45, texto[5], GLUT_BITMAP_TIMES_ROMAN_24);

				strsize = glutBitmapLength ( GLUT_BITMAP_TIMES_ROMAN_24,(unsigned char*) texto[6]);
				drawText( int((screenWidth -strsize)/2), 18, texto[6], GLUT_BITMAP_TIMES_ROMAN_24);
			// restore projection matrix
			glPopMatrix();
			// restore to previous projection matrix
			// restore modelview matrix
			glMatrixMode(GL_MODELVIEW);      	// switch to modelview matrix
		glPopMatrix();                   		// restore to previous modelview matrix
		if (ILLUMINATION)
			glEnable(GL_LIGHTING);
		if (!FLATREPRESENTATION)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glEnable(GL_DEPTH_TEST);
		glEnable (GL_BLEND);
		if (LISTENING)		cout << " 2D text drawed" << endl;
	}
}


void drawSkybox(){
	glPushMatrix();
	glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT );
		float difamb[]={0.75,0.75,0.1,1.0};
		glDisable(GL_LIGHTING);				//turn off lighting, when making the skybox
		glDisable(GL_DEPTH_TEST);			//turn off depth texting
		glEnable(GL_TEXTURE_2D);			//and turn on texturing
		glColor3f(0.4,0.4,0.4);
		glBindTexture(GL_TEXTURE_2D,texname[SKY_BACK]);	//use the texture we want
		glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,difamb);
		makeSkyBox();
	glPopAttrib();
	glPopMatrix();
	glBindTexture(GL_TEXTURE_2D,0);
	// Habilita o depth-buffering
	glEnable(GL_DEPTH_TEST);
	//turn lightning back on
	if (ILLUMINATION)
			glEnable(GL_LIGHTING);
	if (LISTENING)		cout << " skybox drawed" << endl;
}

// draw secondary elements
void drawExtras() {
	glPushMatrix();
		glScalef(0.8,0.8,0.8);
		glTranslatef(0.0, 0.25* sin(omega * DEG_TO_RADIANS) -0.5, 0.0 );
		if (TEXTURED) {
			glBindTexture(GL_TEXTURE_2D, texname[CHECK_RED]);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
			glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
		}
		glColor3f(1.0,0.1,0.1);
		glCallList (CONE_1);
		glCallList (CONE_2);
		glCallList (CONE_3);
		glCallList (CONE_4);
		if (TEXTURED) {
			glBindTexture(GL_TEXTURE_2D, texname[CHECK_WHITE]);
		}
		glCallList (RECTANGLE);
	glPopMatrix();
	glBindTexture(GL_TEXTURE_2D, 0);  // unbind texture
	if (LISTENING)		cout << " extras drawed. Oscillation =" << sin(omega * DEG_TO_RADIANS) << endl;
}

// draw boat windows and hull
void drawLandscape(){
	glPushMatrix();
	glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT );
		glEnable(GL_TEXTURE_2D);	//and turn on texturing
		glColor3f(0.44,0.41,0.29);
	  		// specularity ability
		GLfloat especularidade[4]={0.20,0.20,0.20,1.0};
		//shininess concentration  (brilho)
		GLint especMaterial = 8;
		// material specularity (refletancia)
		glMaterialfv(GL_FRONT,GL_SPECULAR, especularidade);
		// material shininess
		glMateriali(GL_FRONT,GL_SHININESS,especMaterial);
		glScalef(1.1,0.80,0.80);
		if (TEXTURED) {
			glBindTexture(GL_TEXTURE_2D, texname[TERRAIN]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		}
		glCallList(LANDSCAPE);
		glPopAttrib();
	glPopMatrix();
	glBindTexture(GL_TEXTURE_2D, 0);
	if (LISTENING)		cout << " landscape drawed" << endl;

}

void drawWaterWireframe () {
	glPushMatrix();
    glColor3f( 0.2f, 0.2f, 0.7f );
	glScalef(1.0,1.0,1.0);
    for (int r = 0; r < MAP_SIZE_ZZ; ++r)
    { // longitudinal lines
        glBegin( GL_LINE_STRIP );
        for (int c = 0; c < MAP_SIZE_XX; ++c) {
            glVertex3f( watermesh::mesh[r][c].position.X, watermesh::mesh[r][c].position.Y, watermesh::mesh[r][c].position.Z );
        }
        glEnd();
    }
    for (int c = 0; c < MAP_SIZE_XX; ++c)
    {	// transversal lines
        glBegin( GL_LINE_STRIP );
        for (int r = 0; r < MAP_SIZE_ZZ; ++r) {
            glVertex3f( watermesh::mesh[r][c].position.X, watermesh::mesh[r][c].position.Y, watermesh::mesh[r][c].position.Z );
        }
        glEnd();
    }
    glPopMatrix();
}

void drawWaterPoly () {
	glBindTexture(GL_TEXTURE_2D, 0);
    glColor3f( 1.0f,1.0f,1.0f);
    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_NORMAL_ARRAY );
    glEnableClientState( GL_TEXTURE_COORD_ARRAY );
    glClientActiveTexture( GL_TEXTURE0 );
    // define normals array (type, byte offset between consecutive points, point to first normal in array)
    glNormalPointer( GL_FLOAT, sizeof(watermesh::Particle), &watermesh::mesh[0][0].normal );
    // define texture array (type, byte offset between consecutive points, point to first texture in array)
    glTexCoordPointer( 2, GL_FLOAT, sizeof(watermesh::Particle), &watermesh::mesh[0][0].texCoords );
    // define vertex array (type, byte offset between consecutive vertex, point to first texture in array)
    glVertexPointer( 3, GL_FLOAT, sizeof(watermesh::Particle), &watermesh::mesh[0][0].position );
    for (int i = 0; i < MAP_SIZE_XX; ++i) {//
        glBegin( GL_QUAD_STRIP );
        for (int j = 0; j <= MAP_SIZE_ZZ; ++j)   {
        	// render a vertex using the specified vertex array element
            glArrayElement(    i *(MAP_SIZE_ZZ+1) + j );
            glArrayElement( (i+1)*(MAP_SIZE_ZZ+1) + j );
        }
        glEnd();
    }
    glDisableClientState( GL_VERTEX_ARRAY );
    glDisableClientState( GL_NORMAL_ARRAY );
    glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	if (LISTENING)		cout << " polygonized water drawed" << endl;
}



void drawWaterText ()  {
    // Refracted ground
	if(glActiveTexture==NULL)
		cout << "glActiveTexture not supported"<< endl;
    glActiveTexture( GL_TEXTURE0 );
    glEnable( GL_TEXTURE_2D );
    // Underwater mapped texture
    glBindTexture( GL_TEXTURE_2D, SEA_SAND); // SEA_SAND
    GLfloat env_color[4] = { 0.2f, 0.2f, 0.4f, 0.4f };    // 30% grey 30% transparency
    glTexEnvfv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, env_color );
    // Sets the wrap parameter for texture coordinates to be mirror repeated
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_MIRRORED_REPEAT);
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
    glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE );
    glDisable( GL_TEXTURE_GEN_S );
    glDisable( GL_TEXTURE_GEN_T );
    // Environment mapped sky
    glActiveTexture( GL_TEXTURE1 );
    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, SCATTERED_SKY );
    glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );
    glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );
    glEnable( GL_TEXTURE_GEN_S );
    glEnable( GL_TEXTURE_GEN_T );
    // draw polygonized mesh
    drawWaterPoly();
    glActiveTexture( GL_TEXTURE0 );
    glDisable( GL_TEXTURE_2D );
    glActiveTexture( GL_TEXTURE1 );
    glDisable( GL_TEXTURE_2D );
	glBindTexture(GL_TEXTURE_2D, 0);
	if (LISTENING)		cout << " texturized Water drawed" << endl;
}


void drawWaterMesh ()  {
	glPushMatrix();
	glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT );
	glTranslatef(-MAP_SIZE_XX/2,0.0f,-MAP_SIZE_XX/2);
    if (!FLATREPRESENTATION)   {
    	if (LISTENING)			cout << "wireframe water representation" << endl;
    	drawWaterWireframe();
    }
    else   {
        if (TEXTURED){
        	if (LISTENING)		cout << "textured water representation" << endl;
        	drawWaterText();

        } else	{
        	if (LISTENING)    	cout << "polygon water representation" << endl;
        	drawWaterPoly();

        }
    }
    glPopAttrib();
	glPopMatrix();
}



// Draws a colored solid sphere representing a sunset sun
void drawSun(){
	glPushMatrix();
	glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT );
		glTranslatef(sun.X, sun.Y, sun.Z);
		glColor3ub(255, 210, 0);
		GLfloat paleYellow[]={1.0,0.95,0.4,1.0};
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, paleYellow);
		glutSolidSphere(2,32,32);
		glEnd();
	glPopAttrib();
	glPopMatrix();
	if (LISTENING)		cout << " Sun drawed" << endl;
}


// Desenha um texto na janela GLUT
void drawBoat() {
	glPushMatrix();
	glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT );
		glEnable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, 0);
		glScalef(3,3,3);
		glRotatef(boat_yaw,0,1,0);
		glRotatef(boat_pitch,1,0,0);
		//boat window
		glColor3f(0.18,0.18,0.18);			// 18% grey windows
		// sets shininess concentration
		GLfloat shine = 30;
		glMaterialf(GL_FRONT, GL_SHININESS, shine);
		glCallList(BOAT_WINDOW);
		// boat hull
		glColor3f(0.98,0.98,0.98);    // 98% grey hull
		// sets material diffusion
		GLfloat ambient[4]={ 0.1, 0.1, 0.1, 1.0};
		glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
		// sets material diffusion
		GLfloat diffusion[4]={ 0.4, 0.4, 0.4, 1.0};
		glMaterialfv(GL_FRONT, GL_DIFFUSE, diffusion);
		// sets material specularity
		GLfloat specularity[4]={0.7,0.7,0.7,1.0};
		glMaterialfv(GL_FRONT, GL_SPECULAR, specularity);
		// sets shininess concentration
		shine = 12;
		glMaterialf(GL_FRONT, GL_SHININESS, shine);
		glCallList(BOAT_HULL);
		glPopAttrib();
	glPopMatrix();
	glBindTexture(GL_TEXTURE_2D, 0);
	if (LISTENING)		cout << " boat drawed" << endl;
}


// Callback function for display
void display(void){
	// Clear color and depth buffer
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();
		// enables flat/wireframe representation
		if (FLATREPRESENTATION)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		// enables lightning
		if (ILLUMINATION){
			lightning();
			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
		}
		else {
			glDisable(GL_LIGHTING);
			glDisable(GL_LIGHT0);
		}
		// enables multisampling antialiasing
		if (MSAA){
			glEnable(GL_MULTISAMPLE);
			glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
		}
		else {
			glDisable(GL_MULTISAMPLE);
		}
		// Habilita o modelo de colorizacao de Gouraud
		if (SHADING) {
			glEnable(GL_SMOOTH);
			glShadeModel(GL_SMOOTH);
		}
		else {
			glShadeModel(GL_FLAT);
			glDisable(GL_SMOOTH);
		}
		glPushMatrix();
			glTranslatef(boat.position.Z , -boat.position.Y,	boat.position.X);
			// if (textured)
			if (SHOWSKYBOX)			drawSkybox();
			if (SHOWSUN)			drawSun();
			if (SHOWEXTRAS)			drawExtras();
			if (SHOWLANDSCAPE)		drawLandscape();
			if (SHOWWATER)       	drawWaterMesh();
		glPopMatrix();
		if (SHOWBOAT)
			drawBoat();
		drawLabels();
	glPopMatrix();
	glEnable(GL_TEXTURE_2D);
	// Troca os buffers da janela actual (se double buffering habilitado)
	glutSwapBuffers();
}


// Inicializa parmetros de rendering
void init (void){
	listindex = glGenLists(15);
	loadObjects();
	textures();
	watermesh::resetMesh();
	resetGame();
 	// Especifica que a cor de fundo da janela será preta
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	// Habilita o depth-buffering
	glEnable(GL_DEPTH_TEST);
	// habilitar as funcoes de BLEND para transparencias
	// remove back faces
	glCullFace(GL_BACK);
	// hint quality of color and texture coordinate interpolation
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	// set game start time
	// registers a timer callback to be triggered every TIMER milliseconds
    glutTimerFunc (TIMER, timer, 0);
}

// Funcao usada para especificar o volume de visualizacao
void displaySettings(void){
	// Especifica sistema de coordenadas de projecao
	glMatrixMode(GL_PROJECTION);
	// Loads the identity matrix into the current transformation matrix. Used to reset the current transformation matrix before performing a transformation.
	glLoadIdentity();
	// Especifica a projecao perspectiva
	gluPerspective((GLdouble)camera.Zoom,fAspect,0.4,22000);
	// Especifica sistema de coordenadas do modelo
	glMatrixMode(GL_MODELVIEW);
	// Inicializa sistema de coordenadas do modelo
	glLoadIdentity();
	// Especifica posio do observador e do alvo
	gluLookAt((GLdouble)camera.Position.X, (GLdouble)camera.Position.Y, 	(GLdouble)camera.Position.Z,
			  (GLdouble)camera.Right.X, (GLdouble)camera.Right.Y,	(GLdouble)camera.Right.Z,
			  (GLdouble)camera.Up.X,  	(GLdouble)camera.Up.Y,	  	(GLdouble)camera.Up.Z);
}


// callback function when window size is changed
void reshape(GLsizei w, GLsizei h){
	// avoid division by zero
	if ( h == 0 ) h = 1;
	screenWidth = w;
	screenHeight = h;
	// specifies viewport size
	glViewport(0, 0, w, h);
	// aspect ratio
	fAspect = (GLfloat)w/(GLfloat)h;
	displaySettings();
}


// Gerenciamento do menu com as opes de cores
void menuRepresentation(int op){
	switch(op) {
	case 0:
		FLATREPRESENTATION = false;
		break;
	case 1:
		FLATREPRESENTATION = true;
		break;
	}
	if (LISTENING)		cout << "flatrepresentation set to " << FLATREPRESENTATION << endl;
}


// Gerenciamento do menu com as opes de cores
void menuIllumination(int op){
	switch(op) {
	case 0:
		ILLUMINATION = false;
		break;
	case 1:
		ILLUMINATION = true;
		break;
	}
	if (LISTENING)		cout << "ILLUMINATION set to " << ILLUMINATION << endl;
}

// Gerenciamento do menu com as opes de cores
void menuShading(int op){
	switch(op) {
	case 0:
		SHADING = false;

		break;
	case 1:
		SHADING = true;
		break;
	}
    if (LISTENING)     	cout << "SHADING set to " << SHADING << endl;
}

// Gerenciamento do menu com as opes de cores
void menuAntialiasing(int op){
	switch(op) {
	case 0:
		MSAA = false;
		break;
	case 1:
		MSAA = true;
		break;
	}
	if (LISTENING)		cout << "msaa set to " << MSAA << endl;
}

// Gerenciamento do menu com as opes de cores
void menuTexture(int op){
	switch(op) {
	case 0:
		TEXTURED = false;
		break;
	case 1:
		TEXTURED = true;
		break;
	}
	if (LISTENING)		cout << "textured set to " << TEXTURED << endl;
}

// Water options menu
void menuReset(int op){
	switch(op) {
	case 0 :
		watermesh::resetMesh();
		break;
	case 1 :
		camera.resetCamera();
		displaySettings();
		break;
	case 2 :
		resetGame();
		camera.resetCamera();
		watermesh::resetMesh();
		displaySettings();
		break;
	}
}
// Gerenciamento do menu principal
void menuPrincipal(int op){
	switch(op) {
	case 1 :
		SHOWBOAT 		= !SHOWBOAT;
		break;
	case 2 :
		SHOWLANDSCAPE  	= !SHOWLANDSCAPE;
		break;
	case 3 :
		SHOWWATER  		= !SHOWWATER;
		break;
	case 4 :
		SHOWEXTRAS  	= !SHOWEXTRAS;
		break;
	case 5 :
		SHOWSUN  		= !SHOWSUN;
		break;
	case 6 :
		SHOWSKYBOX  	= !SHOWSKYBOX;
		break;
	}
}


// Criacao do Menu
void createMenu() {
	int menu, submenu1, submenu2, submenu3, submenu4, submenu5, submenu6;
	submenu1 = glutCreateMenu(menuRepresentation);
	glutAddMenuEntry("Flat",	1);
	glutAddMenuEntry("Wireframe",0);
	submenu2 = glutCreateMenu(menuIllumination);
	glutAddMenuEntry("On",	1);
	glutAddMenuEntry("Off",	0);
	submenu3 = glutCreateMenu(menuShading);
	glutAddMenuEntry("On",	1);
	glutAddMenuEntry("Off",	0);
	submenu4 = glutCreateMenu(menuAntialiasing);
	glutAddMenuEntry("On",	1);
	glutAddMenuEntry("Off",	0);
	submenu5 = glutCreateMenu(menuTexture);
	glutAddMenuEntry("On",	1);
	glutAddMenuEntry("Off",	0);
	submenu6 = glutCreateMenu(menuReset);
	glutAddMenuEntry("reset water",	0);
	glutAddMenuEntry("reset camera",1);
	glutAddMenuEntry("reset game",	2);
	menu = glutCreateMenu(menuPrincipal);
	glutAddSubMenu("Flat / Wireframe representation",	submenu1);
	glutAddSubMenu("Illumination",						submenu2);
	glutAddSubMenu("Gouraud Shading",					submenu3);
	glutAddSubMenu("Antialiasing",						submenu4);
	glutAddSubMenu("Textured",							submenu5);
	glutAddSubMenu("Reset",							    submenu6);
	glutAddMenuEntry("toggle boat",		1);
	glutAddMenuEntry("toggle landscape",2);
	glutAddMenuEntry("toggle water",	3);
	glutAddMenuEntry("toggle extras",	4);
	glutAddMenuEntry("toggle sun",		5);
	glutAddMenuEntry("toggle sky box",	6);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}


// callback function called when mouse moves over a GLUT window
void moveMouse(int x, int y){
	lastMouseX = x;
	lastMouseY = y;
}

// callback function for mouse events managing while moving over GLUT window while pressing mouse key
void moveMouseKeyPressed(int x, int y){
	dx = x - lastMouseX;
	dy = -y + lastMouseY;
	lastMouseX = x;
	lastMouseY = y;
    dx *= camera.MouseSensitivity;
    dy *= camera.MouseSensitivity;
    camera.Yaw   += dx;
    camera.Pitch += dy;
    // Make sure that when pitch is out of bounds, screen doesn't get flipped
    if (camera.Pitch  > 360.0f) //89.0f
    	camera.Pitch -= 360.0f;
    if (camera.Pitch  <   0.0f)
       	camera.Pitch += 360.0f;
    // Update Front, Right and Up Vectors using the updated Euler angles
    camera.updateCameraVectors();
	displaySettings();
}

// callback function for mouse events managing
void mouseManager(int button, int state, int x, int y){
	//sprintf(texto[3], "x, y, camera.Yaw boatpos x,y,(%d,%d,%f,%f)", x, y,camera.Yaw ,boat.position.Z);
	GLboolean rotate = false;
	if (button == GLUT_RIGHT_BUTTON)
		if (state == GLUT_DOWN)
			createMenu();
	if (button == 3 )
		if (state == GLUT_DOWN)	{
			rotate = true;
			direction =  1;
		}
	if (button == 4 )
		if (state == GLUT_DOWN)	{
			rotate = true;
			direction =  -1;
		}
	if (rotate){
		camera.Yaw += (direction * camera.MovementSpeed);
		if (camera.Yaw >= 360.0f)
			camera.Yaw -= 360.0f;
		if (camera.Yaw < 0.0f)
			camera.Yaw += 360.0f;
		camera.updateCameraVectors();
		displaySettings();
	}
}

// callback function for keyboard events managing
void keyboard(unsigned char key, int x, int y){
	switch (key) {
	case 27:  				// ESC - Sair do Programa
		killTextures();
		exit(0);
		break;

	case 'a' : case 'A' :	// Aumentar a velocidade
		if (boat_speed <= 20 * SPEED_INCREMENT) {
			boat_speed += SPEED_INCREMENT;;
		}
		break;
	case 'z' : case 'Z' :	// Diminuir a velocidade
		if (boat_speed >= SPEED_INCREMENT)
			boat_speed -= SPEED_INCREMENT;
		break;
	case 'l' : case 'L' :  	// Esconder/Mostrar Labels
		LABELS = ! LABELS;
		break;
	case 'p' : case 'P' :	// Esconder/Mostrar Labels
		if (!RUNNING) {
			RUNNING	= true;
			resetGame();
		}
		break;
	case '+' : // Zoom In
		if (camera.Zoom > 0.1f) {
			camera.CameraZomm(1);
			camera.updateCameraVectors();
		}
		break;
	case '-' : // Zoom Out
		if (camera.Zoom <= 128.0f) {
			camera.CameraZomm(0);
			camera.updateCameraVectors();
		}
		break;
	}
	displaySettings();
}

// callback function to manage special keys events
void specialKeys(int key, int x, int y){
	// Camara 1
	if(key == GLUT_KEY_F1)		{
		camera.Pitch = 3.0f;
		camera.Yaw = 90.0f;
		camera.updateCameraVectors();
		displaySettings();
	}
	// Camara 2
	if(key == GLUT_KEY_F2)		{
		camera.Pitch = 3.0f;
		camera.Yaw = 0.0f;
		camera.updateCameraVectors();
		displaySettings();
	}
	// Camara 3
	if(key == GLUT_KEY_F3)		{
		camera.Pitch = 3.0f;
		camera.Yaw = -90.0f;
		camera.updateCameraVectors();
		displaySettings();
	}
	// Camara 4
	if(key == GLUT_KEY_F4)		{
		camera.Pitch = 177.0f;
		camera.Yaw = 0.0f;
		camera.updateCameraVectors();
		displaySettings();
	}
	// Camara 5
	if(key == GLUT_KEY_F5)		{
		camera.Pitch = 90.0f;
		camera.Yaw = 0.0f;
		camera.updateCameraVectors();
		displaySettings();
	}
	// Camara 6
	if(key == GLUT_KEY_F6)		{
		camera.Pitch = -90.0f;
		camera.Yaw = 0.0f;
		camera.updateCameraVectors();
		displaySettings();
	}
	if(key == GLUT_KEY_LEFT)	{	// Left function key 	- Rodar para a esquerda
		boat_yaw += 5;
		if(boat_yaw > 360)
			boat_yaw  -= 360;
		boat.velocity = Vector3((cos(5 * DEG_TO_RADIANS) * boat.velocity.X) - (sin(5 * DEG_TO_RADIANS) * boat.velocity.Z),
								0.0f,
								(sin(5 * DEG_TO_RADIANS) * boat.velocity.X) + (cos(5 * DEG_TO_RADIANS) * boat.velocity.Z));
	}
	if(key == GLUT_KEY_RIGHT)	{	// Right function key	- Rodar para a direita
		boat_yaw -= 5;
		if(boat_yaw  < 0)
			boat_yaw += 360;
		boat.velocity = Vector3((cos(355 * DEG_TO_RADIANS) * boat.velocity.X) - (sin(355 * DEG_TO_RADIANS) * boat.velocity.Z),
								0.0f,
								(sin(355 * DEG_TO_RADIANS) * boat.velocity.X) + (cos(355 * DEG_TO_RADIANS) * boat.velocity.Z));

	}
	if(key == GLUT_KEY_UP)		{	// Up function key		- Inclinar para baixo
		boat_pitch += 5;
		if(boat_pitch >= 360)
			boat_pitch -= 360;
	}
	if(key == GLUT_KEY_DOWN)	{	// Down fuction key		- Inclinar para cima
		boat_pitch -= 5;
		if(boat_pitch<0)
			boat_pitch += 360;
	}
}


// Programa Principal
int main(int argc, char** argv){
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glutInitWindowSize(screenWidth,screenHeight);
	glutCreateWindow("bRa-thing 3D");
	// initial settings
	GLenum err = glewInit();
	if (GLEW_OK != err)
		cout << "glew initialization error" << endl;
	init();
	//Sets the display callback for the current window.
	glutDisplayFunc(display);
	// sets window size changing callback
	glutReshapeFunc(reshape);
	// window mouse callback when the mouse moves within the window while no mouse buttons are pressed.
	glutPassiveMotionFunc(moveMouse);
	// window callback when the mouse moves within the window while one or more mouse buttons are pressed.
	glutMotionFunc(moveMouseKeyPressed);
	// window mouse callback when a user presses and releases mouse buttons.
	glutMouseFunc(mouseManager);
	// keyboard callback for the current window
	glutKeyboardFunc(keyboard);
	// keyboard special keys callback for the current window
	glutSpecialFunc(specialKeys);
	// enters GLUT execution loop
	glutMainLoop();
}
