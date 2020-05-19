// based on learnopengl.com Getting-started Camera sample code
// https://learnopengl.com/Getting-started/Camera



#include <GL/glut.h>
//#include <GL/freeglut.h>
#include "OBJ_Loader.h"
#include <vector>

using namespace objl;

// Default camera values
const float YAW         =   0.0f;
const float PITCH       =   3.0f;
const float SPEED       =   5.0f;
const float SENSITIVITY =   0.4f;
const float ZOOM        =  16.0f;
static float DISTANCE   = 29.0f;

// An abstract camera class that processes input and calculates the corresponding Euler Angles,
class Camera
{
public:
    // Camera Attributes
	Vector3 Position;
    Vector3 Front;
    Vector3 Up;
    Vector3 Right;
    Vector3 WorldUp;
    // Euler Angles
    float Yaw;
    float Pitch;
    // Camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;


    // Constructor with vectors
    Camera(Vector3 position = Vector3(0.0f, 0.0f, 0.0f),
    		Vector3 up = Vector3(0.0f, 1.0f, 0.0f),
			float yaw = YAW,
			float pitch = PITCH) : Front(Vector3(0.0f, 0.0f, -1.0f)),
			MovementSpeed(SPEED),
			MouseSensitivity(SENSITIVITY),
			Zoom(ZOOM)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }


    // Constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
    :
    	Front(Vector3(0.0f, 0.0f, -1.0f)),
		MovementSpeed(SPEED),
		MouseSensitivity(SENSITIVITY),
		Zoom(ZOOM)
    {
        Position = Vector3(posX, posY, posZ);
        WorldUp = Vector3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // Halfes the view angle on zoom request
    void CameraZomm(int dozoom)    {
        if (this->Zoom >= 0.1f && Zoom <= 128.0f) {
        	if (dozoom == 1) {
        		this->Zoom /= 2;
        		if (this->Zoom < 0.1f)
        			this->Zoom = 0.1f;
        	}
        	else {
        		this->Zoom *= 2;
        		if (this->Zoom > 128.0f)
        			this->Zoom = 128.0f;
        	}

        }
    }


    // Calculates the front vector from camera Euler Angles (yaw,pitch)
    void updateCameraVectors() {
        // Calculate the new Front vector
        Vector3 front;
        front.X = cos(DEG_TO_RADIANS * Yaw) * cos(DEG_TO_RADIANS * Pitch);
        front.Y = sin(DEG_TO_RADIANS * Pitch);
        front.Z = sin(DEG_TO_RADIANS * Yaw) * cos(DEG_TO_RADIANS * Pitch);
        Front 	= front.normalized();
        float posX = DISTANCE * sin (Yaw * DEG_TO_RADIANS);
        float posY = DISTANCE * sin (Pitch * DEG_TO_RADIANS);
        float posZ = DISTANCE * cos (Yaw * DEG_TO_RADIANS);
        Up = Vector3(0.0f, 1.0f, 0.0f);
        Position = Vector3(posX,posY,posZ);
    }

    // Reset camera position and zoom
     void resetCamera(){
    	DISTANCE = 29.0f;
        Front 	= Vector3( DISTANCE, 10.0f, 0.0f);
        Right 	= Vector3(0.0f, 0.0f, 0.0f);
        Up 		= Vector3(0.0f, 1.0f, 0.0f);
        Zoom 	= ZOOM;
        Yaw 	= 0.0f;
        Pitch 	= 0.0f;
     }
};
