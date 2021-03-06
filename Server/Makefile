#---------------------------------------------------------------------
# Application name
#---------------------------------------------------------------------
TARGET      = CamTrackAIServer

#---------------------------------------------------------------------
# Important directories used by assorted rules and other variables
#---------------------------------------------------------------------
DIR_OBJS   = objects

#---------------------------------------------------------------------
# Path to CamTrackAI folder
#---------------------------------------------------------------------
CAMTRACKAI_PATH = /home/pi/Desktop/CamTrackAI/CamTrackAI
#CAMTRACKAI_PATH = /home/theb3arybear/Desktop/CamTrackAI/GitHub/CamTrackAI

#---------------------------------------------------------------------
#Change Dynamixel Library to match your architecture
#---------------------------------------------------------------------
ARCHITECTURE_LIBRARY = dxl_sbc_cpp
#ARCHITECTURE_LIBRARY = dxl_x64_cpp

#---------------------------------------------------------------------
# Compiler options
#---------------------------------------------------------------------
CC          = gcc
CX          = g++
CCFLAGS     = -O2 -O3 -DLINUX -D_GNU_SOURCE -Wall $(INCLUDES) -g
CXFLAGS     = -O2 -O3 -DLINUX -D_GNU_SOURCE -Wall $(INCLUDES) -g
LNKCC       = $(CX)
LNKFLAGS    = $(CXFLAGS)

#---------------------------------------------------------------------
# Paths for compiler to include
#---------------------------------------------------------------------
INCLUDES   += -I/usr/local/include/opencv4
INCLUDES   += -I$(CAMTRACKAI_PATH)/DynamixelSDK/include/dynamixel_sdk

#---------------------------------------------------------------------
# Paths to libraries to include
#---------------------------------------------------------------------
#OpenCV Library
LIBRARIES  += -lopencv_gapi -lopencv_stitching -lopencv_aruco -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_dnn_objdetect -lopencv_dnn_superres -lopencv_dpm -lopencv_highgui -lopencv_face -lopencv_freetype -lopencv_fuzzy -lopencv_hfs -lopencv_img_hash -lopencv_line_descriptor -lopencv_quality -lopencv_reg -lopencv_rgbd -lopencv_saliency -lopencv_stereo -lopencv_structured_light -lopencv_phase_unwrapping -lopencv_superres -lopencv_optflow -lopencv_surface_matching -lopencv_tracking -lopencv_datasets -lopencv_text -lopencv_dnn -lopencv_plot -lopencv_videostab -lopencv_video -lopencv_videoio -lopencv_xfeatures2d -lopencv_shape -lopencv_ml -lopencv_ximgproc -lopencv_xobjdetect -lopencv_objdetect -lopencv_calib3d -lopencv_imgcodecs -lopencv_features2d -lopencv_flann -lopencv_xphoto -lopencv_photo -lopencv_imgproc -lopencv_core

#Dynamixel
LIBRARIES  += -l$(ARCHITECTURE_LIBRARY)

#Other
LIBRARIES  += -lrt -lpthread

#---------------------------------------------------------------------
# Source files
#---------------------------------------------------------------------
SOURCES = server.cpp \

OBJECTS  = $(addsuffix .o,$(addprefix $(DIR_OBJS)/,$(basename $(notdir $(SOURCES)))))

#---------------------------------------------------------------------
# Compiling rules
#---------------------------------------------------------------------
$(TARGET): make_directory $(OBJECTS)
	$(LNKCC) $(CFLAGS) $(LNKFLAGS) $(LDFLAGS) $(OBJECTS) -o $(TARGET) $(LIBRARIES)

all: $(TARGET)

clean:
	rm -rf $(TARGET) $(DIR_OBJS) core *~ *.a *.so *.lo

make_directory:
	mkdir -p $(DIR_OBJS)/

$(DIR_OBJS)/%.o: %.c
	$(CC) $(CCFLAGS) -c $? -o $@

$(DIR_OBJS)/%.o: %.cpp
	$(CX) $(CXFLAGS) -c $? -o $@
