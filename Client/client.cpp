////////////////////////////////////////////////////////////////////////////////////////////////////
//Include header files
////////////////////////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <string>
#include <ctime>
#include <ratio>
#include <chrono>
#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <string>
#include <thread>

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Wizard.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/fl_ask.H>

#include <opencv2/opencv.hpp>
#include <opencv2/tracking/tracker.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//Defining namespaces
////////////////////////////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace cv;
using namespace std::chrono;

////////////////////////////////////////////////////////////////////////////////////////////////////
//Define colors for GUI
////////////////////////////////////////////////////////////////////////////////////////////////////
Scalar colorGreen = Scalar(179, 239, 194); //Green
Scalar colorBlue = Scalar(177, 171, 151); //Blue
Scalar colorPurple = Scalar(114, 111, 116); //Purple

////////////////////////////////////////////////////////////////////////////////////////////////////
//Global Variables & Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////
Point LButtonDown = Point(0, 0);
Point LButtonHold = Point(0, 0);
bool MouseClicked = false;
string ChosenTracker = "";

Fl_Window *firstWindow = 0;
Fl_Window *secondWindow = 0;

bool shutdownActivated = false;
bool rebootActivated = false;

bool escPressed = false;
bool yPressed = false;

////////////////////////////////////////////////////////////////////////////////////////////////////
//Defining Properties
////////////////////////////////////////////////////////////////////////////////////////////////////
#define SCREEN_X                        1920
#define SCREEN_Y                        1080

#define PORT                            4097
#define SHUTDOWN						3001
#define REBOOT							3002

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function which triggers when Mouse Movements or Clicks are detected (Used for selecting the Region of interest [ROI])
// @event	Defines for which event the function looks out for.
// @x		x-coordinate of a point
// @y		y-coordinate of a point
// @flags
////////////////////////////////////////////////////////////////////////////////////////////////////
void CallBackFunc(int event, int x, int y, int flags, void* userdata) {
  if (event == EVENT_LBUTTONDOWN && MouseClicked == false) {
    LButtonDown = Point(x, y);
    MouseClicked = true;
  }
  if (event == EVENT_MOUSEMOVE && MouseClicked == true) {
    LButtonHold = Point(x, y);
  }
  if (event == EVENT_LBUTTONUP) {
    MouseClicked = false;
  }
  if (event == EVENT_MOUSEMOVE && flags == EVENT_FLAG_SHIFTKEY) {
    int TempLButtonDownX = LButtonDown.x - x;
    int TempLButtonDownY = LButtonDown.y - y;
    int TempLButtonHoldX = LButtonHold.x - TempLButtonDownX;
    int TempLButtonHoldY = LButtonHold.y - TempLButtonDownY;

    LButtonDown = Point(x, y);
    LButtonHold = Point(TempLButtonHoldX, TempLButtonHoldY);
  }
  if (event == EVENT_MOUSEMOVE && flags == EVENT_FLAG_CTRLKEY) {
    int TempLButtonDownX = LButtonDown.x - x;
    int TempLButtonDownY = LButtonDown.y - y;
    int TempLButtonHoldX = LButtonHold.x + TempLButtonDownX;
    int TempLButtonHoldY = LButtonHold.y + TempLButtonDownY;

    LButtonDown = Point(x, y);
    LButtonHold = Point(TempLButtonHoldX, TempLButtonHoldY);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//Function for Drawing a smoothed out Rectangle (So Camera wont wiggle)
////////////////////////////////////////////////////////////////////////////////////////////////////
Point SmoothFrame (Mat frame, Point Point_1, Point Point_2, Point Point_3, Point Point_4, Point Point_5) {
  double tempX;
  double tempY;
  Point returnValue;

  //Calculate the weighted mean
  tempX = ((Point_1.x * 0.4) + (Point_2.x * 0.3) + (Point_3.x * 0.2) + (Point_4.x * 0.05) + (Point_5.x * 0.05));
  tempY = ((Point_1.y * 0.4) + (Point_2.y * 0.3) + (Point_3.y * 0.2) + (Point_4.y * 0.05) + (Point_5.y * 0.05));

  //Make Point to return
  returnValue = Point((tempX), (tempY));

  //Draw Rectangle (For Smooth Follow Visuals)
  rectangle(frame, Point((tempX - 50), (tempY - 50)), Point((tempX + 50), (tempY + 50)), colorGreen, 2, 8, 0);

  //Return Value
  return returnValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//Function for Calcualting a smooth follow
////////////////////////////////////////////////////////////////////////////////////////////////////
Point SmoothPrediction (Mat frame, Point Middle, Point Point_1, Point Point_2, Point Point_3, Point Point_4, Point Point_5) {
  double tempX;
  double tempY;
  Point returnValue;

  //Calculate the weighted mean
  tempX = (((Point_1.x - Point_2.x) * 0.4) + ((Point_2.x - Point_3.x) * 0.3) + ((Point_3.x - Point_4.x) * 0.2) + ((Point_4.x - Point_5.x) * 0.1)) / 4;
  tempY = (((Point_1.y - Point_2.y) * 0.4) + ((Point_2.y - Point_3.y) * 0.3) + ((Point_3.y - Point_4.y) * 0.2) + ((Point_4.y - Point_5.y) * 0.1)) / 4;

  //Create the Point where the prediction will be
  returnValue = Point(Point_1.x + tempX, Point_1.y + tempY);

  //Draw Arrowed Line (For SMooth Follow Visuals)
  arrowedLine(frame, Middle, returnValue, colorBlue, 2, 2, 0, 0.1);

  return returnValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//Function for handling the whole GUI
////////////////////////////////////////////////////////////////////////////////////////////////////
void GUISetting (Fl_Output*center_X, Fl_Output*center_Y, Fl_Output*framesPerSecond, Fl_Output*msecondsPerSecond, Fl_Output*ScreenSize_X, Fl_Output*ScreenSize_Y, Fl_Output*trackerDisplay, Point Middle, Point FrameInfo, double PictureTime, string Tracker, Point smoothedCenter, FL_Output*errorDisplay, bool trackerError) {
  int FPS = 1000/PictureTime;
  string errorString;

  center_X->value(to_string(smoothedCenter.x).c_str());
  center_Y->value(to_string(smoothedCenter.y).c_str());

  framesPerSecond->value(to_string(FPS).c_str());
  msecondsPerSecond->value(to_string(PictureTime).c_str());

  ScreenSize_X->value(to_string(FrameInfo.x).c_str());
  ScreenSize_Y->value(to_string(FrameInfo.y).c_str());
  
	if(trackerError) {
		errorString = "ERROR";
	} else {
		errorString = "SUCCESS";
	}
  
  errorDisplay->value(errorString.c_str());

  trackerDisplay->value(Tracker.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//Main function 2
////////////////////////////////////////////////////////////////////////////////////////////////////
int TrackerMain(Fl_Output*trackerInfo, Fl_Output*videoInfo, Fl_Output*center_X, Fl_Output*center_Y, Fl_Output*framesPerSecond, Fl_Output*msecondsPerSecond, Fl_Output*ScreenSize_X, Fl_Output*ScreenSize_Y, Fl_Output*trackerDisplay, const char* serverIP, FL_Output*errorDisplay) {

  Point prediction;
  Point smoothedCenter;

  int moveMotorX;
  int moveMotorY;
  int stepX;
  int stepY;
  int differnceToCenterX;
  int differnceToCenterY;

  bool running = true;
  bool runningMain = true;

  int positionArray[2] = {0, 0};

  duration<double, milli> time_span;
  high_resolution_clock::time_point beginTime = high_resolution_clock::now();
  high_resolution_clock::time_point endTime = high_resolution_clock::now();

  //Variables Initializing
  Point Point_1 = Point(1, 9); //Newest Frame
  Point Point_2 = Point(2, 8);
  Point Point_3 = Point(3, 7);
  Point Point_4 = Point(4, 6);
  Point Point_5 = Point(5, 5);
  Point Middle = Point(1,1);

  printf("\033c"); //clear console

  //
  // Networking Setup
  //
  int socket_fd;
  int serverPort;

  serverPort = PORT;

  struct sockaddr_in serverAddr;
  socklen_t addrLen = sizeof(struct sockaddr_in);

  //Initiate Socket
  if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    cout << "socket()\tERROR!" << endl;
  }

  //Specify socket propertirs (Port, IP Address)
  serverAddr.sin_family = PF_INET;
  serverAddr.sin_addr.s_addr = inet_addr(serverIP);
  serverAddr.sin_port = htons(serverPort);

  //Connect to socket
  if (connect(socket_fd, (sockaddr*)&serverAddr, addrLen) < 0) {
    cout << "connect()\tERROR!" << endl;
  }

  //
  //OpenCV Code
  //
  Mat img;
  img = Mat::zeros(300 , 480, CV_8UC3);
  int imgSize = img.total() * img.elemSize();
  uchar *iptr = img.data;

  Point FrameInfo = Point(480, 300);

  namedWindow("CamTrackAI", WINDOW_NORMAL);

  //set the callback function for any mouse event
  setMouseCallback("CamTrackAI", CallBackFunc, NULL);

  Rect2d trackingBox = Rect2d(Point(100,100), Point(0,0));
  int ExitWhile = 0;
  int my_id = 1234;
  int my_net_id = htonl(my_id);

  while (runningMain) {

    positionArray[0] = 0;
    positionArray[1] = 0;

    //Choose Tracker and Start It
    Ptr<Tracker> tracker;
    if (ChosenTracker == "KCF") {
      tracker = TrackerKCF::create();
    } else if (ChosenTracker == "CSRT") {
      tracker = TrackerCSRT::create();
    } else if (ChosenTracker == "TLD") {
      tracker = TrackerTLD::create();
    } else if (ChosenTracker == "MIL") {
      tracker = TrackerMIL::create();
    } else if (ChosenTracker == "MOSSE") {
      tracker = TrackerMOSSE::create();
    } else if (ChosenTracker == "Boosting") {
      tracker = TrackerBoosting::create();
    } else if (ChosenTracker == "MedianFlow") {
      tracker = TrackerMedianFlow::create();
    } else {
      cout << "Error chosing tracker" << endl;
      exit(0);
    }
	
	bool trackerError;
	yPressed == false;

    //Choose what object to track
    while (ExitWhile != 121 || yPressed == true) {
      recv(socket_fd, iptr, imgSize , MSG_WAITALL);

      send(socket_fd, positionArray , 8, 0);

      trackingBox = Rect2d(LButtonDown, LButtonHold);
      rectangle(img, trackingBox, colorPurple, 2, 8);

      imshow("CamTrackAI", img);

      ExitWhile = waitKey(25);
    }

    ExitWhile = 0;

    tracker->init(img, trackingBox);

    running = true;
	escPressed = false;

    while (running) {
      //Timestamp 1
      beginTime = high_resolution_clock::now();

      if (recv(socket_fd, iptr, imgSize , MSG_WAITALL) == -1) {
        cout << "recv()\tERROR!" << endl;

        positionArray[0] = 0;
        positionArray[1] = 0;

        send(socket_fd, positionArray , 8, 0);
        close(socket_fd);

        running = false;

        exit(0);
      }

      //Tracker searches for object in current frame. If it fails to find the object, this loop will send a 0 to the motors, indicating not to move.
      if (trackerError = tracker->update(img, trackingBox) == true) {
        rectangle(img, trackingBox, colorPurple, 2, 8);

        //Calucalte Position of Bounding Boxpthread_exit(NULL);
        Middle = Point((trackingBox.x + trackingBox.width / 2),(trackingBox.y + trackingBox.height / 2));

        //Update Points
        Point_5 = Point_4;
        Point_4 = Point_3;
        Point_3 = Point_2;
        Point_2 = Point_1;
        Point_1 = Middle;

        //Smooth out rectangle
        smoothedCenter = SmoothFrame(img, Point_1, Point_2, Point_3, Point_4, Point_5);

        Fl::check();


        imshow("CamTrackAI", img);


        //480/2 = 240
        stepX = 1023 / (FrameInfo.x/2);
        stepY = 1023 / (FrameInfo.y/2);

        differnceToCenterX = smoothedCenter.x - (FrameInfo.x/2);
        differnceToCenterY = smoothedCenter.y - (FrameInfo.y/2);

        //Checks if the difference from the center is + or minus 0. If < 0 send a number ranging from 0-1023 to the motor. If false, send numbers ranging from 1024-2047. Used to determine the direction.
        if(differnceToCenterX < 0) {
          moveMotorX = abs(differnceToCenterX) * stepX;
        } else {
          moveMotorX = (abs(differnceToCenterX) * stepX) + 1024;
        }

        if(differnceToCenterY < 0) {
          moveMotorY = abs(differnceToCenterY) * stepY;
        } else {
          moveMotorY = (abs(differnceToCenterY) * stepY) + 1024;
        }

        positionArray[0] = moveMotorX;
        positionArray[1] = moveMotorY;
      } else {
        positionArray[0] = 0;
        positionArray[1] = 0;
      }
	  
	  //Handles whole GUI
      GUISetting(center_X, center_Y, framesPerSecond, msecondsPerSecond, ScreenSize_X, ScreenSize_Y, trackerDisplay, Middle, FrameInfo, time_span.count(), ChosenTracker, smoothedCenter, errorDisplay, trackerError);

      if (shutdownActivated == true || rebootActivated == true) {
        if (shutdownActivated) {
          positionArray[0] = SHUTDOWN;
        } else {
          positionArray[0] = REBOOT;
        }
        send(socket_fd, positionArray , 8, 0);

        running = false;
        runningMain = false;
      } else {
        send(socket_fd, positionArray , 8, 0);
      }

      // Exit if ESC pressed.
      int k = waitKey(1);
      if(k == 27 || escPressed == true)
      {
        running = false;
      }
	  
      //Timestamp 2 and FPS Calculation
      endTime = high_resolution_clock::now();
      time_span = endTime - beginTime;
    }
  }
  close(socket_fd);

  exit(0);

  return 0;

}

////////////////////////////////////////////////////////////////////////////////////////////////////
//Buttons for GUI
////////////////////////////////////////////////////////////////////////////////////////////////////
void b_Exit(Fl_Widget *, void *) {
  fl_beep();
  exit(0);
}

void b_Next(Fl_Widget *, void *) {
  fl_beep();
  firstWindow->hide();
}

void b_KCF(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "KCF";
  firstWindow->hide();
}

void b_CSRT(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "CSRT";
  firstWindow->hide();
}

void b_TLD(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "TLD";
  firstWindow->hide();
}

void b_MIL(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "MIL";
  firstWindow->hide();
}

void b_MOSSE(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "MOSSE";
  firstWindow->hide();
}

void b_BOOSTING(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "Boosting";
  firstWindow->hide();
}

void b_MEDIANFLOW(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "MedianFlow";
  firstWindow->hide();
}

void b_UP(Fl_Widget *, void *) {
  fl_beep();
}

void b_DOWN(Fl_Widget *, void *) {
  fl_beep();
}

void b_LEFT(Fl_Widget *, void *) {
  fl_beep();
}

void b_RIGHT(Fl_Widget *, void *) {
  fl_beep();
}

void b_SHUTDOWN(Fl_Widget *, void *) {
  fl_beep();
  shutdownActivated = true;
}

void b_REBOOT(Fl_Widget *, void *) {
  rebootActivated = true;
  fl_beep();
}

void b_END(Fl_Widget *, void *) {
  escPressed = true;
  fl_beep();
}

void b_COMMIT(Fl_Widget *, void *) {
  yPressed = true;
  fl_beep();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//First Page. Here the user will choose which Tracker to use.
////////////////////////////////////////////////////////////////////////////////////////////////////
int FirstWindow(int argc, char **argv) {

  firstWindow = new Fl_Window(0,0,SCREEN_X/2,SCREEN_Y);
  firstWindow->resizable(firstWindow);

  Fl_Output *out1 = new Fl_Output(0, 1*(SCREEN_Y/10)-10, SCREEN_X/2, 1, "Chose a tracker");
  out1->labelsize(25);
  out1->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);

  Fl_Button *kcf = new Fl_Button(0, 1*(SCREEN_Y/10), SCREEN_X/15, SCREEN_Y/15, "KCF");
  kcf->callback(b_KCF,0);

  Fl_Button *csrt = new Fl_Button(1*(SCREEN_X/8), 1*(SCREEN_Y/10), SCREEN_X/15, SCREEN_Y/15, "CSRT");
  csrt->callback(b_CSRT,0);

  Fl_Button *tld = new Fl_Button(2*(SCREEN_X/8), 1*(SCREEN_Y/10), SCREEN_X/15, SCREEN_Y/15, "TLD");
  tld->callback(b_TLD,0);

  Fl_Button *mil = new Fl_Button(3*(SCREEN_X/8), 1*(SCREEN_Y/10), SCREEN_X/15, SCREEN_Y/15, "MIL");
  mil->callback(b_MIL,0);

  Fl_Button *boosting = new Fl_Button(0, 2*(SCREEN_Y/10), SCREEN_X/15, SCREEN_Y/15, "Boosting");
  boosting->callback(b_BOOSTING,0);

  Fl_Button *mosse = new Fl_Button(1*(SCREEN_X/8), 2*(SCREEN_Y/10), SCREEN_X/15, SCREEN_Y/15, "MOSSE");
  mosse->callback(b_MOSSE,0);

  Fl_Button *medianflow = new Fl_Button(2*(SCREEN_X/8), 2*(SCREEN_Y/10), SCREEN_X/15, SCREEN_Y/15, "Medianflow");
  medianflow->callback(b_MEDIANFLOW,0);

  Fl_Button *shutdown = new Fl_Button(0,SCREEN_Y-SCREEN_Y/15, SCREEN_X/15, SCREEN_Y/15, "Shut Down");
  shutdown->callback(b_SHUTDOWN,0);

  Fl_Button *reboot = new Fl_Button(SCREEN_X/15 + 20,SCREEN_Y-SCREEN_Y/15, SCREEN_X/15, SCREEN_Y/15, "Reboot");
  reboot->callback(b_REBOOT,0);

  firstWindow->end();
  firstWindow->show(argc,argv);

  firstWindow->redraw();

  return Fl::run();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//Second Page. This is the main Tracker HUD/GUI
////////////////////////////////////////////////////////////////////////////////////////////////////
int SecondWindow(int argc, char **argv, const char* serverIP) {

  //Create the window
  secondWindow = new Fl_Window(0,0,SCREEN_X/2,SCREEN_Y);
  secondWindow->resizable(secondWindow);

  //Titel "Tracker Information"
  Fl_Output *trackerInfo = new Fl_Output(0, 1*(SCREEN_Y/10)-10, SCREEN_X/2, 1, "Tracker Information");
  trackerInfo->labelsize(25);
  trackerInfo->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);

  //Titel "Video Information"
  Fl_Output *videoInfo = new Fl_Output(0, 3*(SCREEN_Y/10)-10, SCREEN_X/2, 1, "Video Information");
  videoInfo->labelsize(25);
  videoInfo->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);

  //Titel "More Information"
  Fl_Output *moreInfo = new Fl_Output(0, 6*(SCREEN_Y/10)-10, SCREEN_X/2, 1, "More Information");
  moreInfo->labelsize(25);
  moreInfo->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);

  Fl_Output *center_X = new Fl_Output(10, 1*(SCREEN_Y/10)+10, 150, 20, "Center (x) ");
  center_X->labelsize(15);
  center_X->align(FL_ALIGN_RIGHT);

  Fl_Output *center_Y = new Fl_Output(10, 1*(SCREEN_Y/10)+40, 150, 20, "Center (y) ");
  center_Y->labelsize(15);
  center_Y->align(FL_ALIGN_RIGHT);

  Fl_Output *framesPerSecond = new Fl_Output(10, 3*(SCREEN_Y/10)+10, 150, 20, "FPS ");
  framesPerSecond->labelsize(15);
  framesPerSecond->align(FL_ALIGN_RIGHT);

  Fl_Output *msecondsPerSecond = new Fl_Output(10, 3*(SCREEN_Y/10)+40, 150, 20, "Milliseconds per frame ");
  msecondsPerSecond->labelsize(15);
  msecondsPerSecond->align(FL_ALIGN_RIGHT);

  Fl_Output *ScreenSize_X = new Fl_Output(10, 3*(SCREEN_Y/10)+100, 150, 20, "px Screensize (x) ");
  ScreenSize_X->labelsize(15);
  ScreenSize_X->align(FL_ALIGN_RIGHT);

  Fl_Output *ScreenSize_Y = new Fl_Output(10, 3*(SCREEN_Y/10)+130, 150, 20, "px Screensize (y) ");
  ScreenSize_Y->labelsize(15);
  ScreenSize_Y->align(FL_ALIGN_RIGHT);

  Fl_Output *trackerDisplay = new Fl_Output(10, 3*(SCREEN_Y/10)+160, 150, 20, "Chosen Tracker ");
  trackerDisplay->labelsize(15);
  trackerDisplay->align(FL_ALIGN_RIGHT);

  Fl_Output *ipDisplay = new Fl_Output(10, 6*(SCREEN_Y/10)+10, 150, 20, "Connected (IP Address) ");
  ipDisplay->labelsize(15);
  ipDisplay->align(FL_ALIGN_RIGHT);
  ipDisplay->value(serverIP);
  
  Fl_Output *errorDisplay = new Fl_Output(10, 6*(SCREEN_Y/10)+40, 150, 20, "Tracker Status ");
  errorDisplay->labelsize(15);
  errorDisplay->align(FL_ALIGN_RIGHT);

  Fl_Button *shutdown = new Fl_Button(0,SCREEN_Y-SCREEN_Y/15, SCREEN_X/15, SCREEN_Y/15, "Quit");
  shutdown->callback(b_SHUTDOWN,0);

  Fl_Button *reboot = new Fl_Button(1*SCREEN_X/15,SCREEN_Y-SCREEN_Y/15, SCREEN_X/15, SCREEN_Y/15, "Restart");
  reboot->callback(b_REBOOT,0);
  
  Fl_Button *endTracker = new Fl_Button(2*SCREEN_X/15,SCREEN_Y-SCREEN_Y/15, SCREEN_X/15, SCREEN_Y/15, "End Tracker (ESC-Key)");
  endTracker->callback(b_END,0);
  
  Fl_Button *commitTracker = new Fl_Button(3*SCREEN_X/15,SCREEN_Y-SCREEN_Y/15, SCREEN_X/15, SCREEN_Y/15, "Start Tracker (Y-Key)");
  commitTracker->callback(b_COMMIT,0);

  secondWindow->end();
  secondWindow->show(argc,argv);

  thread threadObj(TrackerMain, trackerInfo, videoInfo, center_X, center_Y, framesPerSecond, msecondsPerSecond, ScreenSize_X, ScreenSize_Y, trackerDisplay, serverIP, errorDisplay);

  return Fl::run();

}

string GetStdoutFromCommand(string cmd) {

  string data;
  FILE * stream;

  const int max_buffer = 256;
  char buffer[max_buffer];

  stream = popen(cmd.c_str(), "r");

  if (stream) {
    while (!feof(stream)) {
      if (fgets(buffer, max_buffer, stream) != NULL) {
        data.append(buffer);
      }
    }
    pclose(stream);
  }
  return data;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//Main function
////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {

  string ls = GetStdoutFromCommand("avahi-resolve -n raspberrypi.local -4 | grep \"raspberrypi.local\" | cut -f2");

  const char *serverIP = ls.c_str();

  FirstWindow(argc,argv);

  SecondWindow(argc,argv, serverIP);

}
