////////////////////////////////////////////////////////////////////////
//
//   Harvard University
//   CS175 : Computer Graphics
//   Professor Steven Gortler
//
////////////////////////////////////////////////////////////////////////

#include <cstddef>
#include <vector>
#include <list>
#include <string>
#include <memory>
#include <map>
#include <fstream>
#include <stdexcept>
#include <stdio.h> 
#include <stdlib.h>
#include <sstream> 

#if __GNUG__
#   include <tr1/memory>
#endif

#ifdef __MAC__
#   include <OpenGL/gl3.h>
#   include <GLUT/glut.h>
#else
#   include <GL/glew.h>
#   include <GL/glut.h>
#endif

#include "ppm.h"
#include "cvec.h"
#include "matrix4.h"
#include "rigtform.h"
#include "glsupport.h"
#include "geometrymaker.h"
#include "geometry.h"
#include "arcball.h"
#include "scenegraph.h"
#include "sgutils.h"
#include "mesh.h"
#include <math.h>    

#include "asstcommon.h"
#include "drawer.h"
#include "picker.h"

#define EMBED_SOLUTION_GLSL 1
#define PI 3.14159265

using namespace std;
using namespace tr1;

// G L O B A L S ///////////////////////////////////////////////////

// --------- IMPORTANT --------------------------------------------------------
// Before you start working on this assignment, set the following variable
// properly to indicate whether you want to use OpenGL 2.x with GLSL 1.0 or
// OpenGL 3.x+ with GLSL 1.5.
//
// Set g_Gl2Compatible = true to use GLSL 1.0 and g_Gl2Compatible = false to
// use GLSL 1.5. Use GLSL 1.5 unless your system does not support it.
//
// If g_Gl2Compatible=true, shaders with -gl2 suffix will be loaded.
// If g_Gl2Compatible=false, shaders with -gl3 suffix will be loaded.
// To complete the assignment you only need to edit the shader files that get
// loaded
// ----------------------------------------------------------------------------
const bool g_Gl2Compatible = false;


static const float g_frustMinFov = 60.0;  // A minimal of 60 degree field of view
static float g_frustFovY = g_frustMinFov; // FOV in y direction (updated by updateFrustFovY)

static const float g_frustNear = -0.1;    // near plane
static const float g_frustFar = -50.0;    // far plane
static const float g_groundY = -2.0;      // y coordinate of the ground
static const float g_groundSize = 10.0;   // half the ground length

enum SkyMode { WORLD_SKY = 0, SKY_SKY = 1 };
enum Weather { CLEAR = 0, RAIN = 1, SNOW = 2};

static int g_windowWidth = 1300;
static int g_windowHeight = 512;
static bool g_mouseClickDown = false;    // is the mouse button pressed
static bool g_mouseLClickButton, g_mouseRClickButton, g_mouseMClickButton;
static bool g_spaceDown = false;         // space state, for middle mouse emulation
static int g_mouseClickX, g_mouseClickY; // coordinates for mouse click event
static int g_activeShader = 0;

static SkyMode g_activeCameraFrame = WORLD_SKY;
static Weather weather = CLEAR;
int splashing = 0;

static bool g_displayArcball = true;
static double g_arcballScreenRadius = 100; // number of pixels
static double g_arcballScale = 1;

static bool g_pickingMode = false;

static bool g_playingAnimation = false;

static bool g_shellNeedsUpdate = false;

// Global variables for used physical simulation
static const Cvec3 g_gravity(0, -0.5, 0);  // gavity vector
static double g_timeStep = 0.02;
static double g_numStepsPerFrame = 10;
static double g_damping = 0.96;
static double g_stiffness = 4;
static int g_simulationsPerSecond = 60;

static std::vector<Cvec3> g_tipPos,        // should be hair tip pos in world-space coordinates
g_tipVelocity;   // should be hair tip velocity in world-space coordinates
static vector<vector<Cvec3> > g_prevshells;

// New Geometry
static const int g_numShells = 24; // constants defining how many layers of shells
static double g_furHeight = 0.21;
static double g_hairyness = 0.7;

static shared_ptr<SimpleGeometryPNX> g_bunnyGeometry;
static vector<shared_ptr<SimpleGeometryPNX> > g_bunnyShellGeometries;
static Mesh g_bunnyMesh;


// New Scene node


// --------- Materials
static shared_ptr<Material> g_redDiffuseMat,
g_blueDiffuseMat,
g_arcballMat,
g_pickingMat,
g_greenSolidMat,
g_bumpFloorMat,
g_sunMat,
g_stormMat,
g_lightMat;

static shared_ptr<Material> g_bunnyMat; // for the bunny
static vector<shared_ptr<Material> > g_bunnyShellMats; // for bunny shells
shared_ptr<Material> g_overridingMaterial;


// --------- Geometry
typedef SgGeometryShapeNode MyShapeNode;

// Vertex buffer and index buffer associated with the ground and cube geometry
static shared_ptr<Geometry> g_ground, g_cube, g_sphere;

// --------- Scene

static shared_ptr<SgRootNode> g_world;
static shared_ptr<SgRbtNode> g_skyNode, g_groundNode, g_robot1Node, g_robot2Node, g_light1, g_sun;
static shared_ptr<SgRbtNode> g_bunnyNode;


static shared_ptr<SgRbtNode> g_currentCameraNode;
static shared_ptr<SgRbtNode> g_currentPickedRbtNode;

// ---------- Animation

class Animator {
public:
	typedef vector<shared_ptr<SgRbtNode> > SgRbtNodes;
	typedef vector<RigTForm> KeyFrame;
	typedef list<KeyFrame> KeyFrames;
	typedef KeyFrames::iterator KeyFrameIter;

private:
	SgRbtNodes nodes_;
	KeyFrames keyFrames_;

public:
	void attachSceneGraph(shared_ptr<SgNode> root) {
		nodes_.clear();
		keyFrames_.clear();
		dumpSgRbtNodes(root, nodes_);
	}

	void loadAnimation(const char *filename) {
		ifstream f(filename, ios::binary);
		if (!f)
			throw runtime_error(string("Cannot load ") + filename);
		int numFrames, numRbtsPerFrame;
		f >> numFrames >> numRbtsPerFrame;
		if (numRbtsPerFrame != nodes_.size()) {
			cerr << "Number of Rbt per frame in " << filename
				<< " does not match number of SgRbtNodes in the current scene graph.";
			return;
		}

		Cvec3 t;
		Quat r;
		keyFrames_.clear();
		for (int i = 0; i < numFrames; ++i) {
			keyFrames_.push_back(KeyFrame());
			keyFrames_.back().reserve(numRbtsPerFrame);
			for (int j = 0; j < numRbtsPerFrame; ++j) {
				f >> t[0] >> t[1] >> t[2] >> r[0] >> r[1] >> r[2] >> r[3];
				keyFrames_.back().push_back(RigTForm(t, r));
			}
		}
	}

	void saveAnimation(const char *filename) {
		ofstream f(filename, ios::binary);
		int numRbtsPerFrame = nodes_.size();
		f << getNumKeyFrames() << ' ' << numRbtsPerFrame << '\n';
		for (KeyFrames::const_iterator frameIter = keyFrames_.begin(), e = keyFrames_.end(); frameIter != e; ++frameIter) {
			for (int j = 0; j < numRbtsPerFrame; ++j) {
				const RigTForm& rbt = (*frameIter)[j];
				const Cvec3& t = rbt.getTranslation();
				const Quat& r = rbt.getRotation();
				f << t[0] << ' ' << t[1] << ' ' << t[2] << ' '
					<< r[0] << ' ' << r[1] << ' ' << r[2] << ' ' << r[3] << '\n';
			}
		}
	}

	int getNumKeyFrames() const {
		return keyFrames_.size();
	}

	int getNumRbtNodes() const {
		return nodes_.size();
	}

	// t can be in the range [0, keyFrames_.size()-3]. Fractional amount like 1.5 is allowed.
	void animate(double t) {
		if (t < 0 || t > keyFrames_.size() - 3)
			throw runtime_error("Invalid animation time parameter. Must be in the range [0, numKeyFrames - 3]");

		t += 1; // interpret the key frames to be at t= -1, 0, 1, 2, ...
		const int integralT = int(floor(t));
		const double fraction = t - integralT;

		KeyFrameIter f1 = getNthKeyFrame(integralT), f0 = f1, f2 = f1;
		--f0;
		++f2;
		KeyFrameIter f3 = f2;
		++f3;
		if (f3 == keyFrames_.end()) // this might be true when t is exactly keyFrames_.size()-3.
			f3 = f2; // in which case we step back

		for (int i = 0, n = nodes_.size(); i < n; ++i) {
			nodes_[i]->setRbt(interpolateCatmullRom((*f0)[i], (*f1)[i], (*f2)[i], (*f3)[i], fraction));
		}
	}

	KeyFrameIter keyFramesBegin() {
		return keyFrames_.begin();
	}

	KeyFrameIter keyFramesEnd() {
		return keyFrames_.end();
	}

	KeyFrameIter getNthKeyFrame(int n) {
		KeyFrameIter frameIter = keyFrames_.begin();
		advance(frameIter, n);
		return frameIter;
	}

	void deleteKeyFrame(KeyFrameIter keyFrameIter) {
		keyFrames_.erase(keyFrameIter);
	}

	void pullKeyFrameFromSg(KeyFrameIter keyFrameIter) {
		for (int i = 0, n = nodes_.size(); i < n; ++i) {
			(*keyFrameIter)[i] = nodes_[i]->getRbt();
		}
	}

	void pushKeyFrameToSg(KeyFrameIter keyFrameIter) {
		for (int i = 0, n = nodes_.size(); i < n; ++i) {
			nodes_[i]->setRbt((*keyFrameIter)[i]);
		}
	}

	KeyFrameIter insertEmptyKeyFrameAfter(KeyFrameIter beforeFrame) {
		if (beforeFrame != keyFrames_.end())
			++beforeFrame;

		KeyFrameIter frameIter = keyFrames_.insert(beforeFrame, KeyFrame());
		frameIter->resize(nodes_.size());
		return frameIter;
	}

};

static int g_msBetweenKeyFrames = 2000; // 2 seconds between keyframes
static int g_animateFramesPerSecond = 60; // frames to render per second during animation playback


static Animator g_animator;
static Animator::KeyFrameIter g_curKeyFrame;
static int g_curKeyFrameNum;

typedef struct { 

	float x; 
	float y; 
	float z;  

	float v; // velocity 
	float splashx;

	int splashing;

	shared_ptr<MyShapeNode> node;
	shared_ptr<MyShapeNode> rainspot;

} particles;

typedef struct { 
	float x; 
	float y; 
	float z;  

	float v; // velocity 
	shared_ptr<MyShapeNode> ball1;
	shared_ptr<MyShapeNode> ball2;
	shared_ptr<MyShapeNode> ball3;
	shared_ptr<MyShapeNode> ball4;
} clouds;


#define PARTICLES 1000
#define CLOUDS 20 

particles particle_system[PARTICLES];
clouds cloud_system[CLOUDS];

int neg = 1;

shared_ptr<MyShapeNode> sun;

///////////////// END OF G L O B A L S //////////////////////////////////////////////////


void initParticle(int i) {
	particle_system[i].splashing = 0;
	if (neg) {
		particle_system[i].x = - static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/(g_groundSize))) ;
		neg = 0;
	}
	else {
		particle_system[i].x = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/(g_groundSize))) ;
		neg = 1;
	}

	particle_system[i].splashx = particle_system[i].x + .5;
	particle_system[i].y = 20.0;

	particle_system[i].z = - 2 * static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/(g_groundSize)))  + g_groundSize;

	particle_system[i].v = rand() % 100 / 100; 

	shared_ptr<MyShapeNode> shape(
						new MyShapeNode(g_sphere,
							g_blueDiffuseMat,
							Cvec3(particle_system[i].x, particle_system[i].y, particle_system[i].z),
							Cvec3(0, 0, 0),
							Cvec3(.00001, .00001, .00001)));

	particle_system[i].node = shape; 

	g_world->addChild(shape);
}

void initParticles(void) {
	glEnable(GL_DEPTH_TEST);
	for (int i = 0; i < PARTICLES; i++) {
		initParticle(i);
	}
}

void drawRain(void) {
	if (weather != CLEAR) {
		for (int i = 0; i < PARTICLES; i += 10) {
			if (particle_system[i].splashing) {
				g_world->removeChild(particle_system[i].rainspot);
				particle_system[i].splashing = 0;
			}

			g_world->removeChild(particle_system[i].node);

			if (weather == SNOW) {
				particle_system[i].v = rand() % 1000 / 10000.; 
				shared_ptr<MyShapeNode> shape(
							new MyShapeNode(g_sphere,
								g_lightMat,
								Cvec3(particle_system[i].x, particle_system[i].y, particle_system[i].z),
								Cvec3(0, 0, 0),
								Cvec3(.1, .1, .1)));

				g_world->addChild(shape);
				particle_system[i].node = shape;
			}
			else {
				particle_system[i].v = rand() % 1000 / 1000.; 
				shared_ptr<MyShapeNode> shape(
							new MyShapeNode(g_sphere,
								g_blueDiffuseMat,
								Cvec3(particle_system[i].x, particle_system[i].y, particle_system[i].z),
								Cvec3(0, 0, 0),
								Cvec3(.01, .2, .01)));
				g_world->addChild(shape);
				particle_system[i].node = shape;
			}
			particle_system[i].y -= particle_system[i].v;
		


			float stop; 
			if (weather == RAIN) 
				stop = g_groundY - 1.5;
			else 
				stop = g_groundY -.2;

			if (particle_system[i].y <= stop) {
				g_world->removeChild(particle_system[i].node);
				if (weather == SNOW) {
					shared_ptr<MyShapeNode> shape(
							new MyShapeNode(g_sphere,
								g_lightMat,
								Cvec3(particle_system[i].x, g_groundY, particle_system[i].z),
								Cvec3(0, 0, 0),
								Cvec3(.1, .000001, .1)));

					g_world->addChild(shape);
					initParticle(i);
				}

				else { 
					shared_ptr<MyShapeNode> shape(
							new MyShapeNode(g_sphere,
								g_blueDiffuseMat,
								Cvec3(particle_system[i].x, g_groundY, particle_system[i].z),
								Cvec3(0, 0, 0),
								Cvec3(.1, .000001, .1)));

					g_world->addChild(shape);
					particle_system[i].splashing = 1;
					particle_system[i].rainspot = shape;
					initParticle(i);
				}
			}
			
		}
	}
}


float cloud_speed[3] = {.03, .02, .01};

void initClouds(void) {
	for (int i = 0; i < CLOUDS; i++) {
		if (neg) {
			cloud_system[i].x = - static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/(g_groundSize))) ;
			cloud_system[i].v = cloud_speed[i%3]; 
			neg = 0;
		}
		else {
			cloud_system[i].x = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/(g_groundSize))) ;
			cloud_system[i].v = - .05; 
			neg = 1;
		}

		cloud_system[i].y = 20.0;

		cloud_system[i].z = - 2 * static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/(g_groundSize)))  + g_groundSize;



		shared_ptr<MyShapeNode> cloud1(
						new MyShapeNode(g_sphere,
							g_lightMat,
							Cvec3(cloud_system[i].x, 20, cloud_system[i].z),
							Cvec3(0, 0, 0),
							Cvec3(1, 1, 1)));

		shared_ptr<MyShapeNode> cloud2(
								new MyShapeNode(g_sphere,
									g_lightMat,
									Cvec3(cloud_system[i].x + 1, 20, cloud_system[i].z),
									Cvec3(0, 0, 0),
									Cvec3(1.5, 1.5, 1.5)));

		shared_ptr<MyShapeNode> cloud3(
								new MyShapeNode(g_sphere,
									g_lightMat,
									Cvec3(cloud_system[i].x -1, 20, cloud_system[i].z),
									Cvec3(0, 0, 0),
									Cvec3(1, 1, 1)));

		shared_ptr<MyShapeNode> cloud4(
								new MyShapeNode(g_sphere,
									g_lightMat,
									Cvec3(cloud_system[i].x + 2, 20, cloud_system[i].z),
									Cvec3(0, 0, 0),
									Cvec3(1, 1, 1)));

		cloud_system[i].ball1 = cloud1; 
		cloud_system[i].ball2 = cloud2; 
		cloud_system[i].ball3 = cloud3; 
		cloud_system[i].ball4 = cloud4; 

		g_world->addChild(cloud1);
		g_world->addChild(cloud2);
		g_world->addChild(cloud3);
		g_world->addChild(cloud4);

	}
}

float bloat = 1.0; 

void drawClouds(void) {
	if (weather != CLEAR && bloat <= 2.0)
		bloat += .01;
	else if (weather == CLEAR && bloat >= 1.0)
		bloat -= .01; 
	cerr << bloat << endl;

	for (int i = 0; i < CLOUDS; i ++) {
		g_world->removeChild(cloud_system[i].ball1);
		g_world->removeChild(cloud_system[i].ball2);
		g_world->removeChild(cloud_system[i].ball3);
		g_world->removeChild(cloud_system[i].ball4);

		cloud_system[i].x += cloud_system[i].v;

		if (cloud_system[i].x > 20 || cloud_system[i].x < -20)
			cloud_system[i].v = -1 * cloud_system[i].v;

		shared_ptr<Material> mat;

		if (weather == CLEAR) 
			mat = g_lightMat;
		else 
			mat = g_stormMat;

		shared_ptr<MyShapeNode> cloud1(
								new MyShapeNode(g_sphere,
									mat,
									Cvec3(cloud_system[i].x, 20, cloud_system[i].z),
									Cvec3(0, 0, 0),
									Cvec3(1*bloat, 1*bloat, 1*bloat)));

		shared_ptr<MyShapeNode> cloud2(
								new MyShapeNode(g_sphere,
									mat,
									Cvec3(cloud_system[i].x + 1, 20, cloud_system[i].z),
									Cvec3(0, 0, 0),
									Cvec3(1.5*bloat, 1.5*bloat, 1.5*bloat)));

		shared_ptr<MyShapeNode> cloud3(
								new MyShapeNode(g_sphere,
									mat,
									Cvec3(cloud_system[i].x -1, 20, cloud_system[i].z),
									Cvec3(0, 0, 0),
									Cvec3(1*bloat, 1*bloat, 1*bloat)));

		shared_ptr<MyShapeNode> cloud4(
								new MyShapeNode(g_sphere,
									mat,
									Cvec3(cloud_system[i].x + 2, 20, cloud_system[i].z),
									Cvec3(0, 0, 0),
									Cvec3(1*bloat, 1*bloat, 1*bloat)));

		cloud_system[i].ball1 = cloud1; 
		cloud_system[i].ball2 = cloud2; 
		cloud_system[i].ball3 = cloud3; 
		cloud_system[i].ball4 = cloud4; 

		g_world->addChild(cloud1);
		g_world->addChild(cloud2);
		g_world->addChild(cloud3);
		g_world->addChild(cloud4);
		
	}

}


double tick = 0.0; 
void drawSun(void) {

	g_world->removeChild(g_sun);
	g_sun->removeChild(sun);

	Cvec3 newPos = Cvec3((g_groundSize + 5) * sin(- tick) - g_groundSize, (g_groundSize + 5) * cos(tick), -4.0);
	
	g_sun.reset(new SgRbtNode(RigTForm(newPos)));

	shared_ptr<MyShapeNode> shape(
		new MyShapeNode(g_sphere, g_sunMat, newPos, Cvec3(0.0), Cvec3(2.0)));

	sun = shape;

	g_sun->addChild(sun);
	g_world->addChild(g_sun);
	tick += 0.001;

	float modified = fmod(tick, 6.28);

	float r = (128 + 8*newPos[1] > 255) ? 255. : (128. + 8*newPos[1] > 255.);
	float g = (200 + 8*newPos[1] > 255) ? 255. : (200. + 8*newPos[1] > 255.);
	float b = 255.;

	glClearColor(r / 255., g / 255., b / 255., 0.);

}




static VertexPN findvertex(Mesh::Vertex v, int layer) {
	RigTForm bunny = inv(getPathAccumRbt(g_world, g_bunnyNode));
	
	const Cvec3 n = v.getNormal() * (g_furHeight / g_numShells);
	const Cvec3 d = ((bunny * g_tipPos[v.getIndex()] - v.getPosition() - n * g_numShells) / (g_numShells * g_numShells - g_numShells)) * 2;
	//const Cvec3 d = v.getNormal();


	if (layer == 0) {
		g_prevshells[v.getIndex()][layer] = v.getPosition() + n + d * layer;
		return VertexPN(g_prevshells[v.getIndex()][layer], v.getNormal());  // like normal
	
	}
	else {
		g_prevshells[v.getIndex()][layer] = g_prevshells[v.getIndex()][layer - 1] + n + d * layer;
		return VertexPN(g_prevshells[v.getIndex()][layer],
			g_prevshells[v.getIndex()][layer] - g_prevshells[v.getIndex()][layer - 1]);
	}
}

// Specifying shell geometries based on g_tipPos, g_furHeight, and g_numShells.
// You need to call this function whenver the shell needs to be updated
static void updateShellGeometry() {

	RigTForm bunny = (getPathAccumRbt(g_world, g_bunnyNode));
	int facenum = g_bunnyMesh.getNumFaces(); 

	g_prevshells.resize(g_bunnyMesh.getNumVertices());

	for (int i = 0; i < g_bunnyMesh.getNumVertices(); i++)
		g_prevshells[i].resize(g_numShells);

	for (int i = 0; i < g_numShells; i++) {
		vector<VertexPN> verts;
		for (int n = 0; n < g_bunnyMesh.getNumVertices(); n++)
		{
			verts.push_back(findvertex(g_bunnyMesh.getVertex(n), i));
		}

		vector<VertexPNX> shell;
		for (int j = 0; j < g_bunnyMesh.getNumFaces(); j++) {
			Mesh::Face f = g_bunnyMesh.getFace(j);
			VertexPN v1 = verts[f.getVertex(0).getIndex()];
			VertexPN v2 = verts[f.getVertex(1).getIndex()];
			VertexPN v3 = verts[f.getVertex(2).getIndex()];

			shell.push_back(VertexPNX(Cvec3(v1.p[0], v1.p[1], v1.p[2]), Cvec3(v1.n[0], v1.n[1], v1.n[2]), Cvec2(0, 0)));
			shell.push_back(VertexPNX(Cvec3(v2.p[0], v2.p[1], v2.p[2]), Cvec3(v2.n[0], v2.n[1], v2.n[2]), Cvec2(g_hairyness, 0)));
			shell.push_back(VertexPNX(Cvec3(v3.p[0], v3.p[1], v3.p[2]), Cvec3(v3.n[0], v3.n[1], v3.n[2]), Cvec2(0, g_hairyness)));

		}
		
		g_bunnyShellGeometries[i]->upload(&shell[0], facenum * 3);
	}
	g_shellNeedsUpdate = false;
	cout << "Tip position [" << g_tipPos[10][0] << "]" << endl;

}


// New glut timer call back that perform dynamics simulation
// every g_simulationsPerSecond times per second
static void hairsSimulationCallback(int dontCare) {
	// TASK 2 TODO: wrte dynamics simulation code here as part of TASK2
	for (int i = 0; i < g_numStepsPerFrame; i++) {
		for (int j = 0; j < g_bunnyMesh.getNumVertices(); j++) {

			RigTForm bunny = (getPathAccumRbt(g_world, g_bunnyNode));
			
			Cvec3 temp = g_bunnyMesh.getVertex(j).getPosition() + g_bunnyMesh.getVertex(j).getNormal() * g_furHeight;
			Cvec3 s = bunny * temp;
			Cvec3 t = g_tipPos[j];
			Cvec3 v = g_tipVelocity[j];
			Cvec3 p = bunny * g_bunnyMesh.getVertex(j).getPosition();

			// compute total force acting on tip
			Cvec3 force = g_gravity + (s - t) * g_stiffness;

			// update t
			t = t + v * g_timeStep;

			// constrain t
			g_tipPos[j] = (p + (t - p) / norm(t - p) * g_furHeight);

			// update velocity
			g_tipVelocity[j] = ((v + force * g_timeStep) * g_damping);
		}
	}

	// schedule this to get called again
	glutTimerFunc(1000 / g_simulationsPerSecond, hairsSimulationCallback, 0);
	//updateShellGeometry();
	g_shellNeedsUpdate = true;
	glutPostRedisplay(); // signal redisplaying
}

static void initSimulation() {
	g_tipPos.resize(g_bunnyMesh.getNumVertices(), Cvec3(0));
	g_tipVelocity = g_tipPos;

	int facenum = g_bunnyMesh.getNumFaces();

	vector<VertexPNX> verts;
	verts.reserve(facenum * 3);
	for (int z = 0; z < g_bunnyMesh.getNumFaces(); ++z)
	{
		Mesh::Face temp = g_bunnyMesh.getFace(z);
		for (int n = 0; n < temp.getNumVertices() - 2; ++n)
		{
			verts.push_back(VertexPNX(temp.getVertex(n).getPosition(), temp.getVertex(n).getNormal(), Cvec2(0,0)));
			verts.push_back(VertexPNX( temp.getVertex(n + 1).getPosition(), temp.getVertex(n + 1).getNormal(), Cvec2(g_hairyness, 0)));
			verts.push_back(VertexPNX(temp.getVertex(n + 2).getPosition(), temp.getVertex(n + 2).getNormal(), Cvec2(0,g_hairyness)));
		}
	}

	g_bunnyGeometry.reset(new SimpleGeometryPNX());// upload(&verts[0], vertnum);
												  //g_bunnyGeometry->upload(&verts[0], vertnum);
												  // Now allocate array of SimpleGeometryPNX to for shells, one per layer

	g_bunnyGeometry->upload(&verts[0], facenum * 3);
	// TASK 1 TODO: initialize g_tipPos to "at-rest" hair tips in world coordinates

	for (int z = 0; z < g_bunnyMesh.getNumVertices(); ++z)
	{
		g_tipPos.push_back(g_bunnyMesh.getVertex(z).getPosition() + g_bunnyMesh.getVertex(z).getNormal() * g_furHeight);
		g_tipVelocity.push_back(Cvec3(0));

	}
	// tip of fur

	hairsSimulationCallback(0);
}

static void initBunnyMeshes() {
	g_bunnyMesh.load("bunny.mesh");
	int facenum = g_bunnyMesh.getNumFaces();

	vector<VertexPNX> verts;
	verts.reserve(facenum * 3);

	// TODO: Init the per vertex normal of g_bunnyMesh; see "calculating normals"
	// section of spec
	// ...

	// set all vertices to zero for the entire geometry
	for (int i = 0; i < g_bunnyMesh.getNumVertices(); i++)
		g_bunnyMesh.getVertex(i).setNormal(Cvec3(0));

	// for each face - add the face norm to all the vertices 
	for (int z = 0; z < g_bunnyMesh.getNumFaces(); ++z)
	{
		Mesh::Face temp = g_bunnyMesh.getFace(z);
		Cvec3 facenorm = temp.getNormal();

		for (int n = 0; n < temp.getNumVertices(); ++n)
		{
			Mesh::Vertex oldvertex = temp.getVertex(n);
			oldvertex.setNormal(oldvertex.getNormal() + facenorm);
			temp.getVertex(n).setNormal((temp.getVertex(n).getNormal() + facenorm));
		}
	}

	for (int j = 0; j < g_bunnyMesh.getNumVertices(); j++) {
		Mesh::Vertex vertex = g_bunnyMesh.getVertex(j);
		Cvec3 n = vertex.getNormal();
		vertex.setNormal(n.normalize());
		g_tipPos.push_back(g_bunnyMesh.getVertex(j).getPosition() + g_bunnyMesh.getVertex(j).getNormal() * g_furHeight);
		g_tipVelocity.push_back(Cvec3(0));
	}

	// TODO: Initialize g_bunnyGeometry from g_bunnyMesh; see "mesh preparation"
	// section of spec


	for (int z = 0; z < g_bunnyMesh.getNumFaces(); ++z)
	{
		Mesh::Face temp = g_bunnyMesh.getFace(z);
		for (int n = 0; n < temp.getNumVertices() - 2; ++n)
		{
			verts.push_back(VertexPNX(temp.getVertex(n).getPosition(), temp.getVertex(n).getNormal(), Cvec2(0,0)));
			verts.push_back(VertexPNX(temp.getVertex(n + 1).getPosition(), temp.getVertex(n + 1).getNormal(),Cvec2(g_hairyness,0)));
			verts.push_back(VertexPNX(temp.getVertex(n + 2).getPosition(), temp.getVertex(n + 2).getNormal(),Cvec2(0, g_hairyness)));
		}
	}

	g_bunnyGeometry.reset(new SimpleGeometryPNX());// upload(&verts[0], vertnum);
												  //g_bunnyGeometry->upload(&verts[0], vertnum);
												  // Now allocate array of SimpleGeometryPNX to for shells, one per layer


	g_bunnyGeometry->upload(&verts[0], facenum * 3);

	g_bunnyShellGeometries.resize(g_numShells);
	for (int i = 0; i < g_numShells; ++i) {
		g_bunnyShellGeometries[i].reset(new SimpleGeometryPNX());
	}
}

static void initGround() {
	int ibLen, vbLen;
	getPlaneVbIbLen(vbLen, ibLen);

	// Temporary storage for cube Geometry
	vector<VertexPNTBX> vtx(vbLen);
	vector<unsigned short> idx(ibLen);

	makePlane(g_groundSize * 2, vtx.begin(), idx.begin());
	g_ground.reset(new SimpleIndexedGeometryPNTBX(&vtx[0], &idx[0], vbLen, ibLen));
}

static void initCubes() {
	int ibLen, vbLen;
	getCubeVbIbLen(vbLen, ibLen);

	// Temporary storage for cube Geometry
	vector<VertexPNTBX> vtx(vbLen);
	vector<unsigned short> idx(ibLen);

	makeCube(1, vtx.begin(), idx.begin());
	g_cube.reset(new SimpleIndexedGeometryPNTBX(&vtx[0], &idx[0], vbLen, ibLen));
}

static void initSphere() {
	int ibLen, vbLen;
	getSphereVbIbLen(20, 10, vbLen, ibLen);

	// Temporary storage for sphere Geometry
	vector<VertexPNTBX> vtx(vbLen);
	vector<unsigned short> idx(ibLen);
	makeSphere(1, 20, 10, vtx.begin(), idx.begin());
	g_sphere.reset(new SimpleIndexedGeometryPNTBX(&vtx[0], &idx[0], vtx.size(), idx.size()));
}

static void initRobots() {
	// Init whatever extra geometry  needed for the robots
}

// takes a projection matrix and send to the the shaders
static void sendProjectionMatrix(Uniforms& uniforms, const Matrix4& projMatrix) {
	uniforms.put("uProjMatrix", projMatrix);
}

// update g_frustFovY from g_frustMinFov, g_windowWidth, and g_windowHeight
static void updateFrustFovY() {
	if (g_windowWidth >= g_windowHeight)
		g_frustFovY = g_frustMinFov;
	else {
		const double RAD_PER_DEG = 0.5 * CS175_PI / 180;
		g_frustFovY = atan2(sin(g_frustMinFov * RAD_PER_DEG) * g_windowHeight / g_windowWidth, cos(g_frustMinFov * RAD_PER_DEG)) / RAD_PER_DEG;
	}
}

static Matrix4 makeProjectionMatrix() {
	return Matrix4::makeProjection(
		g_frustFovY, g_windowWidth / static_cast <double> (g_windowHeight),
		g_frustNear, g_frustFar);
}

enum ManipMode {
	ARCBALL_ON_PICKED,
	ARCBALL_ON_SKY,
	EGO_MOTION
};

static ManipMode getManipMode() {
	// if nothing is picked or the picked transform is the transfrom we are viewing from
	if (g_currentPickedRbtNode == NULL || g_currentPickedRbtNode == g_currentCameraNode) {
		if (g_currentCameraNode == g_skyNode && g_activeCameraFrame == WORLD_SKY)
			return ARCBALL_ON_SKY;
		else
			return EGO_MOTION;
	}
	else
		return ARCBALL_ON_PICKED;
}

static bool shouldUseArcball() {
	return getManipMode() != EGO_MOTION;
}

// The translation part of the aux frame either comes from the current
// active object, or is the identity matrix when
static RigTForm getArcballRbt() {
	switch (getManipMode()) {
	case ARCBALL_ON_PICKED:
		return getPathAccumRbt(g_world, g_currentPickedRbtNode);
	case ARCBALL_ON_SKY:
		return RigTForm();
	case EGO_MOTION:
		return getPathAccumRbt(g_world, g_currentCameraNode);
	default:
		throw runtime_error("Invalid ManipMode");
	}
}

static void updateArcballScale() {
	RigTForm arcballEye = inv(getPathAccumRbt(g_world, g_currentCameraNode)) * getArcballRbt();
	double depth = arcballEye.getTranslation()[2];
	if (depth > -CS175_EPS)
		g_arcballScale = 0.02;
	else
		g_arcballScale = getScreenToEyeScale(depth, g_frustFovY, g_windowHeight);
}

static void drawArcBall(Uniforms& uniforms) {
	RigTForm arcballEye = inv(getPathAccumRbt(g_world, g_currentCameraNode)) * getArcballRbt();
	Matrix4 MVM = rigTFormToMatrix(arcballEye) * Matrix4::makeScale(Cvec3(1, 1, 1) * g_arcballScale * g_arcballScreenRadius);

	sendModelViewNormalMatrix(uniforms, MVM, normalMatrix(MVM));

	g_arcballMat->draw(*g_sphere, uniforms);
}

static void drawStuff(bool picking) {

	if (g_shellNeedsUpdate)
	{
		updateShellGeometry();
	}

	drawRain(); 
	drawSun();
	drawClouds();
	
	// if we are not translating, update arcball scale
	if (!(g_mouseMClickButton || (g_mouseLClickButton && g_mouseRClickButton) || (g_mouseLClickButton && !g_mouseRClickButton && g_spaceDown)))
		updateArcballScale();

	Uniforms uniforms;

	// build & send proj. matrix to vshader
	const Matrix4 projmat = makeProjectionMatrix();
	sendProjectionMatrix(uniforms, projmat);

	const RigTForm eyeRbt = getPathAccumRbt(g_world, g_currentCameraNode);
	const RigTForm invEyeRbt = inv(eyeRbt);

	Cvec3 l1 = getPathAccumRbt(g_world, g_sun).getTranslation();
	Cvec3 l2 = getPathAccumRbt(g_world, g_sun).getTranslation();
	uniforms.put("uLight", Cvec3(invEyeRbt * Cvec4(l1, 1)));
	uniforms.put("uLight2", Cvec3(invEyeRbt * Cvec4(l2, 1)));


	if (!picking) {
		Drawer drawer(invEyeRbt, uniforms);
		g_world->accept(drawer);

		if (g_displayArcball && shouldUseArcball())
			drawArcBall(uniforms);
	}
	else {
		Picker picker(invEyeRbt, uniforms);

		g_overridingMaterial = g_pickingMat;
		g_world->accept(picker);
		g_overridingMaterial.reset();

		glFlush();
		g_currentPickedRbtNode = picker.getRbtNodeAtXY(g_mouseClickX, g_mouseClickY);
		if (g_currentPickedRbtNode == g_groundNode)
			g_currentPickedRbtNode.reset(); // set to NULL

		cout << (g_currentPickedRbtNode ? "Part picked" : "No part picked") << endl;

	}
}

// static void display() {

// 	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

// 	drawStuff(false);


// 	glutSwapBuffers();

// 	checkGlErrors();
// }

// void drawBitmapText(char *string, float x, float y, float z)
// {
// 	char *c;
	
// 	glRasterPos3f(x, y, z);

// 	for (c = string; *c != '\0'; c++)
// 		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
	
// }

static void display() {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawStuff(false);

	// const unsigned char * string = vstr(tick);

	// glutBitmapString(GLUT_BITMAP_HELVETICA_18, NumberToString(tick) );
	glutSwapBuffers();

	checkGlErrors();
}

// static void display() {
// 	glClearColor;
// 	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

// 	drawStuff(false);

// 	glPushAttrib(GL_CURRENT_BIT);
// 	glColor3f(1.0, 0.0, 0.0);
// 	int tick_int;
// 	tick_int = (int (tick *1.9) + 12) % 24; 
// 	char tickbuffer[10000];

// 	const char* tickstring = vstr(tick_int) + ":00";

// 	glMatrixMode(GL_PROJECTION);
// 	glLoadIdentity();
// 	glOrtho(0, g_windowWidth, 0, g_windowHeight, -1, 1);

// 	glMatrixMode(GL_MODELVIEW);
// 	glLoadIdentity();

// 	glDisable(GL_LIGHTING);
// 	//glColor3f(0.0f, 0.0f, 0.0f);
// 	drawBitmapText((char *) tickstring, g_windowWidth - 100, g_windowHeight - 25, 0);
// 	glEnable(GL_LIGHTING);
// 	glPopAttrib();

// 	glutSwapBuffers();

// 	checkGlErrors();
// }

static void pick() {
	// We need to set the clear color to black, for pick rendering.
	// so let's save the clear color
	GLdouble clearColor[4];
	glGetDoublev(GL_COLOR_CLEAR_VALUE, clearColor);

	glClearColor(0, 0, 0, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawStuff(true);

	// Uncomment below and comment out the glutPostRedisplay in mouse(...) call back
	// to see result of the pick rendering pass
	// glutSwapBuffers();

	//Now set back the clear color
	glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);

	checkGlErrors();
}

bool interpolateAndDisplay(float t) {
	if (t > g_animator.getNumKeyFrames() - 3)
		return true;
	g_animator.animate(t);
	return false;
}

static void animateTimerCallback(int ms) {
	double t = (double)ms / g_msBetweenKeyFrames;
	bool endReached = interpolateAndDisplay(t);
	if (g_playingAnimation && !endReached) {
		glutTimerFunc(1000 / g_animateFramesPerSecond, animateTimerCallback, ms + 1000 / g_animateFramesPerSecond);
	}
	else {
		cerr << "Finished playing animation" << endl;
		g_curKeyFrame = g_animator.keyFramesEnd();
		advance(g_curKeyFrame, -2);
		g_animator.pushKeyFrameToSg(g_curKeyFrame);
		g_playingAnimation = false;

		g_curKeyFrameNum = g_animator.getNumKeyFrames() - 2;
		cerr << "Now at frame [" << g_curKeyFrameNum << "]" << endl;
	}
	display();
}

static void reshape(const int w, const int h) {
	g_windowWidth = w;
	g_windowHeight = h;
	glViewport(0, 0, w, h);
	cerr << "Size of window is now " << w << "x" << h << endl;
	g_arcballScreenRadius = max(1.0, min(h, w) * 0.25);
	updateFrustFovY();
	glutPostRedisplay();
}

static Cvec3 getArcballDirection(const Cvec2& p, const double r) {
	double n2 = norm2(p);
	if (n2 >= r*r)
		return normalize(Cvec3(p, 0));
	else
		return normalize(Cvec3(p, sqrt(r*r - n2)));
}

static RigTForm moveArcball(const Cvec2& p0, const Cvec2& p1) {
	const Matrix4 projMatrix = makeProjectionMatrix();
	const RigTForm eyeInverse = inv(getPathAccumRbt(g_world, g_currentCameraNode));
	const Cvec3 arcballCenter = getArcballRbt().getTranslation();
	const Cvec3 arcballCenter_ec = Cvec3(eyeInverse * Cvec4(arcballCenter, 1));

	if (arcballCenter_ec[2] > -CS175_EPS)
		return RigTForm();

	Cvec2 ballScreenCenter = getScreenSpaceCoord(arcballCenter_ec,
		projMatrix, g_frustNear, g_frustFovY, g_windowWidth, g_windowHeight);
	const Cvec3 v0 = getArcballDirection(p0 - ballScreenCenter, g_arcballScreenRadius);
	const Cvec3 v1 = getArcballDirection(p1 - ballScreenCenter, g_arcballScreenRadius);

	return RigTForm(Quat(0.0, v1[0], v1[1], v1[2]) * Quat(0.0, -v0[0], -v0[1], -v0[2]));
}

static RigTForm doMtoOwrtA(const RigTForm& M, const RigTForm& O, const RigTForm& A) {
	return A * M * inv(A) * O;
}

static RigTForm getMRbt(const double dx, const double dy) {
	RigTForm M;

	if (g_mouseLClickButton && !g_mouseRClickButton && !g_spaceDown) {
		if (shouldUseArcball())
			M = moveArcball(Cvec2(g_mouseClickX, g_mouseClickY), Cvec2(g_mouseClickX + dx, g_mouseClickY + dy));
		else
			M = RigTForm(Quat::makeXRotation(-dy) * Quat::makeYRotation(dx));
	}
	else {
		double movementScale = getManipMode() == EGO_MOTION ? 0.02 : g_arcballScale;
		if (g_mouseRClickButton && !g_mouseLClickButton) {
			M = RigTForm(Cvec3(dx, dy, 0) * movementScale);
		}
		else if (g_mouseMClickButton || (g_mouseLClickButton && g_mouseRClickButton) || (g_mouseLClickButton && g_spaceDown)) {
			M = RigTForm(Cvec3(0, 0, -dy) * movementScale);
		}
	}

	switch (getManipMode()) {
	case ARCBALL_ON_PICKED:
		break;
	case ARCBALL_ON_SKY:
		M = inv(M);
		break;
	case EGO_MOTION:
		if (g_mouseLClickButton && !g_mouseRClickButton && !g_spaceDown) // only invert rotation
			M = inv(M);
		break;
	}
	return M;
}

static RigTForm makeMixedFrame(const RigTForm& objRbt, const RigTForm& eyeRbt) {
	return transFact(objRbt) * linFact(eyeRbt);
}

// l = w X Y Z
// o = l O
// a = w A = l (Z Y X)^1 A = l A'
// o = a (A')^-1 O
//   => a M (A')^-1 O = l A' M (A')^-1 O

static void motion(const int x, const int y) {
	if (!g_mouseClickDown)
		return;

	const double dx = x - g_mouseClickX;
	const double dy = g_windowHeight - y - 1 - g_mouseClickY;

	const RigTForm M = getMRbt(dx, dy);   // the "action" matrix

										  // the matrix for the auxiliary frame (the w.r.t.)
	RigTForm A = makeMixedFrame(getArcballRbt(), getPathAccumRbt(g_world, g_currentCameraNode));

	shared_ptr<SgRbtNode> target;
	switch (getManipMode()) {
	case ARCBALL_ON_PICKED:
		target = g_currentPickedRbtNode;
		break;
	case ARCBALL_ON_SKY:
		target = g_skyNode;
		break;
	case EGO_MOTION:
		target = g_currentCameraNode;
		break;
	}

	A = inv(getPathAccumRbt(g_world, target, 1)) * A;

	target->setRbt(doMtoOwrtA(M, target->getRbt(), A));

	g_mouseClickX += dx;
	g_mouseClickY += dy;
	glutPostRedisplay();  // we always redraw if we changed the scene
}

static void mouse(const int button, const int state, const int x, const int y) {
	g_mouseClickX = x;
	g_mouseClickY = g_windowHeight - y - 1;  // conversion from GLUT window-coordinate-system to OpenGL window-coordinate-system

	g_mouseLClickButton |= (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN);
	g_mouseRClickButton |= (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN);
	g_mouseMClickButton |= (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN);

	g_mouseLClickButton &= !(button == GLUT_LEFT_BUTTON && state == GLUT_UP);
	g_mouseRClickButton &= !(button == GLUT_RIGHT_BUTTON && state == GLUT_UP);
	g_mouseMClickButton &= !(button == GLUT_MIDDLE_BUTTON && state == GLUT_UP);

	g_mouseClickDown = g_mouseLClickButton || g_mouseRClickButton || g_mouseMClickButton;

	if (g_pickingMode && button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		pick();
		g_pickingMode = false;
		cerr << "Picking mode is off" << endl;
		glutPostRedisplay(); // request redisplay since the arcball will have moved
	}
	glutPostRedisplay();
}

static void keyboardUp(const unsigned char key, const int x, const int y) {
	switch (key) {
	case ' ':
		g_spaceDown = false;
		break;
	}
	glutPostRedisplay();
}

static void keyboard(const unsigned char key, const int x, const int y) {
	switch (key) {
	case ' ':
		g_spaceDown = true;
		break;
	case 27:
		exit(0);                                  // ESC
	case 'h':
		cout << " ============== H E L P ==============\n\n"
			<< "h\t\thelp menu\n"
			<< "s\t\tsave screenshot\n"
			<< "p\t\tUse mouse to pick a part to edit\n"
			<< "v\t\tCycle view\n"
			<< "drag left mouse to rotate\n"
			<< "a\t\tToggle display arcball\n"
			<< "w\t\tWrite animation to animation.txt\n"
			<< "i\t\tRead animation from animation.txt\n"
			<< "c\t\tCopy frame to scene\n"
			<< "u\t\tCopy sceneto frame\n"
			<< "n\t\tCreate new frame after current frame and copy scene to it\n"
			<< "d\t\tDelete frame\n"
			<< ">\t\tGo to next frame\n"
			<< "<\t\tGo to prev. frame\n"
			<< "y\t\tPlay/Stop animation\n"
			<< endl;
		break;
	case 's':
		glFlush();
		writePpmScreenshot(g_windowWidth, g_windowHeight, "out.ppm");
		break;
	case 'v':
	{
		shared_ptr<SgRbtNode> viewers[] = { g_skyNode, g_robot1Node, g_robot2Node };
		for (int i = 0; i < 3; ++i) {
			if (g_currentCameraNode == viewers[i]) {
				g_currentCameraNode = viewers[(i + 1) % 3];
				break;
			}
		}
	}
	break;
	case 'p':
		g_pickingMode = !g_pickingMode;
		cerr << "Picking mode is " << (g_pickingMode ? "on" : "off") << endl;
		break;
	case 'm':
		g_activeCameraFrame = SkyMode((g_activeCameraFrame + 1) % 2);
		cerr << "Editing sky eye w.r.t. " << (g_activeCameraFrame == WORLD_SKY ? "world-sky frame\n" : "sky-sky frame\n") << endl;
		break;
	case 'a':
		g_displayArcball = !g_displayArcball;
		break;
	case 'u':
		if (g_playingAnimation) {
			cerr << "Cannot operate when playing animation" << endl;
			break;
		}

		if (g_curKeyFrame == g_animator.keyFramesEnd()) { // only possible when frame list is empty
			cerr << "Create new frame [0]." << endl;
			g_curKeyFrame = g_animator.insertEmptyKeyFrameAfter(g_animator.keyFramesBegin());
			g_curKeyFrameNum = 0;
		}
		cerr << "Copying scene graph to current frame [" << g_curKeyFrameNum << "]" << endl;
		g_animator.pullKeyFrameFromSg(g_curKeyFrame);
		break;
	case 'n':
		if (g_playingAnimation) {
			cerr << "Cannot operate when playing animation" << endl;
			break;
		}
		if (g_animator.getNumKeyFrames() != 0)
			++g_curKeyFrameNum;
		g_curKeyFrame = g_animator.insertEmptyKeyFrameAfter(g_curKeyFrame);
		g_animator.pullKeyFrameFromSg(g_curKeyFrame);
		cerr << "Create new frame [" << g_curKeyFrameNum << "]" << endl;
		break;
	case 'c':
		if (g_playingAnimation) {
			cerr << "Cannot operate when playing animation" << endl;
			break;
		}
		if (g_curKeyFrame != g_animator.keyFramesEnd()) {
			cerr << "Loading current key frame [" << g_curKeyFrameNum << "] to scene graph" << endl;
			g_animator.pushKeyFrameToSg(g_curKeyFrame);
		}
		else {
			cerr << "No key frame defined" << endl;
		}
		break;
	case 'd':
		if (g_playingAnimation) {
			cerr << "Cannot operate when playing animation" << endl;
			break;
		}
		if (g_curKeyFrame != g_animator.keyFramesEnd()) {
			Animator::KeyFrameIter newCurKeyFrame = g_curKeyFrame;
			cerr << "Deleting current frame [" << g_curKeyFrameNum << "]" << endl;;
			if (g_curKeyFrame == g_animator.keyFramesBegin()) {
				++newCurKeyFrame;
			}
			else {
				--newCurKeyFrame;
				--g_curKeyFrameNum;
			}
			g_animator.deleteKeyFrame(g_curKeyFrame);
			g_curKeyFrame = newCurKeyFrame;
			if (g_curKeyFrame != g_animator.keyFramesEnd()) {
				g_animator.pushKeyFrameToSg(g_curKeyFrame);
				cerr << "Now at frame [" << g_curKeyFrameNum << "]" << endl;
			}
			else
				cerr << "No frames defined" << endl;
		}
		else {
			cerr << "Frame list is now EMPTY" << endl;
		}
		break;
	case '>':
		if (g_playingAnimation) {
			cerr << "Cannot operate when playing animation" << endl;
			break;
		}
		if (g_curKeyFrame != g_animator.keyFramesEnd()) {
			if (++g_curKeyFrame == g_animator.keyFramesEnd())
				--g_curKeyFrame;
			else {
				++g_curKeyFrameNum;
				g_animator.pushKeyFrameToSg(g_curKeyFrame);
				cerr << "Stepped forward to frame [" << g_curKeyFrameNum << "]" << endl;
			}
		}
		break;
	case '<':
		if (g_playingAnimation) {
			cerr << "Cannot operate when playing animation" << endl;
			break;
		}
		if (g_curKeyFrame != g_animator.keyFramesBegin()) {
			--g_curKeyFrame;
			--g_curKeyFrameNum;
			g_animator.pushKeyFrameToSg(g_curKeyFrame);
			cerr << "Stepped backward to frame [" << g_curKeyFrameNum << "]" << endl;
		}
		break;
	case 'w':
		cerr << "Writing animation to animation.txt\n";
		g_animator.saveAnimation("animation.txt");
		break;
	case 'i':
		if (g_playingAnimation) {
			cerr << "Cannot operate when playing animation" << endl;
			break;
		}
		cerr << "Reading animation from animation.txt\n";
		g_animator.loadAnimation("animation.txt");
		g_curKeyFrame = g_animator.keyFramesBegin();
		cerr << g_animator.getNumKeyFrames() << " frames read.\n";
		if (g_curKeyFrame != g_animator.keyFramesEnd()) {
			g_animator.pushKeyFrameToSg(g_curKeyFrame);
			cerr << "Now at frame [0]" << endl;
		}
		g_curKeyFrameNum = 0;
		break;
	case '-':
		g_msBetweenKeyFrames = min(g_msBetweenKeyFrames + 100, 10000);
		cerr << g_msBetweenKeyFrames << " ms between keyframes.\n";
		break;
	case '+':
		g_msBetweenKeyFrames = max(g_msBetweenKeyFrames - 100, 100);
		cerr << g_msBetweenKeyFrames << " ms between keyframes.\n";
		break;
	case 'y':
		if (!g_playingAnimation) {
			if (g_animator.getNumKeyFrames() < 4) {
				cerr << " Cannot play animation with less than 4 keyframes." << endl;
			}
			else {
				g_playingAnimation = true;
				cerr << "Playing animation... " << endl;
				animateTimerCallback(0);
			}
		}
		else {
			cerr << "Stopping animation... " << endl;
			g_playingAnimation = false;
		}
		break;
	 case 'r': 
	 	weather = Weather((weather + 1) % 3);
	 	if (weather == RAIN) {
	 		initParticles();
	 		cerr << "weather forecast is rainy\n" << endl;
	 	}
	 	else if (weather == SNOW) {
	 		cerr << "weather forecast is snowy\n" << endl;
	 	}
	 	else {
	 		//bloat = 0.0;
	 		for (int i = 0; i < PARTICLES; i++) 
	 			g_world->removeChild(particle_system[i].node);
	 		cerr << "weather forecast is clear\n" << endl;
	 	}
	 	break;
	}

	// Sanity check that our g_curKeyFrameNum is in sync with the g_curKeyFrame
	if (g_animator.getNumKeyFrames() > 0)
		assert(g_animator.getNthKeyFrame(g_curKeyFrameNum) == g_curKeyFrame);

	glutPostRedisplay();
}

void idle(void) {
	glutPostRedisplay();
}

static void initGlutState(int argc, char * argv[]) {
	glutInit(&argc, argv);                                  // initialize Glut based on cmd-line args
#ifdef __MAC__
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH); // core profile flag is required for GL 3.2 on Mac
#else
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);  // RGBA pixel channels and double buffering
#endif

	glutInitWindowSize(g_windowWidth, g_windowHeight);      // create a window
	glutCreateWindow("Final Project");                     // title the window

	glutDisplayFunc(display);                               // display rendering callback
	glutReshapeFunc(reshape);                               // window reshape callback
	glutMotionFunc(motion);                                 // mouse movement callback
	glutMouseFunc(mouse);                                   // mouse click callback
	glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyboardUp);
	glutIdleFunc(idle);	
}

static void initGLState() {
	glClearColor(128. / 255., 200. / 255., 255. / 255., 0.);
	glClearDepth(0.);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_GREATER);
	glReadBuffer(GL_BACK);
	if (!g_Gl2Compatible)
		glEnable(GL_FRAMEBUFFER_SRGB);
}

static void initMaterials() {
#if EMBED_SOLUTION_GLSL
	const char *NORMAL_GL3_FS =
		"#version 150\n"
		"\n"
		"uniform sampler2D uTexColor;\n"
		"uniform sampler2D uTexNormal;\n"
		"\n"
		"// lights in eye space\n"
		"uniform vec3 uLight;\n"
		"uniform vec3 uLight2;\n"
		"\n"
		"in vec2 vTexCoord;\n"
		"in mat3 vNTMat;\n"
		"in vec3 vEyePos;\n"
		"\n"
		"out vec4 fragColor;\n"
		"\n"
		"void main() {\n"
		"  vec3 normal = texture(uTexNormal, vTexCoord).xyz * 2.0 - 1.0;\n"
		"\n"
		"  normal = normalize(vNTMat * normal);\n"
		"\n"
		"  vec3 viewDir = normalize(-vEyePos);\n"
		"  vec3 lightDir = normalize(uLight - vEyePos);\n"
		"  vec3 lightDir2 = normalize(uLight2 - vEyePos);\n"
		"\n"
		"  float nDotL = dot(normal, lightDir);\n"
		"  vec3 reflection = normalize( 2.0 * normal * nDotL - lightDir);\n"
		"  float rDotV = max(0.0, dot(reflection, viewDir));\n"
		"  float specular = pow(rDotV, 32.0);\n"
		"  float diffuse = max(nDotL, 0.0);\n"
		"\n"
		"  nDotL = dot(normal, lightDir2);\n"
		"  reflection = normalize( 2.0 * normal * nDotL - lightDir2);\n"
		"  rDotV = max(0.0, dot(reflection, viewDir));\n"
		"  specular += pow(rDotV, 32.0);\n"
		"  diffuse += max(nDotL, 0.0);\n"
		"\n"
		"  vec3 color = texture(uTexColor, vTexCoord).xyz * diffuse + specular * vec3(0.6, 0.6, 0.6);\n"
		"\n"
		"  fragColor = vec4(color, 1);\n"
		"}\n";

	const char *NORMAL_GL2_FS =
		"uniform sampler2D uTexColor;\n"
		"uniform sampler2D uTexNormal;\n"
		"\n"
		"// lights in eye space\n"
		"uniform vec3 uLight;\n"
		"uniform vec3 uLight2;\n"
		"\n"
		"varying vec2 vTexCoord;\n"
		"varying mat3 vNTMat;\n"
		"varying vec3 vEyePos;\n"
		"\n"
		"void main() {\n"
		"  vec3 normal = texture2D(uTexNormal, vTexCoord).xyz * 2.0 - 1.0;\n"
		"\n"
		"  normal = normalize(vNTMat * normal);\n"
		"\n"
		"  vec3 viewDir = normalize(-vEyePos);\n"
		"  vec3 lightDir = normalize(uLight - vEyePos);\n"
		"  vec3 lightDir2 = normalize(uLight2 - vEyePos);\n"
		"\n"
		"  float nDotL = dot(normal, lightDir);\n"
		"  vec3 reflection = normalize( 2.0 * normal * nDotL - lightDir);\n"
		"  float rDotV = max(0.0, dot(reflection, viewDir));\n"
		"  float specular = pow(rDotV, 32.0);\n"
		"  float diffuse = max(nDotL, 0.0);\n"
		"\n"
		"  nDotL = dot(normal, lightDir2);\n"
		"  reflection = normalize( 2.0 * normal * nDotL - lightDir2);\n"
		"  rDotV = max(0.0, dot(reflection, viewDir));\n"
		"  specular += pow(rDotV, 32.0);\n"
		"  diffuse += max(nDotL, 0.0);\n"
		"\n"
		"  vec3 color = texture2D(uTexColor, vTexCoord).xyz * diffuse + specular * vec3(0.6, 0.6, 0.6);\n"
		"  gl_FragColor = vec4(color, 1);\n"
		"}\n";
	Material::addInlineSource("./shaders/normal-gl3.fshader", strlen(NORMAL_GL3_FS), NORMAL_GL3_FS);
	Material::addInlineSource("./shaders/normal-gl2.fshader", strlen(NORMAL_GL2_FS), NORMAL_GL2_FS);
#endif

	// Create some prototype materials
	Material diffuse("./shaders/basic-gl3.vshader", "./shaders/diffuse-gl3.fshader");
	Material solid("./shaders/basic-gl3.vshader", "./shaders/solid-gl3.fshader");

	// copy diffuse prototype and set red color
	g_redDiffuseMat.reset(new Material(diffuse));
	g_redDiffuseMat->getUniforms().put("uColor", Cvec3f(1, 0, 0));

	// copy diffuse prototype and set blue color
	g_blueDiffuseMat.reset(new Material(diffuse));
	g_blueDiffuseMat->getUniforms().put("uColor", Cvec3f(0, 0, 1));

	// copy diffuse prototype and set yellow color
	g_sunMat.reset(new Material(solid));
	g_sunMat->getUniforms().put("uColor", Cvec3f(1, 1, 0));

	// copy solid prototype, and set to color white
	g_greenSolidMat.reset(new Material(solid));
	g_greenSolidMat->getUniforms().put("uColor", Cvec3f(0, 1, .2));

	// normal mapping
	g_bumpFloorMat.reset(new Material("./shaders/normal-gl3.vshader", "./shaders/normal-gl3.fshader"));
	g_bumpFloorMat->getUniforms().put("uTexColor", shared_ptr<ImageTexture>(new ImageTexture("Fieldstone.ppm", true)));
	g_bumpFloorMat->getUniforms().put("uTexNormal", shared_ptr<ImageTexture>(new ImageTexture("FieldstoneNormal.ppm", false)));

	// copy solid prototype, and set to wireframed rendering
	g_arcballMat.reset(new Material(solid));
	g_arcballMat->getUniforms().put("uColor", Cvec3f(1.f, 1.f, 1.f));
	g_arcballMat->getRenderStates().polygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// copy solid prototype, and set to color white
	g_lightMat.reset(new Material(solid));
	g_lightMat->getUniforms().put("uColor", Cvec3f(1, 1, 1));

	g_stormMat.reset(new Material(solid));
	g_stormMat->getUniforms().put("uColor", Cvec3f(.8, .8, .8));

	// pick shader
	g_pickingMat.reset(new Material("./shaders/basic-gl3.vshader", "./shaders/pick-gl3.fshader"));


	//put it here!!
	// bunny material
	g_bunnyMat.reset(new Material("./shaders/basic-gl3.vshader", "./shaders/bunny-gl3.fshader"));
	g_bunnyMat->getUniforms()
		.put("uColorAmbient", Cvec3f(0.45f, 0.3f, 0.3f))
		.put("uColorDiffuse", Cvec3f(0.2f, 0.2f, 0.2f));

	// bunny shell materials;
	shared_ptr<ImageTexture> shellTexture(new ImageTexture("shell.ppm", false)); // common shell texture

																				 // needs to enable repeating of texture coordinates
	shellTexture->bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// eachy layer of the shell uses a different material, though the materials will share the
	// same shader files and some common uniforms. hence we create a prototype here, and will
	// copy from the prototype later
	Material bunnyShellMatPrototype("./shaders/bunny-shell-gl3.vshader", "./shaders/bunny-shell-gl3.fshader");
	bunnyShellMatPrototype.getUniforms().put("uTexShell", shellTexture);
	bunnyShellMatPrototype.getRenderStates()
		.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) // set blending mode
		.enable(GL_BLEND) // enable blending
		.disable(GL_CULL_FACE); // disable culling

								// allocate array of materials
	g_bunnyShellMats.resize(g_numShells);
	for (int i = 0; i < g_numShells; ++i) {
		g_bunnyShellMats[i].reset(new Material(bunnyShellMatPrototype)); // copy from the prototype
																		 // but set a different exponent for blending transparency
		g_bunnyShellMats[i]->getUniforms().put("uAlphaExponent", 2.f + 5.f * float(i + 1) / g_numShells);
	}

};

static void initGeometry() {
	initGround();
	initCubes();
	initSphere();
	initRobots();
	initBunnyMeshes();
}

static void constructRobot(shared_ptr<SgTransformNode> base, shared_ptr<Material> material) {
	const double ARM_LEN = 0.7,
		ARM_THICK = 0.25,
		LEG_LEN = 1,
		LEG_THICK = 0.25,
		TORSO_LEN = 1.5,
		TORSO_THICK = 0.25,
		TORSO_WIDTH = 1,
		HEAD_SIZE = 0.7;
	const int NUM_JOINTS = 10,
		NUM_SHAPES = 10;

	struct JointDesc {
		int parent;
		float x, y, z;
	};

	JointDesc jointDesc[NUM_JOINTS] = {
		{ -1 }, // torso
		{ 0,  TORSO_WIDTH / 2, TORSO_LEN / 2, 0 }, // upper right arm
		{ 0, -TORSO_WIDTH / 2, TORSO_LEN / 2, 0 }, // upper left arm
		{ 1,  ARM_LEN, 0, 0 }, // lower right arm
		{ 2, -ARM_LEN, 0, 0 }, // lower left arm
		{ 0, TORSO_WIDTH / 2 - LEG_THICK / 2, -TORSO_LEN / 2, 0 }, // upper right leg
		{ 0, -TORSO_WIDTH / 2 + LEG_THICK / 2, -TORSO_LEN / 2, 0 }, // upper left leg
		{ 5, 0, -LEG_LEN, 0 }, // lower right leg
		{ 6, 0, -LEG_LEN, 0 }, // lower left
		{ 0, 0, TORSO_LEN / 2, 0 } // head
	};

	struct ShapeDesc {
		int parentJointId;
		float x, y, z, sx, sy, sz;
		shared_ptr<Geometry> geometry;
		shared_ptr<Material> material;
	};

	ShapeDesc shapeDesc[NUM_SHAPES] = {
		{ 0, 0,         0, 0, TORSO_WIDTH, TORSO_LEN, TORSO_THICK, g_cube, material }, // torso
		{ 1, ARM_LEN / 2, 0, 0, ARM_LEN / 2, ARM_THICK / 2, ARM_THICK / 2, g_sphere, material }, // upper right arm
		{ 2, -ARM_LEN / 2, 0, 0, ARM_LEN / 2, ARM_THICK / 2, ARM_THICK / 2, g_sphere, material }, // upper left arm
		{ 3, ARM_LEN / 2, 0, 0, ARM_LEN, ARM_THICK, ARM_THICK, g_cube, material }, // lower right arm
		{ 4, -ARM_LEN / 2, 0, 0, ARM_LEN, ARM_THICK, ARM_THICK, g_cube, material }, // lower left arm
		{ 5, 0, -LEG_LEN / 2, 0, LEG_THICK / 2, LEG_LEN / 2, LEG_THICK / 2, g_sphere, material }, // upper right leg
		{ 6, 0, -LEG_LEN / 2, 0, LEG_THICK / 2, LEG_LEN / 2, LEG_THICK / 2, g_sphere, material }, // upper left leg
		{ 7, 0, -LEG_LEN / 2, 0, LEG_THICK, LEG_LEN, LEG_THICK, g_cube, material }, // lower right leg
		{ 8, 0, -LEG_LEN / 2, 0, LEG_THICK, LEG_LEN, LEG_THICK, g_cube, material }, // lower left leg
		{ 9, 0, HEAD_SIZE / 2 * 1.5, 0, HEAD_SIZE / 2, HEAD_SIZE / 2, HEAD_SIZE / 2, g_sphere, material }, // head
	};

	shared_ptr<SgTransformNode> jointNodes[NUM_JOINTS];

	for (int i = 0; i < NUM_JOINTS; ++i) {
		if (jointDesc[i].parent == -1)
			jointNodes[i] = base;
		else {
			jointNodes[i].reset(new SgRbtNode(RigTForm(Cvec3(jointDesc[i].x, jointDesc[i].y, jointDesc[i].z))));
			jointNodes[jointDesc[i].parent]->addChild(jointNodes[i]);
		}
	}
	for (int i = 0; i < NUM_SHAPES; ++i) {
		shared_ptr<MyShapeNode> shape(
			new MyShapeNode(shapeDesc[i].geometry,
				shapeDesc[i].material,
				Cvec3(shapeDesc[i].x, shapeDesc[i].y, shapeDesc[i].z),
				Cvec3(0, 0, 0),
				Cvec3(shapeDesc[i].sx, shapeDesc[i].sy, shapeDesc[i].sz)));
		jointNodes[shapeDesc[i].parentJointId]->addChild(shape);
	}
}

static void initScene() {
	g_world.reset(new SgRootNode());


	g_skyNode.reset(new SgRbtNode(RigTForm(Cvec3(0.0, 10.0, 30.0))));

	g_groundNode.reset(new SgRbtNode(RigTForm(Cvec3(0, g_groundY, 0))));
	g_groundNode->addChild(shared_ptr<MyShapeNode>(
		new MyShapeNode(g_ground, g_bumpFloorMat)));

	// create a single transform node for both the bunny and the bunny shells
	g_bunnyNode.reset(new SgRbtNode());

	// add bunny as a shape nodes
	g_bunnyNode->addChild(shared_ptr<MyShapeNode>(
		new MyShapeNode(g_bunnyGeometry, g_bunnyMat)));

	// add each shell as shape node
	for (int i = 0; i < g_numShells; ++i) {
		g_bunnyNode->addChild(shared_ptr<MyShapeNode>(
			new MyShapeNode(g_bunnyShellGeometries[i], g_bunnyShellMats[i])));
	}

	g_robot1Node.reset(new SgRbtNode(RigTForm(Cvec3(-2, 1, 0))));
	g_robot2Node.reset(new SgRbtNode(RigTForm(Cvec3(2, 1, 0))));

	constructRobot(g_robot1Node, g_redDiffuseMat); // a Red robot
	constructRobot(g_robot2Node, g_blueDiffuseMat); // a Blue robot

	//g_light1.reset(new SgRbtNode(RigTForm(Cvec3(4.0, 8.0, 5.0))));
	g_sun.reset(new SgRbtNode(RigTForm(Cvec3(g_groundSize, -2.0, 0.0))));

	//g_light1->addChild(shared_ptr<MyShapeNode>(
	//	new MyShapeNode(g_sphere, g_lightMat, Cvec3(0), Cvec3(0), Cvec3(0.5))));

	sun = shared_ptr<MyShapeNode>(
		new MyShapeNode(g_sphere, g_sunMat, Cvec3(0), Cvec3(0), Cvec3(2.0)));
	g_sun->addChild(sun);


	g_world->addChild(g_sun);

	g_world->addChild(g_skyNode);
	g_world->addChild(g_groundNode);
	g_world->addChild(g_robot1Node);
	g_world->addChild(g_robot2Node);
	g_world->addChild(g_bunnyNode);

	//g_world->addChild(g_light1);

	g_currentCameraNode = g_skyNode;

}

static void initAnimation() {
	g_animator.attachSceneGraph(g_world);
	g_curKeyFrame = g_animator.keyFramesBegin();
}

int main(int argc, char * argv[]) {
	try {
		initGlutState(argc, argv);

		// on Mac, we shouldn't use GLEW.

#ifndef __MAC__
		glewInit(); // load the OpenGL extensions
#endif

		cout << (g_Gl2Compatible ? "Will use OpenGL 2.x / GLSL 1.0" : "Will use OpenGL 3.x / GLSL 1.5") << endl;

#ifndef __MAC__
		if ((!g_Gl2Compatible) && !GLEW_VERSION_3_0)
			throw runtime_error("Error: card/driver does not support OpenGL Shading Language v1.3");
		else if (g_Gl2Compatible && !GLEW_VERSION_2_0)
			throw runtime_error("Error: card/driver does not support OpenGL Shading Language v1.0");
#endif

		initGLState();
		initMaterials();
		initGeometry();
		initScene();
		initAnimation();
		initParticles(); 
		initClouds();
		glutMainLoop();
		return 0;
	}
	catch (const runtime_error& e) {
		cout << "Exception caught: " << e.what() << endl;
		return -1;
	}
}