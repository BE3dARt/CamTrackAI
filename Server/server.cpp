////////////////////////////////////////////////////////////////////////////////////////////////////
//Include header files
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "opencv2/opencv.hpp"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <string.h>

#include "dynamixel_sdk.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//Defining namespaces
////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace cv;
using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////
//Defining Properties
////////////////////////////////////////////////////////////////////////////////////////////////////
//Dynamixel
#define DXL_ID_1                        1
#define DXL_ID_2                        2

#define CW_ANGLE_LIMIT                  6
#define CCW_ANGLE_LIMIT                 8
#define MOVING_SPEED                    32
#define TORQUE                          24
#define TORQUE_LIMIT                    34

#define PROTOCOL_VERSION                1.0
#define BAUDRATE                        57600
#define DEVICENAME                      "/dev/ttyUSB0"

//OpenCV
#define VIDEOSOURCE											0

//Networking
#define PORT                            4097

////////////////////////////////////////////////////////////////////////////////////////////////////
//Main Function
////////////////////////////////////////////////////////////////////////////////////////////////////
void TCPServer () {
	//Inizializing variables and structs
	int returnValue;
	int localSocket;
	int remoteSocket;
	bool status = true;
	int positionArray[2];
	Mat img;
	Mat flippedFrame;
	struct sockaddr_in localAddr;
	struct sockaddr_in remoteAddr;
	int addrLen = sizeof(struct sockaddr_in);

	//Populating struct for socket
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = INADDR_ANY;
	localAddr.sin_port = htons(PORT);

	//Set video device
	VideoCapture cap(VIDEOSOURCE);

	//Setup/ prepare socket and check if it an error occurs.
	if ((localSocket = socket(AF_INET , SOCK_STREAM , 0)) < 0){
		cout << "socket()\tERROR!" << endl;
		return;
	}

	//Bind struct with socket and check if an error occurs.
	if (bind(localSocket,(struct sockaddr *)&localAddr , sizeof(localAddr)) < 0) {
		cout << "bind()\tERROR!" << endl;
		close(localSocket);
		return;
	}

	// Listening for an incoming connection
	listen(localSocket , 3);

	//Loop through function until a client connects to the socket
	while(status){
		if ((remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t*)&addrLen)) < 0) {
			cout << "accept()\tERROR" << endl;
			close(localSocket);
			close(remoteSocket);
			return;
		} else {
			status = false;
		}
	}

	//Check if the capturing devics is up
	if (cap.isOpened() == false) {
		cout << "Opening capturing device\tERROR!" << endl;
	}

	//Adjust the resolution of capture device
	cap.set(CAP_PROP_FRAME_WIDTH, 480);
	cap.set(CAP_PROP_FRAME_HEIGHT, 300);

	//Get resolution of capturing device
	int height = cap.get(CAP_PROP_FRAME_HEIGHT);
	int width = cap.get(CAP_PROP_FRAME_WIDTH);

	//Populate matrix with 0
	img = Mat::zeros(height, width, CV_8UC3);

	//Calucalte image size for sending over TCP_IP
	int imgSize = img.total() * img.elemSize();

	//Define port and protocol version for communicating with Dynamixel motors
	dynamixel::PortHandler *portHandler = dynamixel::PortHandler::getPortHandler(DEVICENAME);
	dynamixel::PacketHandler *packetHandler = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION);

	//Open port
	returnValue = portHandler->openPort();

	//Set port baudrate
	returnValue = portHandler->setBaudRate(BAUDRATE);

	//Enable torque
	returnValue = packetHandler->write1ByteTxOnly(portHandler, DXL_ID_1, TORQUE, 1);
	returnValue = packetHandler->write1ByteTxOnly(portHandler, DXL_ID_2, TORQUE, 1);

	//Set torque limit
	returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_1, TORQUE_LIMIT, 1023);
	returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_2, TORQUE_LIMIT, 1023);

	//CW angle limit (Used to set motors to wheel mode)
	returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_1, CW_ANGLE_LIMIT, 0);
	returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_2, CW_ANGLE_LIMIT, 0);

	//CCW angle limit (Used to set motors to wheel mode)
	returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_1, CCW_ANGLE_LIMIT, 0);
	returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_2, CCW_ANGLE_LIMIT, 0);

	status = true;
	while(status){
		// get a frame from the camera
		cap >> img;
		// flip the frame
		flip(img, flippedFrame, 1);

		// send the flipped frame over the network
		std::cout << "Server send" << std::endl;
		send(remoteSocket, flippedFrame.data, imgSize, 0);

		returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_1, MOVING_SPEED, positionArray[0]);

		if(recv(remoteSocket, positionArray, 8, 0) == -1) {
			std::cout << "FèèCK" << positionArray << std::endl;
		}

		positionArray[0] = positionArray[0];
		positionArray[1] = positionArray[1];

		std::cout << "Server receive: x:" << positionArray[0] << "y:" << positionArray[1] << std::endl;

		//Write bytes to Dynamixel Motors
		returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_2, MOVING_SPEED, positionArray[1]);
	}

	uint8_t receivedPackage;
	uint16_t receivedError;

	do {
		packetHandler->read2ByteRx(portHandler, receivedPackage, &receivedError);
		//Set Speed
		returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_1, MOVING_SPEED, 0);
		returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_2, MOVING_SPEED, 0);
	} while(receivedError != 0);

	// Close port
	portHandler->closePort();

	// close socket
	close(remoteSocket);

	return;
}

int main(int argc, char** argv)
{
	TCPServer();
}
