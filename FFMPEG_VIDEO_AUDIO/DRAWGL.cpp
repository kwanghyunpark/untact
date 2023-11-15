#include "DRAWGL.h"


DRAWGL::DRAWGL(void)
{
}


DRAWGL::~DRAWGL(void)
{
}


// ========== By HHKim =======================
void DRAWGL::drawHapticScene(double* pos1, double* ballState)
{
	// unit : meter

	Rect1.x = 0;
	Rect1.y = 0;
	Rect1.width = 640;
	Rect1.height = 480;

	double scale = Rect1.width/1.0; // We scale 1.0 meter to 640

	drawViewportOutline(Rect1);

	// ofPushView() / ofPopView() are automatic
	camera1.begin(Rect1);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	//----Light-----
	setProperLight();
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);


	//----Camera Pos----
	//camera1.setPosition(ofVec3f(3.0,0.5,0.0)*scale);  // distance between camera and target object is 80cm
	//camera1.lookAt(ofVec3f(0.0f,-0.0f,-2.0f)*scale, ofVec3f(0.0f,1.0f,0.0f));

	camera1.setPosition(ofVec3f(0.0,0.7,1.2)*scale);  // distance between camera and target object is 80cm
	camera1.lookAt(ofVec3f(0.0f,-0.3f,-5.0f)*scale, ofVec3f(0.0f,1.0f,0.0f));
	//ofTranslate(0,0,0);
	ofDisableLighting();
	ofEnableLighting();

	// draw floor
	ofPushMatrix();
	GLUquadricObj *floorObj = gluNewQuadric(); 
	gluQuadricDrawStyle(floorObj, GLU_FILL); 
	ofTranslate(0*scale, -1.0*scale, -2.5*scale);
	ofSetHexColor(0x00A040);
	glScalef(200,2,500);
	glutSolidCube(0.01*scale);
	ofPopMatrix();


	// draw shutter
	ofPushMatrix();
	GLUquadricObj *shutterObj = gluNewQuadric(); 
	gluQuadricDrawStyle(shutterObj, GLU_FILL); 
	ofTranslate(0*scale, -0.5*scale, -5.0*scale);
	ofSetHexColor(0xFFFF00);
	glScalef(10,100,10);
	glutSolidCube(0.01*scale);
	ofPopMatrix();



	// drawing the right-hand side falcon
	ofPushMatrix();
	GLUquadricObj *gluQuadps1;
	gluQuadps1 = gluNewQuadric(); 
	gluQuadricDrawStyle(gluQuadps1, GLU_FILL); 

	ofTranslate(pos1[0]*scale, pos1[1]*scale, pos1[2]*scale);
	//printf("%f, %f, %f \n",pos1[0]*scale, pos1[1]*scale, pos1[2]*scale);

	ofSetHexColor(0xFF0000);
	gluSphere( gluQuadps1, 0.05*scale, 20, 20 ); // radius of sphere size is 2cm 
	ofPopMatrix();


	// drawing the left-hand side falcon
	ofPushMatrix();
	GLUquadricObj *gluQuadps2;
	gluQuadps2 = gluNewQuadric(); 
	gluQuadricDrawStyle(gluQuadps2, GLU_FILL); 

	ofTranslate(ballState[0]*scale, ballState[1]*scale, ballState[2]*scale);

	ofSetHexColor(0x0000FF);
	gluSphere( gluQuadps2, 0.05*scale, 20, 20 );
	ofPopMatrix();

	ofSetMatrixMode(OF_MATRIX_MODELVIEW);

	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT0);

	glDisable(GL_DEPTH_TEST);

	camera1.end();
}
// -----------------------------------------



void DRAWGL::bdraw()
{
	ofSetColor(255);

	ofPushMatrix();
	ofTranslate(640/2, 480/2, 0);

	billboardShader.begin();

	ofEnablePointSprites();
	texture.getTextureReference().bind();
	billboards.draw();
	texture.getTextureReference().unbind();
	ofDisablePointSprites();

	billboardShader.end();

	ofPopMatrix();

}
void DRAWGL:: setProperLight()
{
	static int  mat_f = 1;
	GLfloat     mat_amb_diff[]  = {0.9, 0.9, 0.0, 1.0};
	GLfloat     mat_specular[]  = {0.5, 0.5, 0.5, 1.0};
	GLfloat     mat_shininess[] = {5.0};
	GLfloat     light_ambient[] = { 0.01, 0.01, 0.01, 1.0 };
	GLfloat     light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat     light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat     light_position[] = { 100.0, 500.0, 700.0, 1.0 };

	if( mat_f ) {
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_amb_diff);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);	
		glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);
		mat_f = 0;
	}

	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
}
void DRAWGL::drawViewportOutline(const ofRectangle & viewport){
	ofPushStyle();
	ofFill();
	ofSetColor(0);
	ofSetLineWidth(0);
	ofRect(viewport);
	ofNoFill();
	ofSetColor(25);
	ofSetLineWidth(1.0f);
	ofRect(viewport);
	ofPopStyle();

}
void DRAWGL::Canvas1(double* pos1, double* pos2, double* gBallPosition, double gBallRadius)
{
	Rect1.x = 0;
	Rect1.y = 0;
	Rect1.width = 640;
	Rect1.height = 480;

	drawViewportOutline(Rect1);


	// ofPushView() / ofPopView() are automatic
	camera1.begin(Rect1);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	//----Light-----
	setProperLight();
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	//----Camera Pos----
	camera1.setPosition(ofVec3f(60,150,-800));
	camera1.lookAt(ofVec3f(0.0f,0.0f,0.0f), ofVec3f(0.0f,1.0f,0.0f));
	//ofTranslate(0,0,0);
	ofDisableLighting();
	ofEnableLighting();

	ofPushMatrix();
	GLUquadricObj *gluQuadps1;
	gluQuadps1 = gluNewQuadric(); 
	gluQuadricDrawStyle(gluQuadps1, GLU_FILL); 

	ofTranslate(pos1[0], pos1[1], pos1[2]);

	ofSetHexColor(0xFF0000);
	gluSphere( gluQuadps1, 10, 200, 200 );
	ofPopMatrix();

// 	ofPushMatrix();
// 	GLUquadricObj *gluQuadps2;
//	gluQuadps2 = gluNewQuadric(); 
//	gluQuadricDrawStyle(gluQuadps2, GLU_FILL); 
// 	ofTranslate(pos2[0], pos2[1], pos2[2]);
// 	ofSetHexColor(0x0000FF);
// 	gluSphere( gluQuadps2, 10, 200, 200 );
// 	ofPopMatrix();

	ofPushMatrix();
	GLUquadricObj *gluQuadps3;
	gluQuadps3 = gluNewQuadric(); 
	gluQuadricDrawStyle(gluQuadps3, GLU_FILL); 
	ofTranslate(gBallPosition[0], gBallPosition[1], gBallPosition[2]);

	ofSetHexColor(0x00FFFF);
	glutSolidCube(gBallRadius*2);

	ofPopMatrix();


	ofPushMatrix();
	ofSetHexColor(0xFFFFFF);
	glScalef(4,1,1);
	glutWireCube(gBallRadius*2);
	ofPopMatrix();

	ofSetMatrixMode(OF_MATRIX_MODELVIEW);

	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT0);

	glDisable(GL_DEPTH_TEST);


	camera1.end();
}

void DRAWGL::bupdate()
{
	float t = (ofGetElapsedTimef()) * 0.9f;
	float div = 25.0;

	for (int i=0; i<NUM_BILLBOARDS; i++) {

		// noise 
		ofVec3f vec(ofSignedNoise(t, billboards.getVertex(i).y/div, billboards.getVertex(i).z/div),
			ofSignedNoise(billboards.getVertex(i).x/div, t, billboards.getVertex(i).z/div),
			ofSignedNoise(billboards.getVertex(i).x/div, billboards.getVertex(i).y/div, t));

		vec *= 10 * ofGetLastFrameTime();
		billboardVels[i] += vec;
		billboards.getVertices()[i] += billboardVels[i]; 
		billboardVels[i] *= 0.94f; 
		billboards.setNormal(i,ofVec3f(12 + billboardSizeTarget[i] * ofNoise(t+i),0,0));
	}

}

void DRAWGL::bset()
{

	billboards.getVertices().resize(NUM_BILLBOARDS);
	billboards.getColors().resize(NUM_BILLBOARDS);
	billboards.getNormals().resize(NUM_BILLBOARDS,ofVec3f(0));

	// ------------------------- billboard particles
	for (int i=0; i<NUM_BILLBOARDS; i++) {

		billboardVels[i].set(ofRandomf(), -1.0, ofRandomf());
		billboards.getVertices()[i].set(ofRandom(-640, 640), 
			ofRandom(-640, 640), 
			ofRandom(-100, 500));

		billboards.getColors()[i].set(ofColor::fromHsb(ofRandom(96, 160), 255, 255));
		billboardSizeTarget[i] = ofRandom(4, 100);

	}


	billboards.setUsage( GL_DYNAMIC_DRAW );
	billboards.setMode(OF_PRIMITIVE_POINTS);

	billboardShader.load("Billboard");

	// we need to disable ARB textures in order to use normalized texcoords
	ofDisableArbTex();
	texture.loadImage("dot.png");
	ofEnableAlphaBlending();
}