
#pragma comment (lib,"glut32.lib")
#pragma comment (lib,"glaux.lib")

#include <windows.h>
#include <gl/gl.h>
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#define MAX_TEXT_SIZE		600			/* maximum text string length */ // original text size 255
#define GAP					1000		/* ms gap between updates */




//==============================================================================
//Map and movement variables
const int sMax=6, m=40, mm=m+1, direction_parts=36;
int cur_direction=0;
double distance=4.;
//==============================================================================


//==============================================================================
//texture Variables
GLuint texture[2];


struct Image {

    unsigned long sizeX;

    unsigned long sizeY;

    char *data;

};

typedef struct Image Image;


#define checkImageWidth 64

#define checkImageHeight 64


GLubyte checkImage[checkImageWidth][checkImageHeight][3];

void makeCheckImage(void){

    int i, j, c;

    for (i = 0; i < checkImageWidth; i++) {

        for (j = 0; j < checkImageHeight; j++) {

            c = ((((i&0x8)==0)^((j&0x8)==0)))*255;

            checkImage[i][j][0] = (GLubyte) c;

            checkImage[i][j][1] = (GLubyte) c;

            checkImage[i][j][2] = (GLubyte) c;
 
        }

    }

}
//==============================================================================



//==============================================================================
//Timer Variables
static int g_counter = 300;
static int j=0;
//==============================================================================

//==============================================================================
//Texture Loading And Verification
int ImageLoad(char *filename, Image *image) {

    FILE *file;

    unsigned long size; //Size of the image in bytes.

    unsigned long i; //Standard counter.

    unsigned short int planes; //Number of planes in image (must be 1)

    unsigned short int bpp; //Number of bits per pixel (must be 24)

    char temp; //Temporary color storage for bgr-rgb conversion.

    //Make sure the file is there.
    if ((file = fopen(filename, "rb"))==NULL){
        printf("File Not Found : %s\n",filename);
        return 0;
    }

    //Seek through the bmp header, up to the width/height:
    fseek(file, 18, SEEK_CUR);

    //Read the width
    if ((i = fread(&image->sizeX, 4, 1, file)) != 1) {
        printf("Error reading width from %s.\n", filename);
        return 0;
    }

    //Read the height
    if ((i = fread(&image->sizeY, 4, 1, file)) != 1) {
        printf("Error reading height from %s.\n", filename);
        return 0;
    }
    //Calculate the size (assuming 24 bits or 3 bytes per pixel).
    size = image->sizeX * image->sizeY * 3;

    //Read the planes
    if ((fread(&planes, 2, 1, file)) != 1) {
        printf("Error reading planes from %s.\n", filename);
        return 0;
    }

    if (planes != 1) {
        printf("Planes from %s is not 1: %u\n", filename, planes);
        return 0;
    }

    //Read the bitsperpixel
    if ((i = fread(&bpp, 2, 1, file)) != 1) {
        printf("Error reading bpp from %s.\n", filename);
        return 0;
    }

    if (bpp != 24) {
        printf("Bpp from %s is not 24: %u\n", filename, bpp);
        return 0;
    }

    //Seek past the rest of the bitmap header.
    fseek(file, 24, SEEK_CUR);

    //Read the data.
    image->data = (char *) malloc(size);
    if (image->data == NULL) {
        printf("Error allocating memory for color-corrected image data");
        return 0;
    }

    if ((i = fread(image->data, size, 1, file)) != 1) {
        printf("Error reading image data from %s.\n", filename);
        return 0;
    }

    for (i=0;i<size;i+=3) { // reverse all of the colors. (bgr -> rgb)
        temp = image->data[i];
        image->data[i] = image->data[i+2];
        image->data[i+2] = temp;
    }
    return 1;
}

//Loads Floor Texture
Image * loadTexture(){
    Image *image1;
    //Allocate space for texture
    image1 = (Image *) malloc(sizeof(Image));
    if (image1 == NULL) {
        printf("Error allocating space for image");
        exit(0);
    }
    if (!ImageLoad("floortex.bmp", image1)) {
        exit(1);
    }
    return image1;
}

//Loads Wall Texture
Image * loadTexture2(){
    Image *image2;
    //Allocate space for texture
    image2 = (Image *) malloc(sizeof(Image));
    if (image2 == NULL) {
        printf("Error allocating space for image");
        exit(0);
    }
    if (!ImageLoad("walltex.bmp", image2)) {
        exit(1);
    }
    return image2;
}
//==============================================================================


//==============================================================================
//DrawText
//Draws the specified message in both
//Raster and stroke text
void drawText(const char * message, GLfloat x, GLfloat y)
{
//raster pos sets the current raster position
//mapped via the modelview and projection matrices	 
	glRasterPos2f(x, y);

//write using bitmap and stroke chars	 
	while (*message) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *message++);
	}
}
//==============================================================================

//==============================================================================
unsigned textureId = -1, texFloor = 0; //Does Nothing

//==============================================================================
//Win and Path Variables
int glWin,pathLen,myLen=0;
//==============================================================================

//==============================================================================
//Spawns free space for Player and objects
struct Tpos
{char x,y;};
struct Player
{
    int x,y,z;
    int dx,dz;
    bool isGo;
}player,goal,badguy1,badguy2,badguy3,badguy4,food1,food2,food3,food4;

struct Maps
{int x,z;} map;

char data[m+2][m+2], cp[m+2][m+2];

float	lightAmb [] = { 0.03, 0.03, 0.03 };
float	lightDif [] = { 0.95, 0.95, 0.95 };
float	lightPos [] = { (int)m/2,  7,  (int)m/2 };
//==============================================================================


//==============================================================================
//Creates the Floor
void display(void);
void halt(bool f=false);

void drawFloor(GLfloat x1, GLfloat x2, GLfloat z1, GLfloat z2, unsigned texture=texFloor)
{
    //glBindTexture ( GL_TEXTURE_2D, texture );
    //glBindTexture(GL_TEXTURE_2D, texture);
    glBegin(GL_POLYGON);
        glNormal3f( 0.0, 1.0, 0.0);
        glTexCoord2f(0,0);
        glVertex3f( x1, 0, z2 );
        glTexCoord2f(1,0);
        glVertex3f( x2, 0, z2 );
        glTexCoord2f(1,1);
        glVertex3f( x2, 0, z1 );
        glTexCoord2f(0,1);
        glVertex3f( x1, 0, z1 );
        
    glEnd();
    
}
//==============================================================================

//==============================================================================
//Coords For Modeling Cube
GLfloat vertices[] = {1,1,1,  -1,1,1,  -1,-1,1,  1,-1,1,        // v0-v1-v2-v3
                      1,1,1,  1,-1,1,  1,-1,-1,  1,1,-1,        // v0-v3-v4-v5
                      1,1,1,  1,1,-1,  -1,1,-1,  -1,1,1,        // v0-v5-v6-v1
                      -1,1,1,  -1,1,-1,  -1,-1,-1,  -1,-1,1,    // v1-v6-v7-v2
                      -1,-1,-1,  1,-1,-1,  1,-1,1,  -1,-1,1,    // v7-v4-v3-v2
                      1,-1,-1,  -1,-1,-1,  -1,1,-1,  1,1,-1};   // v4-v7-v6-v5

//==============================================================================

//==============================================================================
//Creates Walls For Maze
void drawBox (GLint j, GLint i, unsigned texture=textureId)
{
    GLfloat x1=i, x2=i+1, y1=0, y2=1, z1=j, z2=j+1;
    //glBindTexture ( GL_TEXTURE_2D, texture );
    //glBindTexture(GL_TEXTURE_2D, texture[0]);
    glColor3f(1,1,1);
    if ((j==map.z+1)||(data[j-1][i]!='x'))
    {
    glBegin(GL_POLYGON); // Back
        glNormal3f( 0.0, 0.0, -1.0);
        glTexCoord2f(0,0);
        glVertex3f( x2, y1, z1 );
        glTexCoord2f(1,0);
        glVertex3f( x1, y1, z1 );
        glTexCoord2f(1,1);
        glVertex3f( x1, y2, z1 );
        glTexCoord2f(0,1);
        glVertex3f( x2, y2, z1 );
    glEnd();
    }
    if ((j==map.z-1)||(data[j+1][i]!='x'))
    {
    glBegin(GL_POLYGON); // Front
    	//glColor3f(1,1,1);
        glNormal3f( 0.0, 0.0, 1.0);
        glTexCoord2f(0,0);
        glVertex3f( x1, y1, z2 );
        glTexCoord2f(1,0);
        glVertex3f( x2, y1, z2 );
        glTexCoord2f(1,1);
        glVertex3f( x2, y2, z2 );
        glTexCoord2f(0,1);
        glVertex3f( x1, y2, z2 );
    glEnd();
    }
    if ((i>0)&&(data[j][i-1]!='x'))
    {
    glBegin(GL_POLYGON); // Left
    	//glColor3f(1,1,1);
        glNormal3f( -1.0, 0.0, 0.0);
        glTexCoord2f(0,0);
        glVertex3f( x1, y1, z1 );
        glTexCoord2f(1,0);
        glVertex3f( x1, y1, z2 );
        glTexCoord2f(1,1);
        glVertex3f( x1, y2, z2 );
        glTexCoord2f(0,1);
        glVertex3f( x1, y2, z1 );
    glEnd();
    }
    if ((i<map.x)&&(data[j][i+1]!='x'))
    {
    glBegin(GL_POLYGON); // Right
        //glColor3f(1,1,1);
		glNormal3f( 1.0, 0.0, 0.0);
        glTexCoord2f(0,0);
        glVertex3f( x2, y1, z2 );
        glTexCoord2f(1,0);
        glVertex3f( x2, y1, z1 );
        glTexCoord2f(1,1);
        glVertex3f( x2, y2, z1 );
        glTexCoord2f(0,1);
        glVertex3f( x2, y2, z2 );
    glEnd();
    }
    glBegin(GL_POLYGON); // Top
    	//glColor3f(1,1,1);
        glNormal3f( 0.0, 1.0, 0.0);
        glTexCoord2f(0,0);
        glVertex3f( x1, y2, z2 );
        glTexCoord2f(1,0);
        glVertex3f( x2, y2, z2 );
        glTexCoord2f(1,1);
        glVertex3f( x2, y2, z1 );
        glTexCoord2f(0,1);
        glVertex3f( x1, y2, z1 );
    glEnd();
    glBindTexture ( GL_TEXTURE_2D, texture );
}
//==============================================================================

//==============================================================================
//Animates player movement, winning the game, losing the game,food that gives time, monsters that takes away time
void animate()
{
    if ((player.x == goal.x)&&(player.z==goal.z))
    {
        halt(true);
    };
    if (player.isGo==true)
    {
        if (player.dx>0)	player.dx+=1; else //positive X Axis
        if (player.dz>0)	player.dz+=1; else //positive Z Axis
        if (player.dx<0)	player.dx-=1; else //Negative X Axis
        if (player.dz<0)	player.dz-=1;      //Negative Z Axis
    	if ((player.dx>=sMax)||(player.dz>=sMax))
        {
            player.isGo=false;                
            if (player.dx>0)	player.x+=1;
            if (player.dz>0)	player.z+=1;
            player.dx=0; player.dz=0;
        }else
        if ((player.dx<=-sMax)||(player.dz<=-sMax))
        {
            player.isGo=false;
            if (player.dx<0)	player.x-=1;
            if (player.dz<0)	player.z-=1;
            player.dx=0; player.dz=0;
        }
    }
    
    if ((player.x == food1.x)&&(player.z==food1.z))                             //food1
    {
    		j=1;
    };
    
    if ((player.x == food2.x)&&(player.z==food2.z))                             //food2
    {
    		j=1;
    };
    
    if ((player.x == food3.x)&&(player.z==food3.z))                             //food3
    {
    		j=1;
    };
    
    if ((player.x == food4.x)&&(player.z==food4.z))                             //food4
    {
    		j=1;
    };
    
    //Badguys Steal Time
    if ((player.x ==badguy1.x )&&(player.z==badguy1.z))                         //badguy1
    {
    	j=2;
    };
    
        if ((player.x ==badguy2.x )&&(player.z==badguy2.z))                     //badguy2
    {
    	j=2;
    };
    
        if ((player.x ==badguy3.x )&&(player.z==badguy3.z))                     //badguy3
    {
    	j=2;
    };
    
        if ((player.x ==badguy4.x )&&(player.z==badguy4.z))                     //badguy4
    {
    	j=2;
    };
    glutPostRedisplay();
}
//==============================================================================

//==============================================================================
//loads texture to objects
void init()
{
    glClearColor( 0.0, 0.0, 0.0, 1.0 );
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_TEXTURE_2D );
    glDepthFunc(GL_LESS);
    
	Image *image1 = loadTexture();
	Image *image2 = loadTexture2();
	if(image1 == NULL){
        printf("Image was not returned from loadTexture\n");
        exit(0);
    }

    makeCheckImage();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	    glGenTextures(2, texture);

	//Create texture
	//Floor
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); //scale linearly when image bigger than texture
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); //scale linearly when image smalled than texture
    glTexImage2D(GL_TEXTURE_2D, 0, 3, image1->sizeX, image1->sizeY, 0,
    GL_RGB, GL_UNSIGNED_BYTE, image1->data);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	//Walls
	glBindTexture(GL_TEXTURE_2D, texture[1]);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); //scale linearly when image bigger than texture
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); //scale linearly when image smalled than texture
    glTexImage2D(GL_TEXTURE_2D, 0, 3, image2->sizeX, image2->sizeY, 0,
    GL_RGB, GL_UNSIGNED_BYTE, image2->data);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		
	glEnable( GL_CULL_FACE );
    glPixelStorei ( GL_PACK_ALIGNMENT, 1 );
    glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );
    glShadeModel (GL_SMOOTH);
    glEnable ( GL_LIGHT0 );
    glEnable ( GL_LIGHTING );
    player.dx=0; player.dz=0; player.isGo=false;
    glEnable(GL_COLOR_MATERIAL);
}
//==============================================================================

//==============================================================================
//drawing text on screen
//Shows Timer   
void drawString(const char *string, GLfloat x, GLfloat y)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();   
	     
    glLoadIdentity();   
    int w = glutGet( GLUT_WINDOW_WIDTH );
    int h = glutGet( GLUT_WINDOW_HEIGHT );;
    glOrtho(1.0, w, 0.0, h, 1.0, -1.0f);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable( GL_DEPTH_TEST ); 

    glDisable( GL_LIGHTING );
	glColor3f(1, 1, 1);
    glRasterPos2i(x, y);
    void *font = GLUT_BITMAP_HELVETICA_18; 
    for (const char* c=string; *c != '\0'; c++) 
    {
        glutBitmapCharacter(font, *c); 
    }

    glEnable( GL_LIGHTING );

    glEnable (GL_DEPTH_TEST);     

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();  
}
//==============================================================================


//==============================================================================
//Disabled HUD (NOT WORKING QUITE WELL)
void drawHUD()
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(1.0f, 0.0f, 1.0f, 0.0f, 0.5f, -1.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	
	glBegin(GL_QUADS);
	glVertex2f(0.05,0.05);
	glVertex2f(0.3,0.05);
	glVertex2f(0.3,0.15);
	glVertex2f(0.05,0.15);
	glEnd(); 
	
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}
//==============================================================================

//==============================================================================
//Renders the Screen
void display()
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    //camera follows player movement
    gluLookAt(player.x+(1.0*player.dx/sMax)+0.5f+3*cos(M_PI_2+cur_direction/double(direction_parts)*2.*M_PI),player.y+distance,player.z+(1.0*player.dz/sMax)+0.5f+3*sin(M_PI_2+cur_direction/double(direction_parts)*2.*M_PI),
              player.x+(1.0*player.dx/sMax)+0.5f,player.y+0.5f,player.z+(1.0*player.dz/sMax)+0.5f,
              0,1,0);
              
//Generates the Map and Textures
    glPushMatrix();
    for (int i=0;i<map.x;i++)
        for (int j=0;j<map.z;j++)
            if (data[j][i] == 'x')
            {
            	glPushMatrix();
            	glBindTexture(GL_TEXTURE_2D, texture[1]);
                drawBox(j,i);
                glPopMatrix();
                
            } else {
            	glPushMatrix();
            	glBindTexture(GL_TEXTURE_2D, texture[0]);
                drawFloor(i,i+1,j,j+1);
                glPopMatrix();
}	
	glPopMatrix();
	
//============================================================================== 
//Character
	glPushMatrix();
	glTranslatef ( player.x+(1.0*player.dx/sMax)+0.5f, player.y+0.5f, player.z+(1.0*player.dz/sMax)+0.5f);
	glPushMatrix();
	glTranslatef(0,-0.5,0);
	//glRotatef(90,0,1,0);
	glScalef(0.2,0.2,0.2);
	
	//foot right
	glPushMatrix();
	glColor3f(0.8,0.8,1);
	glTranslatef(0.05,0.4,0.4);
	glutSolidSphere(0.4,10,10);
   	glPopMatrix();

	//foot left
	glPushMatrix();
	glColor3f(0.8,0.8,1);
	glTranslatef(0.05,0.4,-0.4);
	glutSolidSphere(0.4,10,10);
   	glPopMatrix();
   	
	//leg right
	glPushMatrix();
	glDisableClientState(GL_COLOR_ARRAY);
	glColor3f(0.8,0.8,0.8);
	glTranslatef(0.05,1.4,0.4);
    glScalef(0.2,0.8,0.2);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glDrawArrays(GL_QUADS, 0, 24);
   	glPopMatrix();

	//leg left
	glPushMatrix();
	glDisableClientState(GL_COLOR_ARRAY);
	glColor3f(0.8,0.8,0.8);
	glTranslatef(0.05,1.4,-0.4);
    glScalef(0.2,0.8,0.2);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glDrawArrays(GL_QUADS, 0, 24);
   	glPopMatrix();
   	
	//lower torso
	glPushMatrix();
	glDisableClientState(GL_COLOR_ARRAY);
	glColor3f(0.9,0.2,0.3);
	glTranslatef(0,2.8,0);
    glScalef(0.35,0.6,0.6);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glDrawArrays(GL_QUADS, 0, 24);
   	glPopMatrix();
   	
	//upper torso
	glPushMatrix();
	glDisableClientState(GL_COLOR_ARRAY);
	glColor3f(0.1,0.1,0.1);
	glTranslatef(-0.05,3.8,0);
    glScalef(0.4,0.4,0.8);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glDrawArrays(GL_QUADS, 0, 24);
   	glPopMatrix();   	

	//arm right
	glPushMatrix();
	glDisableClientState(GL_COLOR_ARRAY);
	glColor3f(0.8,0.8,0.8);
	glTranslatef(0.05,3.4,1.0);
    glScalef(0.2,0.8,0.2);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glDrawArrays(GL_QUADS, 0, 24);
   	glPopMatrix();
   	
	//hand right
   	glPushMatrix();
	glColor3f(0.8,0.8,1);
	glTranslatef(0.05,2.4,1.0);
	glutSolidSphere(0.3,10,10);
   	glPopMatrix();

	//arm left
	glPushMatrix();
	glDisableClientState(GL_COLOR_ARRAY);
	glColor3f(0.8,0.8,0.8);
	glTranslatef(0.05,3.4,-1.0);
    glScalef(0.2,0.8,0.2);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glDrawArrays(GL_QUADS, 0, 24);
   	glPopMatrix();
   	
	//hand left
   	glPushMatrix();
	glColor3f(0.8,0.8,1);
	glTranslatef(0.05,2.4,-1.0);
	glutSolidSphere(0.3,10,10);
   	glPopMatrix();

	//head
   	glPushMatrix();
	glColor3f(0.8,0.8,1);
	glTranslatef(-0.05,4.6,0);
	glutSolidSphere(0.6,10,10);
   	glPopMatrix();
	glPopMatrix();
	glPopMatrix();
//==============================================================================

//==============================================================================
 	//Good Things
 	//food1
    glPushMatrix(); 
    glTranslatef ( food1.x +0.5f, food1.y+0.5f, food1.z+0.5f);
    glColor3d(1,0.5,0);
    glutSolidSphere(0.4,100,100);
    glColor3d(1,1,1);
    glPopMatrix();
	
	//food2
	glPushMatrix(); 
    glTranslatef ( food2.x +0.5f, food2.y+0.5f, food2.z+0.5f);
    glColor3d(1,0.5,0);
    glutSolidSphere(0.4,100,100);
    glColor3d(1,1,1);
    glPopMatrix();
	
	//food3
	glPushMatrix(); 
    glTranslatef ( food3.x +0.5f, food3.y+0.5f, food3.z+0.5f);
    glColor3d(1,0.5,0);
    glutSolidSphere(0.4,100,100);
    glColor3d(1,1,1);
    glPopMatrix();
	
	//food4
	glPushMatrix(); 
    glTranslatef ( food4.x +0.5f, food4.y+0.5f, food4.z+0.5f);
    glColor3d(1,0.5,0);
    glutSolidSphere(0.4,100,100);
    glColor3d(1,1,1);
    glPopMatrix();  
//==============================================================================

//==============================================================================
//BadGuy1
 glPushMatrix();
 glTranslatef ( badguy1.x +0.5f, badguy1.y+0.5f, badguy1.z+0.5f);
 glPushMatrix();
 glTranslatef(0,-0.5,0);   
 glColor3d(1,0.0,0.5);
 	glScalef(0.2,0.2,0.2);
   //head
   	glPushMatrix(); glColor3f(1,0,0); glTranslatef(0,1.6,0); glutSolidSphere(1,10,10); glPopMatrix();
	//Right legs
   	//front right leg
   	glPushMatrix(); glRotatef(0,0,1,0); glPushMatrix(); glColor3f(0.8,0.8,0.8); glTranslatef(-1,1.4,1.0); glRotatef(-45,1,0,1);	glRotatef(-15,0,0,1); glScalef(0.2,0.8,0.2); glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(3, GL_FLOAT, 0, vertices); glDrawArrays(GL_QUADS, 0, 24); glPopMatrix();
   	//back right leg
	glPushMatrix(); glColor3f(0.8,0.8,0.8); glTranslatef(0.9,1.4,1.0); glRotatef(180,1,0,0); glPushMatrix(); glRotatef(-45,1,0,1); glRotatef(-15,0,0,1); glPopMatrix; glScalef(0.2,0.8,0.2); glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(3, GL_FLOAT, 0, vertices); glDrawArrays(GL_QUADS, 0, 24); glPopMatrix(); glPopMatrix();	
	//Left legs
   	//front Left leg
   	glPushMatrix(); glRotatef(180,0,1,0); glPushMatrix(); glColor3f(0.8,0.8,0.8); glTranslatef(-1,1.4,1.0); glRotatef(-45,1,0,1);	glRotatef(-15,0,0,1); glScalef(0.2,0.8,0.2); glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(3, GL_FLOAT, 0, vertices); glDrawArrays(GL_QUADS, 0, 24); glPopMatrix();
   	//back Left leg
	glPushMatrix(); glColor3f(0.8,0.8,0.8); glTranslatef(0.9,1.4,1.0); glRotatef(180,1,0,0); glPushMatrix(); glRotatef(-45,1,0,1); glRotatef(-15,0,0,1); glPopMatrix; glScalef(0.2,0.8,0.2); glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(3, GL_FLOAT, 0, vertices); glDrawArrays(GL_QUADS, 0, 24); glPopMatrix(); glPopMatrix();	
glPopMatrix();
glPopMatrix();		
//===============================================================================

//===============================================================================
//BadGuy2
	glPushMatrix();
	glTranslatef ( badguy2.x +0.5f, badguy2.y+0.5f, badguy2.z+0.5f);  
	glPushMatrix();
	glTranslatef(0,-0.5,0);   
 	glColor3d(1,0.0,0.5);
 	glScalef(0.2,0.2,0.2);
   //head
   	glPushMatrix(); glColor3f(1,0,0); glTranslatef(0,1.6,0); glutSolidSphere(1,10,10); glPopMatrix();
	//Right legs //front right leg
   	glPushMatrix(); glRotatef(0,0,1,0); glPushMatrix(); glColor3f(0.8,0.8,0.8); glTranslatef(-1,1.4,1.0); glRotatef(-45,1,0,1);	glRotatef(-15,0,0,1); glScalef(0.2,0.8,0.2); glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(3, GL_FLOAT, 0, vertices); glDrawArrays(GL_QUADS, 0, 24); glPopMatrix();
   	//back right leg
	glPushMatrix(); glColor3f(0.8,0.8,0.8); glTranslatef(0.9,1.4,1.0); glRotatef(180,1,0,0); glPushMatrix(); glRotatef(-45,1,0,1); glRotatef(-15,0,0,1); glPopMatrix; glScalef(0.2,0.8,0.2); glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(3, GL_FLOAT, 0, vertices); glDrawArrays(GL_QUADS, 0, 24); glPopMatrix(); glPopMatrix();	
	//Left legs //front Left leg
   	glPushMatrix(); glRotatef(180,0,1,0); glPushMatrix(); glColor3f(0.8,0.8,0.8); glTranslatef(-1,1.4,1.0); glRotatef(-45,1,0,1);	glRotatef(-15,0,0,1); glScalef(0.2,0.8,0.2); glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(3, GL_FLOAT, 0, vertices); glDrawArrays(GL_QUADS, 0, 24); glPopMatrix();
   	//back Left leg
	glPushMatrix(); glColor3f(0.8,0.8,0.8); glTranslatef(0.9,1.4,1.0); glRotatef(180,1,0,0); glPushMatrix(); glRotatef(-45,1,0,1); glRotatef(-15,0,0,1); glPopMatrix; glScalef(0.2,0.8,0.2); glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(3, GL_FLOAT, 0, vertices); glDrawArrays(GL_QUADS, 0, 24); glPopMatrix(); glPopMatrix();	
glPopMatrix();
glPopMatrix();	
//==============================================================================

//===============================================================================
//Bad Guy3
	glPushMatrix();
	glTranslatef ( badguy3.x +0.5f, badguy3.y+0.5f, badguy3.z+0.5f);
	glPushMatrix();
	glTranslatef(0,-0.5,0);   
 	glColor3d(1,0.0,0.5);
 	glScalef(0.2,0.2,0.2);
   //head
   	glPushMatrix(); glColor3f(1,0,0); glTranslatef(0,1.6,0); glutSolidSphere(1,10,10); glPopMatrix();
	//Right legs //front right leg
   	glPushMatrix(); glRotatef(0,0,1,0); glPushMatrix(); glColor3f(0.8,0.8,0.8); glTranslatef(-1,1.4,1.0); glRotatef(-45,1,0,1);	glRotatef(-15,0,0,1); glScalef(0.2,0.8,0.2); glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(3, GL_FLOAT, 0, vertices); glDrawArrays(GL_QUADS, 0, 24); glPopMatrix();
   	//back right leg
	glPushMatrix(); glColor3f(0.8,0.8,0.8); glTranslatef(0.9,1.4,1.0); glRotatef(180,1,0,0); glPushMatrix(); glRotatef(-45,1,0,1); glRotatef(-15,0,0,1); glPopMatrix; glScalef(0.2,0.8,0.2); glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(3, GL_FLOAT, 0, vertices); glDrawArrays(GL_QUADS, 0, 24); glPopMatrix(); glPopMatrix();	
	//Left legs //front Left leg
   	glPushMatrix(); glRotatef(180,0,1,0); glPushMatrix(); glColor3f(0.8,0.8,0.8); glTranslatef(-1,1.4,1.0); glRotatef(-45,1,0,1);	glRotatef(-15,0,0,1); glScalef(0.2,0.8,0.2); glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(3, GL_FLOAT, 0, vertices); glDrawArrays(GL_QUADS, 0, 24); glPopMatrix();
   	//back Left leg
	glPushMatrix(); glColor3f(0.8,0.8,0.8); glTranslatef(0.9,1.4,1.0); glRotatef(180,1,0,0); glPushMatrix(); glRotatef(-45,1,0,1); glRotatef(-15,0,0,1); glPopMatrix; glScalef(0.2,0.8,0.2); glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(3, GL_FLOAT, 0, vertices); glDrawArrays(GL_QUADS, 0, 24); glPopMatrix(); glPopMatrix();	
glPopMatrix();
glPopMatrix();	
//==============================================================================
 
//===============================================================================
//Bad Guy4
	glPushMatrix();
	glTranslatef ( badguy4.x +0.5f, badguy4.y+0.5f, badguy4.z+0.5f);
	glPushMatrix(); 
	glTranslatef(0,-0.5,0); 
 	glColor3d(1,0.0,0.5);
 	glScalef(0.2,0.2,0.2);
   //head
   	glPushMatrix(); glColor3f(1,0,0); glTranslatef(0,1.6,0); glutSolidSphere(1,10,10); glPopMatrix();
	//Right legs //front right leg
   	glPushMatrix(); glRotatef(0,0,1,0); glPushMatrix(); glColor3f(0.8,0.8,0.8); glTranslatef(-1,1.4,1.0); glRotatef(-45,1,0,1);	glRotatef(-15,0,0,1); glScalef(0.2,0.8,0.2); glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(3, GL_FLOAT, 0, vertices); glDrawArrays(GL_QUADS, 0, 24); glPopMatrix();
   	//back right leg
	glPushMatrix(); glColor3f(0.8,0.8,0.8); glTranslatef(0.9,1.4,1.0); glRotatef(180,1,0,0); glPushMatrix(); glRotatef(-45,1,0,1); glRotatef(-15,0,0,1); glPopMatrix; glScalef(0.2,0.8,0.2); glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(3, GL_FLOAT, 0, vertices); glDrawArrays(GL_QUADS, 0, 24); glPopMatrix(); glPopMatrix();	
	//Left legs //front Left leg
   	glPushMatrix(); glRotatef(180,0,1,0); glPushMatrix(); glColor3f(0.8,0.8,0.8); glTranslatef(-1,1.4,1.0); glRotatef(-45,1,0,1);	glRotatef(-15,0,0,1); glScalef(0.2,0.8,0.2); glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(3, GL_FLOAT, 0, vertices); glDrawArrays(GL_QUADS, 0, 24); glPopMatrix();
   	//back Left leg
	glPushMatrix(); glColor3f(0.8,0.8,0.8); glTranslatef(0.9,1.4,1.0); glRotatef(180,1,0,0); glPushMatrix(); glRotatef(-45,1,0,1); glRotatef(-15,0,0,1); glPopMatrix; glScalef(0.2,0.8,0.2); glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(3, GL_FLOAT, 0, vertices); glDrawArrays(GL_QUADS, 0, 24); glPopMatrix(); glPopMatrix();	
glPopMatrix();
glPopMatrix();	
//==============================================================================
 
//==============================================================================
//Game Goal
    glPushMatrix(); 
    glTranslatef ( goal.x +0.5f, goal.y+0.5f, goal.z+0.5f);
    glColor4d(1,1,0.,0.4);
    glutSolidSphere(0.5,100,100);
    glColor3d(1,1,1);
    glPopMatrix(); 
//==============================================================================


//==============================================================================	
//Prints text to screen
	char text[MAX_TEXT_SIZE];
	glColor3f(1,0,1);
	sprintf(text,"%d Seconds Remaining.", g_counter);
	drawString(text, 1100, 790);
	drawString("Time IS Running Out!!", 1100, 760);
	drawString("Your Goal Is The Yellow Ball!", 1100, 720);
	drawString("Collect Orange Candies To Increase Time", 1100, 680);
	drawString("Avoid Monsters That Decrease Time", 1100, 640);
	drawString("Controls:", 1100, 600);
	drawString("'Q' Quits Game", 1100, 560);
	drawString("'R' Resets Camera", 1100, 540);
	drawString("'A' Rotates Camera To Left", 1100, 520);
	drawString("'S' Rotates Camera To Right", 1100, 500);
	drawString("'Z' Zooms Camera Out", 1100, 480);
	drawString("'X' Zooms Camera In", 1100, 460);
	drawString("'Left Arrow' Moves Player Left", 1100, 440);
	drawString("'Right Arrow' Moves Player Right", 1100, 420);
	drawString("'Up Arrow' Moves Player Up", 1100, 400);
	drawString("'Down Arrow' Moves Player Down", 1100, 380);
    glutSwapBuffers();
}
//==============================================================================

//==============================================================================
void reshape ( int w, int h )
{
    glViewport( 0, 0, (GLsizei)w, (GLsizei)h);
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    gluPerspective( 60.0, (GLfloat)w/(GLfloat)h, 1.0, 60.0);
    glOrtho(0.0,(GLfloat)w,(GLfloat)h, 0.0, 0.0, 0.0);
    
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    gluLookAt(0,0,25,0,0,0,0,1,0);
}
//==============================================================================

//==============================================================================
//Camera Controls
void key( unsigned char key, int x, int y)
{
    if ( key=='q' || key=='Q' || key == 27) halt(false); //printf("You Left The Game!\n"); exit(0);  // Exit Key
    if ( key=='a' || key=='A' ) { cur_direction--; if (cur_direction<0) cur_direction+=direction_parts;} // Rotate Camera to the Left
    if ( key=='s' || key=='S' ) { cur_direction++; if (cur_direction==direction_parts) cur_direction=0;} // Rotate Camera to the Right
    if ((key=='z' || key=='Z') && (distance<8.) ) distance+=0.25;  // Zoom In Camera (+)
    if ((key=='x' || key=='X') && (distance>3.) ) distance-=0.25;   // zoom Out Camera (-)
    if ( key=='r' || key=='R' ) { cur_direction=0; distance=4.;}    // Reset Camera

}
//==============================================================================

//==============================================================================
//Movement Controls
const int move[4][2]={{-1,0},{0,-1},{1,0},{0,1}};
const int move_key[4] = {GLUT_KEY_UP, GLUT_KEY_LEFT, GLUT_KEY_DOWN, GLUT_KEY_RIGHT }; //Movement Controls
bool good_move(int z, int x){
    return (0<=z && 0<=x && z<map.z && x<map.x && data[z][x]!='x');
}

void keys( int key, int x, int y)
{
    if (player.isGo) return;
    int dir=int(((direction_parts-cur_direction-1)/double(direction_parts))*4.+0.5);
    for (int i=0; i<4; i++)
        if ( key == move_key[i] ) {
            int newz=player.z+move[(dir+i)%4][0];
            int newx=player.x+move[(dir+i)%4][1];
            if (good_move(newz,newx)) {
                player.isGo=true;
                player.dz+=move[(dir+i)%4][0];
                player.dx+=move[(dir+i)%4][1];
                myLen++;
            }
        }
}
//==============================================================================

//==============================================================================
//Timer
void myTimer(int value) {
	if(j==0)
	{
	g_counter = g_counter - 1;
	}
	else if(j==1)
	{
		g_counter = g_counter + 10;
		j=1-1;
	}
	else if(j==2)
	{
		g_counter = g_counter - 10;
		j=2-2;
	}
	else{
		g_counter = 10;
	}
	if(g_counter < 0){
		printf("Game Over!\n");
		halt(false);
	}
	glutPostRedisplay();
	glutTimerFunc(GAP, myTimer, g_counter);
}
//==============================================================================

//==============================================================================
void genMap(void);

int main (int argc, char** argv)
{
    genMap();
    glutInit(&argc,argv);
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
    glutInitWindowSize(1024,768);
    
    glWin = glutCreateWindow("Lost In The Maze V 0.5.0");
    init();
    glutDisplayFunc(drawHUD);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(key);
    glutSpecialFunc(keys);
    glutTimerFunc(GAP, myTimer, 0);
    glutIdleFunc(animate);
    glutFullScreen();
    glutMainLoop();
    return 0;
}
//==============================================================================

//==============================================================================
int step(int y, int x)
{
    int res=0;
    if ((data[y][x]=='x')) res++;
    if ((y<mm)&&(data[y+1][x]=='x')) res++;
    if ((y>0 )&&(data[y-1][x]=='x')) res++;
    if ((x<mm)&&(data[y][x+1]=='x')) res++;
    if ((x>0 )&&(data[y][x-1]=='x')) res++;
    return res;
}

int stepC(int y, int x)
{
    int res=0;
    if ((cp[y][x]=='x')) res++;
    if ((y<mm)&&(cp[y+1][x]=='x')) res++;
    if ((y>0 )&&(cp[y-1][x]=='x')) res++;
    if ((x<mm)&&(cp[y][x+1]=='x')) res++;
    if ((x>0 )&&(cp[y][x-1]=='x')) res++;
    return res;
}

void DataToCp(void)
{
    for(int j=1;j<=mm;j++)
        for (int i=1;i<=mm;i++)
            cp[j][i]=data[j][i];
}
//==============================================================================

//==============================================================================
void fill (int y,int x,Tpos *v, int *l)
{
    int st=1,en=0;
    int G[mm*mm],Ll[mm*mm];
    G[0]=y*mm+x; Ll[0]=0;
    cp[y][x]='x';
    while (st!=en)
    {
        if ((G[en]%mm+1<mm)&&(cp[(int)G[en]/mm][G[en]%mm+1]=='.'))
        {
            G[st]=G[en]+1;
            Ll[st]=Ll[en]+1;
            cp[(int)G[st]/mm][G[st]%mm]='x';
            st++;
        }
        if ((G[en]%mm-1>0)&&(cp[(int)G[en]/mm][G[en]%mm-1]=='.'))
        {
            G[st]=G[en]-1;
            Ll[st]=Ll[en]+1;
            cp[(int)G[st]/mm][G[st]%mm]='x';
            st++;
        }
        if ((((int)G[en]/mm)+1<mm)&&(cp[((int)G[en]/mm)+1][G[en]%mm]=='.'))
        {
            G[st]=G[en]+mm;
            Ll[st]=Ll[en]+1;
            cp[(int)G[st]/mm][G[st]%mm]='x';
            st++;
        }
        if ((((int)G[en]/mm)-1>0)&&(cp[((int)G[en]/mm)-1][G[en]%mm]=='.'))
        {
            G[st]=G[en]-mm;
            Ll[st]=Ll[en]+1;
            cp[(int)G[st]/mm][G[st]%mm]='x';
            st++;
        }
        en++;
    }
    int rnd=rand()%5+1;
    (*v).x =(int) G[en-rnd] % mm;
    (*v).y =(int) G[en-rnd] / mm;
    *l=Ll[en-rnd];
}
//==============================================================================

//==============================================================================
//Collision Detection for player
bool isGood(Tpos a)
{
    DataToCp();
    int k=0,i,j,lt;
    Tpos temp;
    cp[a.y][a.x]='x';
    if ((step(a.y,a.x)>3)||(step(a.y+1,a.x)>3)||(step(a.y-1,a.x)>3)||(step(a.y,a.x+1)>3)||(step(a.y,a.x-1)>3))
        return false;
    for(j=1;j<=m;j++)
        for(i=1;i<=m;i++)
            if (cp[j][i]=='.')
            {
                if (k>0)
                    return false;
                fill(j,i,&temp,&lt);
                k++;
            }
    return true;
}
//==============================================================================

//==============================================================================
//Generates Map, Player, Food, BadGuys
void genMap()
{
    int i,j;
    map.x=m+2; map.z=m+2;
    printf("Loading... Please Wait.\n");
    srand( (unsigned)time( NULL ) );
    for (j=0;j<map.z;j++)
        for (i=0;i<map.x;i++)
        {
            if ((i==0)||(j==0)||(i==map.x-1)||(j==map.z-1))
                data[j][i]='x';
                else
                data[j][i]='.';
        }

    int k=0; 
    Tpos t;
    while (k<(int)m*m/2.5)
    {
        t.x = rand()%mm+1;
        t.y = rand()%mm+1;
        if ((data[t.y][t.x]=='.')&&(isGood(t)))
        {
            data[t.y][t.x]='x';
            k++;
        }
    }
    for (j=1;j<=m;j++)
        for (i=1;i<=m;i++)
            if ((data[j][i]=='x')&&(step(j,i)>3))
            {
                data[j][i]='.';
            }
    for (j=1;j<=m;j++)
        for (i=1;i<=m;i++)
            if ((data[j][i]=='.')&&(step(j,i)<2))
            {
                t.x = i; t.y=j;
                if (isGood(t))
                    data[j][i]='x';
            }

    Tpos ps[11]; k=0;
    while (k<=10)
    {
        t.x = rand()%mm+1;
        t.y = rand()%mm+1;
        if (data[t.y][t.x]=='.')
        {
            ps[k]=t;
            k++;
        }
    }
    k=rand()%11;
    player.x=ps[k].x;
    player.z=ps[k].y;
    DataToCp();
    fill(ps[k].y,ps[k].x,&t,&pathLen);
    goal.x=t.x;
    goal.z=t.y;
    
	//badguy1 rand spawn
	while (k<=10){ t.x = rand()%mm+1; t.y = rand()%mm+1; if (data[t.y][t.x]=='.'){ps[k]=t;k++;}}
    k=rand()%11;// fill(ps[k].y,ps[k].x,&t,&pathLen);
    badguy1.x=ps[k].x; badguy1.z=ps[k].y;
    
   	//badguy2 rand spawn
	while (k<=10){ t.x = rand()%mm+1; t.y = rand()%mm+1; if (data[t.y][t.x]=='.'){ps[k]=t;k++;}}
    k=rand()%11; //fill(ps[k].y,ps[k].x,&t,&pathLen);
    badguy2.x=ps[k].x; badguy2.z=t.y;
 
    
	//badguy3 rand spawn
	while (k<=10){ t.x = rand()%mm+1; t.y = rand()%mm+1; if (data[t.y][t.x]=='.'){ps[k]=t;k++;}}
    k=rand()%11; //fill(ps[k].y,ps[k].x,&t,&pathLen);
	badguy3.x=ps[k].x; badguy3.z=t.y;


	//badguy4 rand spawn
	while (k<=10){ t.x = rand()%mm+1; t.y = rand()%mm+1; if (data[t.y][t.x]=='.'){ps[k]=t;k++;}}
    k=rand()%11; //fill(ps[k].y,ps[k].x,&t,&pathLen);
	badguy4.x=ps[k].x; badguy4.z=t.y;
	
	
	//food1 rando spawn
	while (k<=10){ t.x = rand()%mm+1; t.y = rand()%mm+1; if (data[t.y][t.x]=='.'){ps[k]=t;k++;}}
    k=rand()%11;// fill(ps[k].y,ps[k].x,&t,&pathLen);
	DataToCp();fill(ps[k].y,ps[k].x,&t,&pathLen);
	food1.x=ps[k].x; food1.z=ps[k].y;
	 
	//food2 rando spawn
	while (k<=10){ t.x = rand()%mm+1; t.y = rand()%mm+1; if (data[t.y][t.x]=='.'){ps[k]=t;k++;}}
    k=rand()%11;// fill(ps[k].y,ps[k].x,&t,&pathLen);
	DataToCp();fill(ps[k].y,ps[k].x,&t,&pathLen);
	food2.x=ps[k].x; food2.z=ps[k].y;
	
	//food3 rando spawn
	while (k<=10){ t.x = rand()%mm+1; t.y = rand()%mm+1; if (data[t.y][t.x]=='.'){ps[k]=t;k++;}}
    k=rand()%11;// fill(ps[k].y,ps[k].x,&t,&pathLen);
	DataToCp();fill(ps[k].y,ps[k].x,&t,&pathLen);
	food3.x=ps[k].x; food3.z=ps[k].y;
	
	//food4 rando spawn
	while (k<=10){ t.x = rand()%mm+1; t.y = rand()%mm+1; if (data[t.y][t.x]=='.'){ps[k]=t;k++;}}
    k=rand()%11;// fill(ps[k].y,ps[k].x,&t,&pathLen);
	DataToCp();fill(ps[k].y,ps[k].x,&t,&pathLen);
	food4.x=ps[k].x; food4.z=ps[k].y; 
	 
    printf("Loading complete.\n");
}
//==============================================================================

//==============================================================================
//Game Win or Lose Screen
void halt(bool f)
{
        glutDestroyWindow(glWin);
        printf("\n-----------------------------------------------------\n");
        printf("\n#                    The Maze Game                  #\n");
        if (f==true)
        {
        printf("\n-----------------------------------------------------\n");
        printf("\n# The shortest path %d.                             #\n",pathLen);
        printf("\n# Your path %d.                                     #\n",myLen);
        printf("\n# Congratulate you passed the game!                 #\n");
        if (  1.2*pathLen >= myLen )
        {
        printf("\n-----------------------------------------------------\n");
        printf("\n#          YOU ARE REALLY GOOD AT THIS GAME!        #\n");
        printf("\n-----------------------------------------------------\n");
        }
        }
        if (f==false)
        {
        	printf("\n-----------------------------------------------------\n");
        	printf("\n# Game Over!                                    #\n");
        	printf("\n# Time Ran Out!                                 #\n");
        	printf("\n# The shortest path %d.                         #\n",pathLen);
        	printf("\n-----------------------------------------------------\n");
        }
		getch();
        exit(0);
}
