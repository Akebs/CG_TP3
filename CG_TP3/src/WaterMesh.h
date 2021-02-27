// based on https://www.gamasutra.com/view/feature/131531/refractive_texture_mapping_part_

#include <GL/glew.h>
#include <GL/glut.h>
#include <iostream>
#include "OBJ_Loader.h"
#include "camera.h"


using namespace std;
using namespace objl;


//	class Mesh
const int MAP_SIZE_XX 			= 240;
const int MAP_SIZE_YY			= 240;
const int MAP_SIZE_ZZ 			= 240;
const int MAX_SPRINGS 			= ((MAP_SIZE_XX+1) * MAP_SIZE_ZZ * 2) // Horizontal and vertical
									 + (MAP_SIZE_XX * MAP_SIZE_ZZ * 2); // Diagonal
const float WATER_DEPTH     	= 5.0f;
const float SPRING_TENSION 		= 400.0f;
const float SHEAR_TENSION  		= 300.0f;
const float GROUND_TENSION 		= 100.0f;
const float SPRING_DAMPING  	= 1.0f;
const float GROUND_DAMPING  	= 0.3f;
const float GRAVITY         	= 0.0f;
const float SPLASH_FORCE    	= 4.0f;
const float PARTICLE_MASS		= 1.0f;		// 1 kg (F = ma)
const float REFRACTION_FACTOR   = 0.2f;
static int mesh_offset_CC 		= 0;
static int mesh_offset_RR 		= 0;

bool listening2 			= false;
int   numSprings;
static Camera 	camera ;


// Description: The namespace for water mesh requirements
namespace watermesh {
	// mesh components structure definition
	struct Particle{
		Vector3 position;
		Vector3 velocity;
		Vector3 acceleration;
		Vector3 forces;
		Vector3 normal;
		Vector3 texCoords;
		bool fixed;
	};

	// spring components structure definition
	struct Spring	{
		Particle p1;
		Particle p2;
		float tension;
		float damping;
		float restLength;
	};


	static Particle mesh  [MAP_SIZE_ZZ+1][MAP_SIZE_XX+1];
	static Spring 	cloth [MAX_SPRINGS];


	Spring makeSpring(Particle left, Particle right, float springTension = SPRING_TENSION) {
		Spring spr;
		spr.p1 = left;
		spr.p2 = right;
		spr.tension = springTension;
		spr.damping = SPRING_DAMPING;
		spr.restLength = (right.position - left.position).length();
		return spr;
	}

	// Unlock all particles
	void unlockParticles(){
		for (int r= 0; r <= MAP_SIZE_ZZ;++r){
			for (int c= 0; c <= MAP_SIZE_XX;++c){
				mesh[r][c].fixed = false;
			}
		}
	}

	// update mesh displacement
	void updateMeshOffset(Vector3 oldppos, Vector3 newpos ){
		mesh_offset_CC = (MAP_SIZE_XX/2 ) -newpos.Z;
		mesh_offset_RR = (MAP_SIZE_ZZ/2 ) -newpos.X;
		if (mesh_offset_CC < 0)
			mesh_offset_CC = 0;
		if (mesh_offset_CC > MAP_SIZE_XX)
			mesh_offset_CC = MAP_SIZE_XX;
		if (mesh_offset_RR < 0)
			mesh_offset_RR = 0;
		if (mesh_offset_RR > MAP_SIZE_ZZ)
			mesh_offset_RR = MAP_SIZE_ZZ;
		unlockParticles();
	}

	// update forces according to vertex positions
	void updateForces () {
		Vector3 deltaX;
		Vector3 deltaV;
		Vector3 f1, f2;
		Particle ground;
		int r, c;
		// reset forces
		for (r =0; r <= MAP_SIZE_ZZ; ++r) {
				for (c = 0; c <= MAP_SIZE_XX; ++c) {
				mesh[r][c].forces = Vector3(0.0f,0.0f,0.0f);
			}
		}
		if (listening2) 		cout << "updateForces resetted rc:" << r <<" " << c << endl;
		// Compute gravitational force
		if (GRAVITY > 0.0f)		{
			for (r =0; r <= MAP_SIZE_ZZ; ++r) {
				for (c = 0; c <= MAP_SIZE_XX; ++c) {
					Particle& p1 =mesh[r][c];
					if (p1.position.Y > 0.0f) {
						p1.forces.Y += -GRAVITY;
					}
				}
			}
			if (listening2)		cout << "updateForces gravity rc:" << r <<" " << c << endl;
		}
		// Compute ground reaction forces
		for (r =0; r <= MAP_SIZE_ZZ; ++r) {
			for (c = 0; c <= MAP_SIZE_XX; ++c) {
				Particle& p1 = mesh[r][c];
				ground = p1;
				ground.position.Y = -WATER_DEPTH;
				deltaX = p1.position - ground.position;
				deltaV = p1.velocity;
				float force = (
					GROUND_TENSION * (deltaX.length() - WATER_DEPTH) +
					GROUND_DAMPING * (math::DotV3( deltaV, deltaX ) / deltaX.length())
				);
				f1 = deltaX.normalized() * -force  ;
				p1.forces.Y += f1.Y;
			}
		}
		if (listening2)			cout << "updateForces ground rc:" << r <<" " << c << endl;

		// Compute mesh spring forces
		int i;
		for ( i = 0; i < numSprings; ++i)	{
			Spring& s = cloth[i];
			Particle& p1 = s.p1;
			Particle& p2 = s.p2;
			deltaX = p1.position - p2.position;
			deltaV = p1.velocity - p2.velocity;
			if (p1.velocity.length() > 0.0f){
			cout << "  p1.velocity     " << p1.velocity.length()  <<  endl;
			}
			if (p2.velocity.length() > 0.0f){
			cout << "  p2.velocity     " << p2.velocity.length()  <<  endl;
			}

			float force = (
				s.tension * (deltaX.length() - s.restLength)  +
				s.damping * (math::DotV3( deltaV, deltaX ) / deltaX.length())
			);
			if (force > 0.0f){
				cout << " p1.forces.Y:     " << p1.forces.Y << "   		position:     " << p1.position.X << " 	" << p1.position.Y <<  endl;
				cout << " p2.forces.Y:     " << p2.forces.Y << "   		position:     " << p2.position.X << " 	" << p2.position.Y <<  endl;
			}
			f1 = -deltaX.normalized() * force;
			f2 = -f1;
			p1.forces.Y += f1.Y;
			p2.forces.Y += f2.Y;
		}
	}

	// Derive vertex new positions due forces applied
	void applyForces (float time) {
		int r, c;
		for (r =0; r <= MAP_SIZE_ZZ; ++r) {
			for (c = 0; c <= MAP_SIZE_XX; ++c) {
				Particle& p = mesh[r][c];
				if (!p.fixed)	{
					p.acceleration = p.forces / PARTICLE_MASS;
					p.velocity += (p.acceleration * time);
					p.position += (p.velocity * time); // TODO velocity * boat.speed
//					if (p.acceleration.X > 10 || p.acceleration.Y > 10 || p.acceleration.Z > 10 || p.velocity.X > 10|| p.velocity.Y > 100|| p.velocity.Z > 10 || p.position.Y > 10) {
//						cout << " not fixed acceleration: " << p.acceleration.X << " 	" << p.acceleration.Y << " 	" << p.acceleration.Z << endl;
//						cout << "   		velocity:     " << p.velocity.X << " 	" << p.velocity.Y << " 	" << p.velocity.Z << endl;
//						cout << "   		position:     " << p.position.X << " 	" << p.position.Y << " 	" << p.position.Z << endl;
//					}
				}
			}
		}
		if (listening2)			cout << "applyForces       rc:" << r <<" " << c << endl;
	}

	// Get face normals and texture coordinates_
	void genNormals () {
		static Vector3 normals [MAP_SIZE_ZZ][MAP_SIZE_XX];
		int r, c;
		// get face normal
		for (r =0; r < MAP_SIZE_ZZ; ++r) {
			for (c = 0; c < MAP_SIZE_XX ; ++c) {
					const Vector3& v0 = mesh[r][c].position;
					const Vector3& v1 = mesh[r][c+1].position;
					const Vector3& v2 = mesh[r+1][c].position;
					normals[r][c] = math::CrossV3(v2 - v0, v1 - v0 ).normalized();
			}
		}
		// averaged face normal with surrounding vertex normal values
		Vector3 normal;
		for (r = 0; r <= MAP_SIZE_ZZ; ++r) {
			for (c = 0; c <= MAP_SIZE_XX; ++c) {
				normal = Vector3(0.0,0.0,0.0);
				int count = 0;
				if (r > 0 && c > 0) {
					normal += normals[r-1][c-1];
					count++;
				}
				if (r > 0 && c < MAP_SIZE_XX) {
					normal += normals[r-1][c];
					count++;
				}
				if (r < MAP_SIZE_ZZ && c < MAP_SIZE_XX) {
					normal += normals[r][c];
					count++;
				}
				if (r <  MAP_SIZE_ZZ && c > 0) {
					normal += normals[r][c-1];
					count++;
				}
				mesh[r][c].normal = normal / count;
			}
		}
		if (listening2)			cout << "genNormals  avg  rc:" << r <<" " << c << endl;
	}

	// Compute texture coordinates
	void genTexCoordinates () {
		int r, c;
		Vector3 incident;
		Vector3 refracted;
		for (r = 0; r <= MAP_SIZE_ZZ ; ++r) {
			for (c = 0; c <= MAP_SIZE_XX; ++c) {
				Particle& p = mesh[r][c];
				incident  = (p.position - camera.Position).normalized();
				refracted = (-p.normal * REFRACTION_FACTOR + incident).normalized();
				float depth = WATER_DEPTH + p.position.Y;
				float refractionCoef = depth / -refracted.Y;
				p.texCoords.X = (p.position.X + refracted.X * refractionCoef) / MAP_SIZE_XX;
				p.texCoords.Y = (p.position.Z + refracted.Z * refractionCoef) / MAP_SIZE_ZZ;
			}
		}
		if (listening2)
			cout << "gentexcoordin rc:" << r <<" " << c << endl;
	}

	// Initialize particles
	void resetMesh (){
		int r, c;
		for (r = 0; r <= MAP_SIZE_ZZ; ++r) {
			for (c = 0; c <= MAP_SIZE_XX; ++c) {
				Particle& p = mesh[r][c];
				p.position = Vector3( (float) c, 0.0f, (float) r );
				p.velocity = Vector3(0.0f,0.0f,0.0f);
				p.acceleration = Vector3(0.0f,0.0f,0.0f);
				p.forces = Vector3(0.0f,0.0f,0.0f);
				p.fixed = (r == 0 || c == 0 || r == MAP_SIZE_ZZ || c == MAP_SIZE_XX );
			}
		}
		if (listening2)			cout << "resetMesh particles rc:" << r <<" " << c << endl;
		// Initialize springs
		int s = 0;
		for (r = 0; r <= MAP_SIZE_ZZ; ++r) {
			for (c = 0; c <= MAP_SIZE_XX; ++c) {
				if (c < MAP_SIZE_XX) {							// transversal  springs
					cloth[s++] = makeSpring( mesh[r][c], mesh[r][c+1] );
				}
				if (r < MAP_SIZE_ZZ) {							// longitudinal springs
					cloth[s++] = makeSpring( mesh[r][c], mesh[r+1][c] );
				}
				if (c < MAP_SIZE_XX && r < MAP_SIZE_ZZ)	{		// diagonal springs
					cloth[s++] = makeSpring( mesh[r][c], mesh[r+1][c+1], SHEAR_TENSION );
					cloth[s++] = makeSpring( mesh[r+1][c], mesh[r][c+1], SHEAR_TENSION );
				}
			}
			if (listening2)	cout << "cloth:" << r << " " << c << ". Position: "<< mesh[r][c].position.X << " "  << mesh[r][c+1].position.X << " " << mesh[r+1][c].position.X << " " << mesh[r+1][c+1].position.X << endl;
		}
		numSprings = s;
		if (listening2)		cout << "water resetted. number of spings: " <<numSprings <<". RC: " << r-1 <<" " << c-1 << endl;
	}
}
