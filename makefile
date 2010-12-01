########################################################################
# Makefile for Vrui toolkit and required basic libraries.
# Copyright (c) 1998-2010 Oliver Kreylos
#
# This file is part of the WhyTools Build Environment.
# 
# The WhyTools Build Environment is free software; you can redistribute
# it and/or modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
# 
# The WhyTools Build Environment is distributed in the hope that it will
# be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with the WhyTools Build Environment; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA
########################################################################

########################################################################
# Please do not change the following lines
########################################################################

# Define the root of the toolkit source tree
VRUIPACKAGEROOT := $(shell pwd)

# Include definitions for the system environment
include $(VRUIPACKAGEROOT)/BuildRoot/SystemDefinitions
include $(VRUIPACKAGEROOT)/BuildRoot/Packages

########################################################################
# The root directory where Vrui will be installed when running
# "make install". This must be a different directory than the one that
# contains this makefile, i.e., Vrui cannot be built in-tree. If the
# installation directory is changed from the default, the makefiles of
# Vrui applications typically need to be changed to use the same
# installation directory.
########################################################################

INSTALLDIR = $(HOME)/Vrui-2.0

########################################################################
# Some settings that might need adjustment. In general, do not bother
# with these unless something goes wrong during the build process, or
# you have very specific requirements. Proceed with caution!
# The settings below are conservative; see the README file for what they
# mean, and how they depend on capabilities of the host system.
########################################################################

# Set the following three flags to 1 if the image handling library shall
# contain support for reading/writing PNG, JPEG, and TIFF images,
# respectively. This requires that libpng, libjpeg, and libtiff are
# installed on the host computer, and that the paths to their respective
# header files / libraries are set properly in
# $(VRUIPACKAGEROOT)/BuildRoot/Packages. For now, the following code
# tries to determine automatically whether PNG, JPEG, and/or TIFF are
# supported. This might or might not work.
ifneq ($(strip $(PNG_BASEDIR)),)
  SYSTEM_HAVE_LIBPNG = 1
else
  SYSTEM_HAVE_LIBPNG = 0
endif
ifneq ($(strip $(JPEG_BASEDIR)),)
  SYSTEM_HAVE_LIBJPEG = 1
else
  SYSTEM_HAVE_LIBJPEG = 0
endif
ifneq ($(strip $(TIFF_BASEDIR)),)
  SYSTEM_HAVE_LIBTIFF = 1
else
  SYSTEM_HAVE_LIBTIFF = 0
endif

# Set the following two flags to 1 if the low-level sound library shall
# contain support for ALSA sound devices and the SPEEX speech
# compression library, respectively. This requires that libasound and
# speex are installed on the host computer, and that the paths to their
# respective header files / libraries are set properly in
# $(VRUIPACKAGEROOT)/BuildRoot/Packages. For now, the following code
# tries to determine automatically whether ALSA and/or SPEEX are
# supported. This might or might not work.
ifneq ($(strip $(ALSA_BASEDIR)),)
  SYSTEM_HAVE_ALSA = 1
else
  SYSTEM_HAVE_ALSA = 0
endif
ifneq ($(strip $(SPEEX_BASEDIR)),)
  SYSTEM_HAVE_SPEEX = 1
else
  SYSTEM_HAVE_SPEEX = 0
endif

# Set the following three flags to 1 if the low-level video library
# shall contain support for Video4Linux2 and FireWire DC1394 video
# devices and the Theora video compression library, respectively. This
# requires that libv4l2, libdc1394, and theora are installed on the host
# computer, and that the paths to their respective header files /
# libraries are set properly in $(VRUIPACKAGEROOT)/BuildRoot/Packages.
# For now, the following code tries to determine automatically whether
# V4L2, DC1394, and/or Theora are supported. This might or might not
# work.
ifneq ($(strip $(V4L2_BASEDIR)),)
  SYSTEM_HAVE_V4L2 = 1
else
  SYSTEM_HAVE_V4L2 = 0
endif
ifneq ($(strip $(DC1394_BASEDIR)),)
  SYSTEM_HAVE_DC1394 = 1
else
  SYSTEM_HAVE_DC1394 = 0
endif
ifneq ($(strip $(THEORA_BASEDIR)),)
  SYSTEM_HAVE_THEORA = 1
else
  SYSTEM_HAVE_THEORA = 0
endif

# Set this to 1 if Vrui shall contain support for spatial audio using
# the OpenAL API. This requires that OpenAL is installed on the host
# computer, and that the path to the OpenAL header files / libraries is
# set properly in $(VRUIPACKAGEROOT)/BuildRoot/Packages.
# For now, the following code tries to automatically determine whether
# OpenAL is supported. This might or might not work.
ifeq ($(SYSTEM),DARWIN)
  SYSTEM_HAVE_OPENAL = 1
else
  ifneq ($(strip $(OPENAL_BASEDIR)),)
    SYSTEM_HAVE_OPENAL = 1
  else
    SYSTEM_HAVE_OPENAL = 0
  endif
endif

# Set this to 1 if Vrui executables and shared libraries shall contain
# links to any shared libraries they link to. This will relieve a user
# from having to set up a dynamic library search path.
USE_RPATH = 1

# Set the following flag to 1 if the GL support library shall contain
# support for multithreaded rendering to several windows. This is
# required for shared-memory multi-pipe rendering systems such as SGI
# Onyx or Prism, but somewhat reduces performance when accessing per-
# window data elements or OpenGL extensions. If the compiler and run-
# time environment do not support a direct mechanism for thread-local
# storage (such as gcc's __thread extension), this uses the even slower
# POSIX thread-local storage mechanism. If setting this to 1 causes
# compilation errors, the SYSTEM_HAVE_TLS variable in
# BuildRoot/SystemDefinitions needs to be set to 0.
GLSUPPORT_USE_TLS = 0

# Set this to 1 if the VRWindow class shall be compiled with support for
# swap locks and swap groups (NVidia extension). This is only necessary
# in very rare cases; if you don't already know you need it, leave this
# setting at 0.
# If this is set to 1 and the NVidia extension is not there,
# VRWindow.cpp will generate compiler errors.
VRUI_VRWINDOW_USE_SWAPGROUPS = 0

# Set this to 1 if the operating system supports the input abstraction
# layer. If this is set to 1 and the input abstraction is not supported,
# Joystick.cpp will generate compiler errors.
VRDEVICES_USE_INPUT_ABSTRACTION = 0

# Set this to 1 if the Linux input.h header file has the required
# structure definitions (usually on newer Linux versions). If this is
# set wrongly, HIDDevice.cpp will generate compiler errors.
# This setting is ignored on MacOS X.
VRDEVICES_INPUT_H_HAS_STRUCTS = 1

# Set this to 1 if the Vrui VR device driver shall support Nintendo Wii
# controllers using the bluez user-level Bluetooth library. This
# requires that bluez is installed on the host computer, and that the
# path to the bluez header files / libraries is set properly in
# $(VRUIPACKAGEROOT)/BuildRoot/Packages.
# For now, the following code tries to automatically determine whether
# bluez is supported. This might or might not work.
ifneq ($(strip $(BLUETOOTH_BASEDIR)),)
  VRDEVICES_USE_BLUETOOTH = 1
else
  VRDEVICES_USE_BLUETOOTH = 0
endif

########################################################################
# Please do not change anything below this line
########################################################################

# Specify version of created dynamic shared libraries
VRUI_VERSION = 2000003
MAJORLIBVERSION = 2
MINORLIBVERSION = 0
VRUI_NAME = Vrui-$(MAJORLIBVERSION).$(MINORLIBVERSION)

# Specify default optimization/debug level
ifdef DEBUG
  DEFAULTDEBUGLEVEL = 3
  DEFAULTOPTLEVEL = 0
else
  DEFAULTDEBUGLEVEL = 0
  DEFAULTOPTLEVEL = 3
endif

# Set destination directory for libraries
ifdef DEBUG
  LIBDESTDIR = $(VRUIPACKAGEROOT)/$(LIBEXT)/debug
else
  LIBDESTDIR = $(VRUIPACKAGEROOT)/$(LIBEXT)
endif

# Directories for installation components
ifdef SYSTEMINSTALL
  HEADERINSTALLDIR = $(INSTALLDIR)/usr/$(INCLUDEEXT)/$(VRUI_NAME)
  LIBINSTALLDIR = $(INSTALLDIR)/usr/$(LIBEXT)/$(VRUI_NAME)
  EXECUTABLEINSTALLDIR = $(INSTALLDIR)/usr/$(BINEXT)
  PLUGININSTALLDIR = $(INSTALLDIR)/usr/$(LIBEXT)/$(VRUI_NAME)
  ETCINSTALLDIR = $(INSTALLDIR)/etc/$(VRUI_NAME)
  SHAREINSTALLDIR = $(INSTALLDIR)/usr/share/$(VRUI_NAME)
  PKGCONFIGINSTALLDIR = $(INSTALLDIR)/usr/$(LIBEXT)/pkgconfig
  DOCINSTALLDIR = $(INSTALLDIR)/usr/share/doc/$(VRUI_NAME)
else
  HEADERINSTALLDIR = $(INSTALLDIR)/$(INCLUDEEXT)
  ifdef DEBUG
    LIBINSTALLDIR = $(INSTALLDIR)/$(LIBEXT)/debug
    EXECUTABLEINSTALLDIR = $(INSTALLDIR)/$(BINEXT)/debug
    PLUGININSTALLDIR = $(INSTALLDIR)/$(LIBEXT)/debug
  else
    LIBINSTALLDIR = $(INSTALLDIR)/$(LIBEXT)
    EXECUTABLEINSTALLDIR = $(INSTALLDIR)/$(BINEXT)
    PLUGININSTALLDIR = $(INSTALLDIR)/$(LIBEXT)
  endif
  ETCINSTALLDIR = $(INSTALLDIR)/etc
  SHAREINSTALLDIR = $(INSTALLDIR)/share
  PKGCONFIGINSTALLDIR = $(INSTALLDIR)/$(LIBEXT)/pkgconfig
  DOCINSTALLDIR = $(INSTALLDIR)/share/doc
endif

########################################################################
# Specify additional compiler and linker flags
########################################################################

# Set flags to distinguish between static and shared libraries
ifdef STATIC_LINK
  LIBRARYNAME = $(LIBDESTDIR)/$(1).$(LDEXT).a
  OBJDIREXT = Static
else
  CFLAGS += $(CDSOFLAGS)
  LIBRARYNAME=$(LIBDESTDIR)/$(call FULLDSONAME,$(1))
endif

########################################################################
# List packages used by this project
# (Supported packages can be found in ./BuildRoot/Packages)
########################################################################

PACKAGES = 

########################################################################
# Specify all final targets
########################################################################

LIBRARIES = 
PLUGINS = 
EXECUTABLES = 

#
# The basic libraries:
#

LIBRARY_NAMES = libMisc \
                libPlugins \
                libRealtime \
                libThreads \
                libComm \
                libMath \
                libGeometry \
                libGLWrappers \
                libGLSupport \
                libGLXSupport \
                libGLGeometry \
                libGLMotif \
                libImages \
                libSound \
                libVideo \
                libALSupport \
                libSceneGraph \
                libVrui

LIBRARIES += $(LIBRARY_NAMES:%=$(call LIBRARYNAME,%))

#
# The Vrui VR tool plug-in hierarchy:
#

VRTOOLS_SOURCES = $(wildcard Vrui/Tools/*.cpp)

VRTOOLSDIREXT = VRTools
VRTOOLSDIR= $(LIBDESTDIR)/$(VRTOOLSDIREXT)
VRTOOLS = $(VRTOOLS_SOURCES:Vrui/Tools/%.cpp=$(VRTOOLSDIR)/lib%.$(PLUGINFILEEXT))
PLUGINS += $(VRTOOLS)

#
# The Vrui vislet plug-in hierarchy:
#

VRVISLETS_SOURCES = $(wildcard Vrui/Vislets/*.cpp)

VRVISLETSDIREXT = VRVislets
VRVISLETSDIR = $(LIBDESTDIR)/$(VRVISLETSDIREXT)
VRVISLETS = $(VRVISLETS_SOURCES:Vrui/Vislets/%.cpp=$(VRVISLETSDIR)/lib%.$(PLUGINFILEEXT))
PLUGINS += $(VRVISLETS)

#
# The VR device driver daemon:
#

EXECUTABLES += $(EXEDIR)/VRDeviceDaemon

#
# The VR device driver plug-ins:
#

VRDEVICES_SOURCES = VRDeviceDaemon/VRDevices/AscensionFlockOfBirds.cpp \
                    VRDeviceDaemon/VRDevices/PolhemusFastrak.cpp \
                    VRDeviceDaemon/VRDevices/InterSense.cpp \
                    VRDeviceDaemon/VRDevices/FakespacePinchGlove.cpp \
                    VRDeviceDaemon/VRDevices/ArtDTrack.cpp \
                    VRDeviceDaemon/VRDevices/ViconTarsus.cpp \
                    VRDeviceDaemon/VRDevices/ViconTarsusRaw.cpp \
                    VRDeviceDaemon/VRDevices/PCTracker.cpp \
                    VRDeviceDaemon/VRDevices/PCWand.cpp \
                    VRDeviceDaemon/VRDevices/MouseButtons.cpp \
                    VRDeviceDaemon/VRDevices/SpaceBallRaw.cpp \
                    VRDeviceDaemon/VRDevices/SpaceBall.cpp \
                    VRDeviceDaemon/VRDevices/HIDDevice.cpp \
                    VRDeviceDaemon/VRDevices/VRPNClient.cpp \
                    VRDeviceDaemon/VRDevices/RemoteDevice.cpp \
                    VRDeviceDaemon/VRDevices/DummyDevice.cpp
ifneq ($(VRDEVICES_USE_INPUT_ABSTRACTION),0)
  VRDEVICES_SOURCES += VRDeviceDaemon/VRDevices/Joystick.cpp
endif
ifneq ($(VRDEVICES_USE_BLUETOOTH),0)
  VRDEVICES_SOURCES += VRDeviceDaemon/VRDevices/WiimoteTracker.cpp
endif

VRDEVICESDIREXT = VRDevices
VRDEVICESDIR = $(LIBDESTDIR)/$(VRDEVICESDIREXT)
VRDEVICES = $(VRDEVICES_SOURCES:VRDeviceDaemon/VRDevices/%.cpp=$(VRDEVICESDIR)/lib%.$(PLUGINFILEEXT))
PLUGINS += $(VRDEVICES)

#
# The VR tracker calibrator plug-ins:
#

VRCALIBRATORS_SOURCES = VRDeviceDaemon/VRCalibrators/TransformCalibrator.cpp \
                        VRDeviceDaemon/VRCalibrators/GridCalibrator.cpp

VRCALIBRATORSDIREXT = VRCalibrators
VRCALIBRATORSDIR = $(LIBDESTDIR)/$(VRCALIBRATORSDIREXT)
VRCALIBRATORS = $(VRCALIBRATORS_SOURCES:VRDeviceDaemon/VRCalibrators/%.cpp=$(VRCALIBRATORSDIR)/lib%.$(PLUGINFILEEXT))
PLUGINS += $(VRCALIBRATORS)

#
# The Vrui device driver test program:
#

EXECUTABLES += $(EXEDIR)/DeviceTest

#
# The input device data file dumping program:
#

EXECUTABLES += $(EXEDIR)/PrintInputDeviceDataFile

#
# The Vrui calibration utilities:
#

EXECUTABLES += $(EXEDIR)/XBackground \
               $(EXEDIR)/MeasureEnvironment \
               $(EXEDIR)/ScreenCalibrator \
               $(EXEDIR)/AlignTrackingMarkers

# Set the name of the makefile fragment:
ifdef DEBUG
  MAKEFILEFRAGMENT = Share/Vrui.debug.makeinclude
else
  MAKEFILEFRAGMENT = Share/Vrui.makeinclude
endif

# Set the name of the pkg-config meta data file:
PKGCONFIGFILE = Share/Vrui.pc

# Remember the names of all generated files for "make clean":
ALL = $(LIBRARIES) $(EXECUTABLES) $(PLUGINS) $(MAKEFILEFRAGMENT) $(PKGCONFIGFILE)

.PHONY: all
all: config $(ALL)

$(PLUGINS): $(LIBRARIES)
$(EXECUTABLES): $(LIBRARIES)

########################################################################
# Pseudo-target to print configuration options and configure libraries
########################################################################

.PHONY: config
config: Configure-End

.PHONY: Configure-Begin
Configure-Begin:
	@echo "---- Configured Vrui options: ----"
	@echo "Installation directory: $(INSTALLDIR)"
ifdef SYSTEMINSTALL
	@echo "System-level installation requested"
endif
ifneq ($(USE_RPATH),0)
	@echo "Run-time library search paths enabled"
else
	@echo "Run-time library search paths disabled"
endif

.PHONY: Configure-End
Configure-End: Configure-Realtime \
               Configure-GLSupport \
               Configure-Images \
               Configure-Sound \
               Configure-ALSupport \
               Configure-Video \
               Configure-Vrui \
               Configure-VRDeviceDaemon
	@echo "---- End of Vrui configuration options ----"

########################################################################
# Specify other actions to be performed on a `make clean'
########################################################################

.PHONY: extraclean
extraclean:
	-rm -f $(LIBRARY_NAMES:%=$(LIBDESTDIR)/$(call DSONAME,%))
	-rm -f $(LIBRARY_NAMES:%=$(LIBDESTDIR)/$(call LINKDSONAME,%))

.PHONY: extrasqueakyclean
extrasqueakyclean:
	-rm -f $(ALL)
	-rm -rf $(VRUIPACKAGEROOT)/$(LIBEXT)
	-rm -f Share/Vrui.makeinclude Share/Vrui.debug.makeinclude

# Include basic makefile
include $(VRUIPACKAGEROOT)/BuildRoot/BasicMakefile

########################################################################
# Specify build rules for dynamic shared objects
########################################################################

#
# Function to get the build dependencies of a package
#

DEPENDENCIES = $(patsubst -l%.$(LDEXT),$(call LIBRARYNAME,lib%),$(foreach PACKAGENAME,$(filter MY%,$($(1)_DEPENDS)),$($(PACKAGENAME)_LIBS)))

#
# The Miscellaneous Support Library (Misc)
#

MISC_HEADERS = $(wildcard Misc/*.h) \
               $(wildcard Misc/*.icpp)

MISC_SOURCES = $(wildcard Misc/*.cpp)

$(MISC_SOURCES): config

$(call LIBRARYNAME,libMisc): PACKAGES += $(MYMISC_DEPENDS)
$(call LIBRARYNAME,libMisc): EXTRACINCLUDEFLAGS += $(MYMISC_INCLUDE)
$(call LIBRARYNAME,libMisc): $(call DEPENDENCIES,MYMISC)
$(call LIBRARYNAME,libMisc): $(MISC_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: libMisc
libMisc: $(call LIBRARYNAME,libMisc)

#
# The Plugin Handling Library (Plugins):
#

PLUGINS_HEADERS = $(wildcard Plugins/*.h) \
                  $(wildcard Plugins/*.icpp)

PLUGINS_SOURCES = $(wildcard Plugins/*.cpp)

$(PLUGINS_SOURCES): config

$(call LIBRARYNAME,libPlugins): PACKAGES += $(MYPLUGINS_DEPENDS)
$(call LIBRARYNAME,libPlugins): EXTRACINCLUDEFLAGS += $(MYPLUGINS_INCLUDE)
$(call LIBRARYNAME,libPlugins): $(call DEPENDENCIES,MYPLUGINS)
$(call LIBRARYNAME,libPlugins): $(PLUGINS_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: libPlugins
libPlugins: $(call LIBRARYNAME,libPlugins)

#
# The Realtime Processing Library (Realtime):
#

.PHONY: Configure-Realtime
Configure-Realtime: Configure-Begin
ifneq ($(SYSTEM_HAVE_RT),0)
	@echo Realtime library uses timers
else
	@echo Realtime library uses wallclock time
endif
	@cp Realtime/Config.h Realtime/Config.h.temp
	@$(call CONFIG_SETVAR,Realtime/Config.h.temp,REALTIME_CONFIG_HAVE_POSIX_TIMERS,$(SYSTEM_HAVE_RT))
	@if ! diff Realtime/Config.h.temp Realtime/Config.h > /dev/null ; then cp Realtime/Config.h.temp Realtime/Config.h ; fi
	@rm Realtime/Config.h.temp
Realtime/Config.h: Configure-Realtime

REALTIME_HEADERS = $(wildcard Realtime/*.h) \
                   $(wildcard Realtime/*.icpp)

REALTIME_SOURCES = $(wildcard Realtime/*.cpp)

$(REALTIME_SOURCES): config

$(call LIBRARYNAME,libRealtime): PACKAGES += $(MYREALTIME_DEPENDS)
$(call LIBRARYNAME,libRealtime): EXTRACINCLUDEFLAGS += $(MYREALTIME_INCLUDE)
$(call LIBRARYNAME,libRealtime): $(call DEPENDENCIES,MYREALTIME)
$(call LIBRARYNAME,libRealtime): $(REALTIME_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: libRealtime
libRealtime: $(call LIBRARYNAME,libRealtime)

#
# The Portable Threading Library (Threads)
#

THREADS_HEADERS = $(wildcard Threads/*.h) \
                  $(wildcard Threads/*.icpp)

THREADS_SOURCES = $(wildcard Threads/*.cpp)

$(THREADS_SOURCES): config

$(call LIBRARYNAME,libThreads): PACKAGES += $(MYTHREADS_DEPENDS)
$(call LIBRARYNAME,libThreads): EXTRACINCLUDEFLAGS += $(MYTHREADS_INCLUDE)
$(call LIBRARYNAME,libThreads): $(call DEPENDENCIES,MYTHREADS)
$(call LIBRARYNAME,libThreads): $(THREADS_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: libThreads
libThreads: $(call LIBRARYNAME,libThreads)

#
# The Portable Communications Library (Comm)
#

COMM_HEADERS = $(wildcard Comm/*.h) \
               $(wildcard Comm/*.icpp)

COMM_SOURCES = $(wildcard Comm/*.cpp)

$(COMM_SOURCES): config

$(call LIBRARYNAME,libComm): PACKAGES += $(MYCOMM_DEPENDS)
$(call LIBRARYNAME,libComm): EXTRACINCLUDEFLAGS += $(MYCOMM_INCLUDE)
$(call LIBRARYNAME,libComm): $(call DEPENDENCIES,MYCOMM)
$(call LIBRARYNAME,libComm): $(COMM_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: libComm
libComm: $(call LIBRARYNAME,libComm)

#
# The Templatized Math Library (Math)
#

MATH_HEADERS = $(wildcard Math/*.h) \
               $(wildcard Math/*.icpp)

MATH_SOURCES = $(wildcard Math/*.cpp)

$(MATH_SOURCES): config

$(call LIBRARYNAME,libMath): PACKAGES += $(MYMATH_DEPENDS)
$(call LIBRARYNAME,libMath): EXTRACINCLUDEFLAGS += $(MYMATH_INCLUDE)
$(call LIBRARYNAME,libMath): $(call DEPENDENCIES,MYMATH)
$(call LIBRARYNAME,libMath): $(MATH_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: libMath
libMath: $(call LIBRARYNAME,libMath)

#
# The Templatized Geometry Library (Geometry)
#

GEOMETRY_HEADERS = $(wildcard Geometry/*.h) \
                   $(wildcard Geometry/*.icpp)

GEOMETRY_SOURCES = $(wildcard Geometry/*.cpp)

$(GEOMETRY_SOURCES): config

$(call LIBRARYNAME,libGeometry): PACKAGES += $(MYGEOMETRY_DEPENDS)
$(call LIBRARYNAME,libGeometry): EXTRACINCLUDEFLAGS += $(MYGEOMETRY_INCLUDE)
$(call LIBRARYNAME,libGeometry): $(call DEPENDENCIES,MYGEOMETRY)
$(call LIBRARYNAME,libGeometry): $(GEOMETRY_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: libGeometry
libGeometry: $(call LIBRARYNAME,libGeometry)

#
# The Mac OSX Support Library (MacOSX)
#

MACOSX_HEADERS = $(wildcard MacOSX/*.h) \
                 $(wildcard MacOSX/*.icpp)

#
# The OpenGL C++ Wrapper Library (GLWrappers)
#

GLWRAPPERS_HEADERS = GL/GLTexCoordTemplates.h \
                     GL/GLMultiTexCoordTemplates.h \
                     GL/GLColorTemplates.h \
                     GL/GLSecondaryColorTemplates.h \
                     GL/GLNormalTemplates.h \
                     GL/GLFogCoordTemplates.h \
                     GL/GLVertexTemplates.h \
                     GL/GLIndexTemplates.h \
                     GL/GLPrimitiveTemplates.h \
                     GL/GLScalarLimits.h \
                     GL/GLScalarConverter.h \
                     GL/GLColor.h \
                     GL/GLColorOperations.h \
                     GL/GLVector.h \
                     GL/GLBox.h \
                     GL/GLVertexArrayTemplates.h \
                     GL/GLVertexArrayParts.h \
                     GL/GLVertex.h GL/GLVertex.icpp \
                     GL/GLFogEnums.h \
                     GL/GLFogTemplates.h \
                     GL/GLFog.h \
                     GL/GLLightEnums.h \
                     GL/GLLightTemplates.h \
                     GL/GLLight.h \
                     GL/GLLightModelEnums.h \
                     GL/GLLightModelTemplates.h \
                     GL/GLMaterialEnums.h \
                     GL/GLMaterialTemplates.h \
                     GL/GLMaterial.h \
                     GL/GLTexEnvEnums.h \
                     GL/GLTexEnvTemplates.h \
                     GL/GLMatrixEnums.h \
                     GL/GLMatrixTemplates.h \
                     GL/GLMiscTemplates.h \
                     GL/GLGetTemplates.h \
                     GL/GLGetPrimitiveTemplates.h \
                     GL/GLGetFogTemplates.h \
                     GL/GLGetLightTemplates.h \
                     GL/GLGetMaterialTemplates.h \
                     GL/GLGetTexEnvTemplates.h \
                     GL/GLGetMatrixTemplates.h \
                     GL/GLGetMiscTemplates.h

GLWRAPPERS_SOURCES = GL/GLScalarLimits.cpp \
                     GL/GLColor.cpp \
                     GL/GLVertex.cpp \
                     GL/GLFog.cpp \
                     GL/GLLight.cpp \
                     GL/GLMaterial.cpp

$(GLWRAPPERS_SOURCES): config

$(call LIBRARYNAME,libGLWrappers): PACKAGES += $(MYGLWRAPPERS_DEPENDS)
$(call LIBRARYNAME,libGLWrappers): EXTRACINCLUDEFLAGS += $(MYGLWRAPPERS_INCLUDE)
$(call LIBRARYNAME,libGLWrappers): CFLAGS += $(MYGLWRAPPERS_CFLAGS)
$(call LIBRARYNAME,libGLWrappers): $(call DEPENDENCIES,MYGLWRAPPERS)
$(call LIBRARYNAME,libGLWrappers): $(GLWRAPPERS_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: libGLWrappers
libGLWrappers: $(call LIBRARYNAME,libGLWrappers)

#
# The OpenGL Support Library (GLSupport)
#

.PHONY: Configure-GLSupport
Configure-GLSupport: Configure-Begin
ifneq ($(GLSUPPORT_USE_TLS),0)
  ifneq ($(SYSTEM_HAVE_TLS),0)
	@echo "Multithreaded rendering enabled via TLS"
  else
	@echo "Multithreaded rendering enabled via pthreads"
  endif
else
	@echo "Multithreaded rendering disabled"
endif
	@cp GL/Config.h GL/Config.h.temp
	@$(call CONFIG_SETVAR,GL/Config.h.temp,GLSUPPORT_CONFIG_USE_TLS,$(GLSUPPORT_USE_TLS))
	@$(call CONFIG_SETVAR,GL/Config.h.temp,GLSUPPORT_CONFIG_HAVE_BUILTIN_TLS,$(SYSTEM_HAVE_TLS))
	@if ! diff GL/Config.h.temp GL/Config.h > /dev/null ; then cp GL/Config.h.temp GL/Config.h ; fi
	@rm GL/Config.h.temp
GL/Config.h: Configure-GLSupport

GLSUPPORT_HEADERS = GL/Config.h \
                    GL/GLPrintError.h \
                    GL/GLMarshallers.h GL/GLMarshallers.icpp \
                    GL/GLValueCoders.h \
                    GL/TLSHelper.h \
                    GL/GLObject.h \
                    GL/GLContextData.h \
                    GL/GLExtensions.h \
                    GL/GLExtensionManager.h \
                    GL/GLShader.h \
                    GL/GLGeometryShader.h \
                    GL/GLColorMap.h \
                    GL/GLNumberRenderer.h \
                    GL/GLFont.h \
                    GL/GLString.h \
                    GL/GLLabel.h \
                    GL/GLLineIlluminator.h \
                    GL/GLModels.h

GLSUPPORTEXTENSION_HEADERS = $(wildcard GL/Extensions/*.h) \
                             $(wildcard GL/Extensions/*.icpp)

GLSUPPORT_SOURCES = GL/GLPrintError.cpp \
                    GL/GLMarshallers.cpp \
                    GL/GLValueCoders.cpp \
                    GL/GLObject.cpp \
                    GL/Internal/GLThingManager.cpp \
                    GL/GLContextData.cpp \
                    GL/GLExtensions.cpp \
                    GL/GLExtensionManager.cpp \
		    $(wildcard GL/Extensions/*.cpp) \
                    GL/GLShader.cpp \
                    GL/GLGeometryShader.cpp \
                    GL/GLColorMap.cpp \
                    GL/GLNumberRenderer.cpp \
                    GL/GLFont.cpp \
                    GL/GLString.cpp \
                    GL/GLLabel.cpp \
                    GL/GLLineIlluminator.cpp \
                    GL/GLModels.cpp

ifneq ($(SYSTEM_HAVE_GLXGETPROCADDRESS), 0)
  $(OBJDIR)/GL/GLExtensionManager.o: CFLAGS += -DGLSUPPORT_HAVE_GLXGETPROCADDRESS
endif
$(OBJDIR)/GL/GLFont.o: CFLAGS += -DSYSGLFONTDIR='"$(SHAREINSTALLDIR)/GLFonts"'

$(GLSUPPORT_SOURCES): config

$(call LIBRARYNAME,libGLSupport): PACKAGES += $(MYGLSUPPORT_DEPENDS)
$(call LIBRARYNAME,libGLSupport): EXTRACINCLUDEFLAGS += $(MYGLSUPPORT_INCLUDE)
$(call LIBRARYNAME,libGLSupport): CFLAGS += $(MYGLSUPPORT_CFLAGS)
$(call LIBRARYNAME,libGLSupport): $(call DEPENDENCIES,MYGLSUPPORT)
$(call LIBRARYNAME,libGLSupport): $(GLSUPPORT_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: libGLSupport
libGLSupport: $(call LIBRARYNAME,libGLSupport)

#
# The OpenGL/GLX Support Library (GLXSupport)
#

GLXSUPPORT_HEADERS = GL/GLWindow.h

GLXSUPPORT_SOURCES = GL/GLWindow.cpp

$(GLXSUPPORT_SOURCES): config

$(call LIBRARYNAME,libGLXSupport): PACKAGES += $(MYGLXSUPPORT_DEPENDS)
$(call LIBRARYNAME,libGLXSupport): EXTRACINCLUDEFLAGS += $(MYGLXSUPPORT_INCLUDE)
$(call LIBRARYNAME,libGLXSupport): CFLAGS += $(MYGLXSUPPORT_CFLAGS)
$(call LIBRARYNAME,libGLXSupport): $(call DEPENDENCIES,MYGLXSUPPORT)
$(call LIBRARYNAME,libGLXSupport): $(GLXSUPPORT_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: libGLXSupport
libGLXSupport: $(call LIBRARYNAME,libGLXSupport)

#
# The OpenGL Wrapper Library for the Templatized Geometry Library (GLGeometry)
#

GLGEOMETRY_HEADERS = GL/GLGeometryWrappers.h \
                     GL/GLGeometryVertex.h GL/GLGeometryVertex.icpp \
                     GL/GLTransformationWrappers.h GL/GLTransformationWrappers.icpp \
                     GL/GLFrustum.h GL/GLFrustum.icpp \
                     GL/GLPolylineTube.h

GLGEOMETRY_SOURCES = GL/GLGeometryVertex.cpp \
                     GL/GLTransformationWrappers.cpp \
                     GL/GLFrustum.cpp \
                     GL/GLPolylineTube.cpp

$(GLGEOMETRY_SOURCES): config

$(call LIBRARYNAME,libGLGeometry): PACKAGES += $(MYGLGEOMETRY_DEPENDS)
$(call LIBRARYNAME,libGLGeometry): EXTRACINCLUDEFLAGS += $(MYGLGEOMETRY_INCLUDE)
$(call LIBRARYNAME,libGLGeometry): $(call DEPENDENCIES,MYGLGEOMETRY)
$(call LIBRARYNAME,libGLGeometry): $(GLGEOMETRY_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: libGLGeometry
libGLGeometry: $(call LIBRARYNAME,libGLGeometry)

#
# The GLMotif 3D User Interface Component Library (GLMotif)
#

GLMOTIF_HEADERS = $(wildcard GLMotif/*.h) \
                  $(wildcard GLMotif/*.icpp)

GLMOTIF_SOURCES = $(wildcard GLMotif/*.cpp)

$(GLMOTIF_SOURCES): config

$(call LIBRARYNAME,libGLMotif): PACKAGES += $(MYGLMOTIF_DEPENDS)
$(call LIBRARYNAME,libGLMotif): EXTRACINCLUDEFLAGS += $(MYGLMOTIF_INCLUDE)
$(call LIBRARYNAME,libGLMotif): $(call DEPENDENCIES,MYGLMOTIF)
$(call LIBRARYNAME,libGLMotif): $(GLMOTIF_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: libGLMotif
libGLMotif: $(call LIBRARYNAME,libGLMotif)

#
# The Image Handling Library (Images)
#

.PHONY: Configure-Images
Configure-Images: Configure-Begin
ifneq ($(SYSTEM_HAVE_LIBPNG),0)
	@echo "PNG image file format enabled"
else
	@echo "PNG image file format disabled"
endif
ifneq ($(SYSTEM_HAVE_LIBJPEG),0)
	@echo "JPG image file format enabled"
else
	@echo "JPG image file format disabled"
endif
ifneq ($(SYSTEM_HAVE_LIBTIFF),0)
	@echo "TIFF image file format enabled"
else
	@echo "TIFF image file format disabled"
endif
	@cp Images/Config.h Images/Config.h.temp
	@$(call CONFIG_SETVAR,Images/Config.h.temp,IMAGES_CONFIG_HAVE_PNG,$(SYSTEM_HAVE_LIBPNG))
	@$(call CONFIG_SETVAR,Images/Config.h.temp,IMAGES_CONFIG_HAVE_JPEG,$(SYSTEM_HAVE_LIBJPEG))
	@$(call CONFIG_SETVAR,Images/Config.h.temp,IMAGES_CONFIG_HAVE_TIFF,$(SYSTEM_HAVE_LIBTIFF))
	@if ! diff Images/Config.h.temp Images/Config.h > /dev/null ; then cp Images/Config.h.temp Images/Config.h ; fi
	@rm Images/Config.h.temp
Images/Config.h: Configure-Images

IMAGES_HEADERS = $(wildcard Images/*.h) \
                 $(wildcard Images/*.icpp)

IMAGES_SOURCES = $(wildcard Images/*.cpp)

$(IMAGES_SOURCES): config

$(call LIBRARYNAME,libImages): PACKAGES += $(MYIMAGES_DEPENDS)
$(call LIBRARYNAME,libImages): EXTRACINCLUDEFLAGS += $(MYIMAGES_INCLUDE)
$(call LIBRARYNAME,libImages): $(call DEPENDENCIES,MYIMAGES)
$(call LIBRARYNAME,libImages): $(IMAGES_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: libImages
libImages: $(call LIBRARYNAME,libImages)

#
# The basic sound library (Sound)
#

.PHONY: Configure-Sound
Configure-Sound: Configure-Begin
ifneq ($(SYSTEM_HAVE_ALSA),0)
	@echo "ALSA sound device support enabled"
else
	@echo "ALSA sound device support disabled"
endif
ifneq ($(SYSTEM_HAVE_SPEEX),0)
	@echo "SPEEX speech compression support enabled"
else
	@echo "SPEEX speech compression support disabled"
endif
	@cp Sound/Config.h Sound/Config.h.temp
	@$(call CONFIG_SETVAR,Sound/Config.h.temp,SOUND_CONFIG_HAVE_ALSA,$(SYSTEM_HAVE_ALSA))
	@$(call CONFIG_SETVAR,Sound/Config.h.temp,SOUND_CONFIG_HAVE_SPEEX,$(SYSTEM_HAVE_SPEEX))
	@if ! diff Sound/Config.h.temp Sound/Config.h > /dev/null ; then cp Sound/Config.h.temp Sound/Config.h ; fi
	@rm Sound/Config.h.temp
Sound/Config.h: Configure-Sound

SOUND_HEADERS = $(wildcard Sound/*.h) \
                $(wildcard Sound/*.icpp)
ifeq ($(SYSTEM),LINUX)
  SOUND_LINUX_HEADERS = 
  ifneq ($(SYSTEM_HAVE_ALSA),0)
    SOUND_LINUX_HEADERS += Sound/Linux/ALSAPCMDevice.h
  endif
  ifneq ($(SYSTEM_HAVE_SPEEX),0)
    SOUND_LINUX_HEADERS += Sound/Linux/SpeexEncoder.h \
                           Sound/Linux/SpeexDecoder.h
  endif
endif

SOUND_SOURCES = $(wildcard Sound/*.cpp)
ifeq ($(SYSTEM),LINUX)
  ifneq ($(SYSTEM_HAVE_ALSA),0)
    SOUND_SOURCES += Sound/Linux/ALSAPCMDevice.cpp
  endif
  ifneq ($(SYSTEM_HAVE_SPEEX),0)
    SOUND_SOURCES += Sound/Linux/SpeexEncoder.cpp \
                     Sound/Linux/SpeexDecoder.cpp
  endif
endif

$(SOUND_SOURCES): config

$(call LIBRARYNAME,libSound): PACKAGES += $(MYSOUND_DEPENDS)
$(call LIBRARYNAME,libSound): EXTRACINCLUDEFLAGS += $(MYSOUND_INCLUDE)
$(call LIBRARYNAME,libSound): $(call DEPENDENCIES,MYSOUND)
$(call LIBRARYNAME,libSound): $(SOUND_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: libSound
libSound: $(call LIBRARYNAME,libSound)

#
# The basic video library (Video)
#

.PHONY: Configure-Video
Configure-Video: Configure-Begin
ifneq ($(SYSTEM_HAVE_V4L2),0)
	@echo "Video4Linux2 video device support enabled"
else
	@echo "Video4Linux2 video device support disabled"
endif
ifneq ($(SYSTEM_HAVE_DC1394),0)
	@echo "FireWire DC1394 video device support enabled"
else
	@echo "FireWire DC1394 video device support disabled"
endif
ifneq ($(SYSTEM_HAVE_THEORA),0)
	@echo "Theora video codec support enabled"
else
	@echo "Theora video codec support disabled"
endif
	@cp Video/Config.h Video/Config.h.temp
	@$(call CONFIG_SETVAR,Video/Config.h.temp,VIDEO_CONFIG_HAVE_V4L2,$(SYSTEM_HAVE_V4L2))
	@$(call CONFIG_SETVAR,Video/Config.h.temp,VIDEO_CONFIG_HAVE_DC1394,$(SYSTEM_HAVE_DC1394))
	@$(call CONFIG_SETVAR,Video/Config.h.temp,VIDEO_CONFIG_HAVE_THEORA,$(SYSTEM_HAVE_THEORA))
	@if ! diff Video/Config.h.temp Video/Config.h > /dev/null ; then cp Video/Config.h.temp Video/Config.h ; fi
	@rm Video/Config.h.temp
Video/Config.h: Configure-Video

VIDEO_HEADERS = Video/Config.h \
                Video/VideoDataFormat.h \
                Video/FrameBuffer.h \
                Video/ImageExtractor.h \
                Video/VideoDevice.h \
                Video/Colorspaces.h \
                Video/ImageExtractorRGB8.h \
                Video/ImageExtractorYUYV.h \
                Video/ImageExtractorBA81.h \
                Video/YpCbCr420Texture.h \
                Video/VideoPane.h
ifneq ($(SYSTEM_HAVE_LIBJPEG),0)
  VIDEO_HEADERS += Video/ImageExtractorMJPG.h
endif
ifeq ($(SYSTEM),LINUX)
  VIDEO_LINUX_HEADERS = 
  ifneq ($(SYSTEM_HAVE_V4L2),0)
    VIDEO_LINUX_HEADERS += Video/Linux/V4L2VideoDevice.h
  endif
  ifneq ($(SYSTEM_HAVE_DC1394),0)
    VIDEO_LINUX_HEADERS += Video/Linux/DC1394VideoDevice.h
  endif
endif
ifneq ($(SYSTEM_HAVE_THEORA),0)
  VIDEO_HEADERS += Video/OggPage.h \
                   Video/OggSync.h \
                   Video/OggStream.h \
                   Video/TheoraInfo.h \
                   Video/TheoraComment.h \
                   Video/TheoraPacket.h \
                   Video/TheoraFrame.h \
                   Video/TheoraEncoder.h \
                   Video/TheoraDecoder.h
endif

VIDEO_SOURCES = Video/VideoDataFormat.cpp \
                Video/VideoDevice.cpp \
                Video/ImageExtractorRGB8.cpp \
                Video/ImageExtractorYUYV.cpp \
                Video/ImageExtractorBA81.cpp \
                Video/YpCbCr420Texture.cpp \
                Video/VideoPane.cpp
ifneq ($(SYSTEM_HAVE_LIBJPEG),0)
  VIDEO_SOURCES += Video/ImageExtractorMJPG.cpp
endif
ifneq ($(SYSTEM_HAVE_V4L2),0)
  VIDEO_SOURCES += Video/Linux/V4L2VideoDevice.cpp
endif
ifneq ($(SYSTEM_HAVE_DC1394),0)
  VIDEO_SOURCES += Video/Linux/DC1394VideoDevice.cpp
endif
ifneq ($(SYSTEM_HAVE_THEORA),0)
  VIDEO_SOURCES += Video/OggSync.cpp \
                   Video/OggStream.cpp \
                   Video/TheoraInfo.cpp \
                   Video/TheoraComment.cpp \
                   Video/TheoraPacket.cpp \
                   Video/TheoraFrame.cpp \
                   Video/TheoraEncoder.cpp \
                   Video/TheoraDecoder.cpp
endif

$(VIDEO_SOURCES): config

$(call LIBRARYNAME,libVideo): PACKAGES += $(MYVIDEO_DEPENDS)
$(call LIBRARYNAME,libVideo): EXTRACINCLUDEFLAGS += $(MYVIDEO_INCLUDE)
$(call LIBRARYNAME,libVideo): $(call DEPENDENCIES,MYVIDEO)
$(call LIBRARYNAME,libVideo): $(VIDEO_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: libVideo
libVideo: $(call LIBRARYNAME,libVideo)

#
# The OpenAL Support Library (ALSupport)
#

.PHONY: Configure-ALSupport
Configure-ALSupport: Configure-Begin
ifneq ($(SYSTEM_HAVE_OPENAL),0)
	@echo "Spatial sound using OpenAL enabled"
else
	@echo "Spatial sound using OpenAL disabled"
endif
	@cp AL/Config.h AL/Config.h.temp
	@$(call CONFIG_SETVAR,AL/Config.h.temp,ALSUPPORT_CONFIG_HAVE_OPENAL,$(SYSTEM_HAVE_OPENAL))
	@if ! diff AL/Config.h.temp AL/Config.h > /dev/null ; then cp AL/Config.h.temp AL/Config.h ; fi
	@rm AL/Config.h.temp
AL/Config.h: Configure-ALSupport

ALSUPPORT_HEADERS = $(wildcard AL/*.h) \
                    $(wildcard AL/*.icpp)

ALSUPPORT_SOURCES = $(wildcard AL/*.cpp) \
                    $(wildcard AL/Internal/*.cpp)

$(ALSUPPORT_SOURCES): config

$(call LIBRARYNAME,libALSupport): PACKAGES += $(MYALSUPPORT_DEPENDS)
$(call LIBRARYNAME,libALSupport): EXTRACINCLUDEFLAGS += $(MYALSUPPORT_INCLUDE)
$(call LIBRARYNAME,libALSupport): $(call DEPENDENCIES,MYALSUPPORT)
$(call LIBRARYNAME,libALSupport): $(ALSUPPORT_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: libALSupport
libALSupport: $(call LIBRARYNAME,libALSupport)

#
# The Scene Graph Rendering Library (SceneGraph)
#

SCENEGRAPH_HEADERS = $(wildcard SceneGraph/*.h) \
                     $(wildcard SceneGraph/*.icpp)

SCENEGRAPH_SOURCES = $(wildcard SceneGraph/*.cpp) \
                     $(wildcard SceneGraph/Internal/*.cpp)

$(OBJDIR)/SceneGraph/Internal/Doom3MaterialManager.o: CFLAGS += -DSCENEGRAPH_DOOM3MATERIALMANAGER_SHADERDIR='"$(SHAREINSTALLDIR)/Shaders/SceneGraph"'

$(SCENEGRAPH_SOURCES): config

$(call LIBRARYNAME,libSceneGraph): PACKAGES += $(MYSCENEGRAPH_DEPENDS)
$(call LIBRARYNAME,libSceneGraph): EXTRACINCLUDEFLAGS += $(MYSCENEGRAPH_INCLUDE)
$(call LIBRARYNAME,libSceneGraph): $(call DEPENDENCIES,MYSCENEGRAPH)
$(call LIBRARYNAME,libSceneGraph): $(SCENEGRAPH_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: libSceneGraph
libSceneGraph: $(call LIBRARYNAME,libSceneGraph)

#
# The Vrui Virtual Reality User Interface Library (Vrui)
#

.PHONY: Configure-Vrui
Configure-Vrui: Configure-Begin
ifneq ($(VRUI_VRWINDOW_USE_SWAPGROUPS),0)
	@echo "Swapgroup support for Vrui windows enabled"
else
	@echo "Swapgroup support for Vrui windows disabled"
endif
ifneq ($(SYSTEM_HAVE_LIBPNG),0)
	@echo "Vrui will save screenshots in PNG format"
else
	@echo "Vrui will save screenshots in PPM format"
endif
ifneq ($(SYSTEM_HAVE_OPENAL),0)
	@echo "Support for spatial audio enabled"
else
	@echo "Support for spatial audio disabled"
endif
ifneq ($(SYSTEM_HAVE_THEORA),0)
	@echo "Support to save screen captures in Ogg/Theora format enabled"
else
	@echo "Support to save screen captures in Ogg/Theora format disabled"
endif

VRUI_HEADERS = $(wildcard Vrui/*.h) \
               $(wildcard Vrui/*.icpp)

VRUI_SOURCES = $(wildcard Vrui/*.cpp) \
               $(filter-out Vrui/Internal/TheoraMovieSaver.cpp,$(wildcard Vrui/Internal/*.cpp)) \
               $(wildcard Vrui/Internal/$(OSSPECFILEINSERT)/*.cpp)
ifneq ($(SYSTEM_HAVE_THEORA),0)
  VRUI_SOURCES += Vrui/Internal/TheoraMovieSaver.cpp
endif

$(OBJDIR)/Vrui/Internal/InputDeviceAdapterMouse.o: CFLAGS += -DDEFAULTMOUSECURSORIMAGEFILENAME='"$(SHAREINSTALLDIR)/Textures/Cursor.Xcur"'
ifneq ($(HIDDEVICE_CPP_INPUT_H_HAS_STRUCTS),0)
  $(OBJDIR)/Vrui/Internal/Linux/InputDeviceAdapterHID.o: CFLAGS += -DINPUT_H_HAS_STRUCTS
endif
$(OBJDIR)/Vrui/Internal/InputDeviceAdapterPlayback.o: CFLAGS += -DDEFAULTMOUSECURSORIMAGEFILENAME='"$(SHAREINSTALLDIR)/Textures/Cursor.Xcur"'
$(OBJDIR)/Vrui/ToolManager.o: CFLAGS += -DSYSTOOLDSONAMETEMPLATE='"$(PLUGININSTALLDIR)/$(VRTOOLSDIREXT)/lib%s.$(PLUGINFILEEXT)"'
$(OBJDIR)/Vrui/VisletManager.o: CFLAGS += -DSYSVISLETDSONAMETEMPLATE='"$(PLUGININSTALLDIR)/$(VRVISLETSDIREXT)/lib%s.$(PLUGINFILEEXT)"'
$(OBJDIR)/Vrui/VRWindow.o: CFLAGS += -DAUTOSTEREODIRECTORY='"$(SHAREINSTALLDIR)/Textures"'
ifneq ($(VRUI_VRWINDOW_USE_SWAPGROUPS),0)
  $(OBJDIR)/Vrui/VRWindow.o: CFLAGS += -DVRWINDOW_USE_SWAPGROUPS
endif
$(OBJDIR)/Vrui/Internal/Vrui.Workbench.o: CFLAGS += -DSYSVRUICONFIGFILE='"$(ETCINSTALLDIR)/Vrui.cfg"' -DVRUIDEFAULTROOTSECTIONNAME='"Desktop"'

$(VRUI_SOURCES): config

$(call LIBRARYNAME,libVrui): PACKAGES += $(MYVRUI_DEPENDS)
$(call LIBRARYNAME,libVrui): EXTRACINCLUDEFLAGS += $(MYVRUI_INCLUDE)
$(call LIBRARYNAME,libVrui): $(call DEPENDENCIES,MYVRUI)
$(call LIBRARYNAME,libVrui): $(VRUI_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: libVrui
libVrui: $(call LIBRARYNAME,libVrui)

#
# The Vrui VR tool plug-in hierarchy:
#

# Implicit rule for creating VR tool plug-ins:
$(VRTOOLSDIR)/lib%.$(PLUGINFILEEXT): PACKAGES += MYVRUI
$(VRTOOLSDIR)/lib%.$(PLUGINFILEEXT): CFLAGS += $(CPLUGINFLAGS)
$(VRTOOLSDIR)/lib%.$(PLUGINFILEEXT): PLUGINDEPENDENCIES = 
ifneq ($(SYSTEM_HAVE_TRANSITIVE_PLUGINS),0)
  $(VRTOOLSDIR)/lib%.$(PLUGINFILEEXT): PLUGINLINKFLAGS += -L$(VRTOOLSDIR) $(DEPENDENCIES:$(VRTOOLSDIR)/lib%.$(PLUGINFILEEXT)=-l%)
  ifneq ($(SYSTEM_HAVE_RPATH),0)
    ifneq ($(USE_RPATH),0)
      $(VRTOOLSDIR)/lib%.$(PLUGINFILEEXT): PLUGINLINKFLAGS += -Wl,-rpath=$(PLUGININSTALLDIR)/$(VRTOOLSDIREXT)
    endif
  endif
  $(VRTOOLSDIR)/lib%.$(PLUGINFILEEXT): PLUGINDEPENDENCIES += $(LINKDIRFLAGS) $(LINKLIBFLAGS)
endif
$(VRTOOLSDIR)/lib%.$(PLUGINFILEEXT): $(OBJDIR)/Vrui/Tools/%.o
	@mkdir -p $(VRTOOLSDIR)
ifdef SHOWCOMMAND
	$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^ $(PLUGINDEPENDENCIES)
else
	@echo Linking $@...
	@$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^ $(PLUGINDEPENDENCIES)
endif

# Vrui tool settings:
$(OBJDIR)/Vrui/Tools/JediTool.o: CFLAGS += -DDEFAULTLIGHTSABERIMAGEFILENAME='"$(SHAREINSTALLDIR)/Textures/Lightsaber.png"'

$(VRTOOLS_SOURCES): config

# Mark all VR tool object files as intermediate:
.SECONDARY: $(VRTOOLS_SOURCES:%.cpp=$(OBJDIR)/%.o)

.PHONY: VRTools
VRTools: $(VRTOOLS)

#
# The Vrui vislet plug-in hierarchy:
#

# Implicit rule for creating vislet plug-ins:
$(VRVISLETSDIR)/lib%.$(PLUGINFILEEXT): PACKAGES += MYVRUI
$(VRVISLETSDIR)/lib%.$(PLUGINFILEEXT): CFLAGS += $(CPLUGINFLAGS)
$(VRVISLETSDIR)/lib%.$(PLUGINFILEEXT): PLUGINDEPENDENCIES = 
ifneq ($(SYSTEM_HAVE_TRANSITIVE_PLUGINS),0)
  $(VRVISLETSDIR)/lib%.$(PLUGINFILEEXT): PLUGINLINKFLAGS += -L$(VRVISLETSDIR) $(DEPENDENCIES:$(VRVISLETSDIR)/lib%.$(PLUGINFILEEXT)=-l%)
  ifneq ($(SYSTEM_HAVE_RPATH),0)
    ifneq ($(USE_RPATH),0)
      $(VRVISLETSDIR)/lib%.$(PLUGINFILEEXT): PLUGINLINKFLAGS += -Wl,-rpath=$(PLUGININSTALLDIR)/$(VRVISLETSDIREXT)
    endif
  endif
  $(VRVISLETSDIR)/lib%.$(PLUGINFILEEXT): PLUGINDEPENDENCIES += $(LINKDIRFLAGS) $(LINKLIBFLAGS)
endif
$(VRVISLETSDIR)/lib%.$(PLUGINFILEEXT): $(OBJDIR)/Vrui/Vislets/%.o
	@mkdir -p $(VRVISLETSDIR)
ifdef SHOWCOMMAND
	$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^ $(PLUGINDEPENDENCIES)
else
	@echo Linking $@...
	@$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^ $(PLUGINDEPENDENCIES)
endif

# Dependencies between Vrui vislets:
$(VRVISLETSDIR)/libSceneGraphViewer.$(PLUGINFILEEXT): PACKAGES += MYSCENEGRAPH

$(VRVISLETS_SOURCES): config

# Mark all vislet object files as intermediate:
.SECONDARY: $(VRVISLETS_SOURCES:%.cpp=$(OBJDIR)/%.o)

.PHONY: VRVislets
VRVislets: $(VRVISLETS)

#
# The VR device driver daemon:
#

.PHONY: Configure-VRDeviceDaemon
Configure-VRDeviceDaemon: Configure-Begin
ifneq ($(VRDEVICES_USE_INPUT_ABSTRACTION),0)
	@echo "Event device support (for joysticks and USB devices) enabled"
  ifneq ($(VRDEVICES_INPUT_H_HAS_STRUCTS),0)
	@echo "input.h contains event structure definitions"
  else
	@echo "input.h does not contain event structure definitions"
  endif
else
	@echo "Event device support (for joysticks and USB devices) disabled"
endif
ifneq ($(VRDEVICES_USE_BLUETOOTH),0)
	@echo "Bluetooth support (for Nintendo Wii controller) enabled"
else
	@echo "Bluetooth support (for Nintendo Wii controller) disabled"
endif

VRDEVICEDAEMON_SOURCES = VRDeviceDaemon/VRDevice.cpp \
                         VRDeviceDaemon/VRCalibrator.cpp \
                         VRDeviceDaemon/VRDeviceManager.cpp \
                         Vrui/Internal/VRDevicePipe.cpp \
                         VRDeviceDaemon/VRDeviceServer.cpp \
                         VRDeviceDaemon/VRDeviceDaemon.cpp

$(OBJDIR)/VRDeviceDaemon/VRDeviceManager.o: CFLAGS += -DSYSVRDEVICEDIRECTORY='"$(PLUGININSTALLDIR)/$(VRDEVICESDIREXT)"' \
                                                      -DSYSVRCALIBRATORDIRECTORY='"$(PLUGININSTALLDIR)/$(VRCALIBRATORSDIREXT)"'
$(OBJDIR)/VRDeviceDaemon/VRDeviceDaemon.o: CFLAGS += -DSYSVRDEVICEDAEMONCONFIGFILENAME='"$(ETCINSTALLDIR)/VRDevices.cfg"'

$(VRDEVICEDAEMON_SOURCES): config

$(EXEDIR)/VRDeviceDaemon: PACKAGES += MYGEOMETRY MYCOMM MYTHREADS MYMISC DL
$(EXEDIR)/VRDeviceDaemon: EXTRACINCLUDEFLAGS += $(MYVRUI_INCLUDE)
$(EXEDIR)/VRDeviceDaemon: CFLAGS += -DVERBOSE -DSYSDSONAMETEMPLATE='"lib%s.$(PLUGINFILEEXT)"'
$(EXEDIR)/VRDeviceDaemon: LINKFLAGS += $(PLUGINHOSTLINKFLAGS)
$(EXEDIR)/VRDeviceDaemon: $(call LIBRARYNAME,libGeometry) $(call LIBRARYNAME,libComm) $(call LIBRARYNAME,libMisc)
$(EXEDIR)/VRDeviceDaemon: $(VRDEVICEDAEMON_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: VRDeviceDaemon
VRDeviceDaemon: $(EXEDIR)/VRDeviceDaemon

#
# The VR device driver plug-ins:
#

ifneq ($(VRDEVICES_INPUT_H_HAS_STRUCTS),0)
  $(OBJDIR)/VRDeviceDaemon/VRDevices/HIDDevice.o: CFLAGS += -DINPUT_H_HAS_STRUCTS
endif

ifeq ($(SYSTEM),DARWIN)
  $(VRDEVICESDIR)/libHIDDevice.$(PLUGINFILEEXT): PLUGINLINKFLAGS += -framework System -framework IOKit -framework CoreFoundation
endif

$(VRDEVICESDIR)/libVRPNClient.$(PLUGINFILEEXT): $(OBJDIR)/VRDeviceDaemon/VRDevices/VRPNConnection.o \
                                                $(OBJDIR)/VRDeviceDaemon/VRDevices/VRPNClient.o

$(VRDEVICESDIR)/libWiimoteTracker.$(PLUGINFILEEXT): PLUGINLINKFLAGS += $(BLUETOOTH_LIBDIR) $(BLUETOOTH_LIBS)
$(VRDEVICESDIR)/libWiimoteTracker.$(PLUGINFILEEXT): $(OBJDIR)/VRDeviceDaemon/VRDevices/Wiimote.o \
                                                    $(OBJDIR)/VRDeviceDaemon/VRDevices/WiimoteTracker.o

# Implicit rule for creating plugins:
$(VRDEVICESDIR)/lib%.$(PLUGINFILEEXT): PACKAGES += MYGEOMETRY MYCOMM MYTHREADS MYMISC
$(VRDEVICESDIR)/lib%.$(PLUGINFILEEXT): EXTRACINCLUDEFLAGS += $(MYVRUI_INCLUDE)
$(VRDEVICESDIR)/lib%.$(PLUGINFILEEXT): CFLAGS += $(CPLUGINFLAGS) -DVERBOSE -DSYSDSONAMETEMPLATE='"lib%s.$(PLUGINFILEEXT)"'
$(VRDEVICESDIR)/lib%.$(PLUGINFILEEXT): $(OBJDIR)/VRDeviceDaemon/VRDevices/%.o
	@mkdir -p $(VRDEVICESDIR)
ifdef SHOWCOMMAND
	$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^
else
	@echo Linking $@...
	@$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^
endif

$(VRDEVICES_SOURCES): config

# Mark all VR device driver object files as intermediate:
.SECONDARY: $(VRDEVICES_SOURCES:%.cpp=$(OBJDIR)/%.o)

#
# The VR tracker calibrator plug-ins:
#

# Implicit rule for creating plugins:
$(VRCALIBRATORSDIR)/lib%.$(PLUGINFILEEXT): PACKAGES += MYGEOMETRY MYCOMM MYTHREADS MYMISC
$(VRCALIBRATORSDIR)/lib%.$(PLUGINFILEEXT): EXTRACINCLUDEFLAGS += $(MYVRUI_INCLUDE)
$(VRCALIBRATORSDIR)/lib%.$(PLUGINFILEEXT): CFLAGS += $(CPLUGINFLAGS) -DSYSDSONAMETEMPLATE='"lib%s.$(PLUGINFILEEXT)"'
$(VRCALIBRATORSDIR)/lib%.$(PLUGINFILEEXT): $(OBJDIR)/VRDeviceDaemon/VRCalibrators/%.o
	@mkdir -p $(VRCALIBRATORSDIR)
ifdef SHOWCOMMAND
	$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^
else
	@echo Linking $@...
	@$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^
endif

$(VRCALIBRATORS_SOURCES): config

# Mark all VR calibrator object files as intermediate:
.SECONDARY: $(VRCALIBRATORS_SOURCES:%.cpp=$(OBJDIR)/%.o)

.PHONY: VRCalibrators
VRCalibrators: $(VRCALIBRATORS)

#
# The VR Device Daemon Test program:
#

Vrui/Utilities/DeviceTest.cpp: config

$(EXEDIR)/DeviceTest: PACKAGES += MYVRUI
$(EXEDIR)/DeviceTest: $(OBJDIR)/Vrui/Utilities/DeviceTest.o
.PHONY: DeviceTest
DeviceTest: $(EXEDIR)/DeviceTest

#
# The Vrui input device data file printer:
#

Vrui/Utilities/PrintInputDeviceDataFile.cpp: config

$(EXEDIR)/PrintInputDeviceDataFile: PACKAGES += MYVRUI
$(EXEDIR)/PrintInputDeviceDataFile: $(OBJDIR)/Vrui/Utilities/PrintInputDeviceDataFile.o
.PHONY: PrintInputDeviceDataFile
PrintInputDeviceDataFile: $(EXEDIR)/PrintInputDeviceDataFile

#
# The calibration pattern generator:
#

Calibration/XBackground.cpp: config

$(EXEDIR)/XBackground: PACKAGES += X11
$(EXEDIR)/XBackground: EXTRACINCLUDEFLAGS += -ICalibration
$(EXEDIR)/XBackground: $(OBJDIR)/Calibration/XBackground.o
.PHONY: XBackground
XBackground: $(EXEDIR)/XBackground

#
# The measurement utility:
#

MEASUREENVIRONMENT_SOURCES = Calibration/TotalStation.cpp \
                             Calibration/NaturalPointClient.cpp \
                             Calibration/MeasureEnvironment.cpp

$(MEASUREENVIRONMENT_SOURCES): config

$(EXEDIR)/MeasureEnvironment: PACKAGES += MYVRUI
$(EXEDIR)/MeasureEnvironment: EXTRACINCLUDEFLAGS += -ICalibration
$(EXEDIR)/MeasureEnvironment: $(MEASUREENVIRONMENT_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: MeasureEnvironment
MeasureEnvironment: $(EXEDIR)/MeasureEnvironment

#
# The calibration transformation calculator:
#

Calibration/ScreenCalibrator.cpp: config

$(EXEDIR)/ScreenCalibrator: PACKAGES += MYVRUI
$(EXEDIR)/ScreenCalibrator: EXTRACINCLUDEFLAGS += -ICalibration
$(EXEDIR)/ScreenCalibrator: $(OBJDIR)/Calibration/ScreenCalibrator.o
.PHONY: ScreenCalibrator
ScreenCalibrator: $(EXEDIR)/ScreenCalibrator

#
# The rigid body transformation calculator:
#

ALIGNTRACKINGMARKERS_SOURCES = Calibration/ReadOptiTrackMarkerFile.cpp \
                               Calibration/NaturalPointClient.cpp \
                               Calibration/AlignTrackingMarkers.cpp

$(ALIGNTRACKINGMARKERS_SOURCES): config

$(EXEDIR)/AlignTrackingMarkers: PACKAGES += MYVRUI
$(EXEDIR)/AlignTrackingMarkers: EXTRACINCLUDEFLAGS += -ICalibration
$(EXEDIR)/AlignTrackingMarkers: $(ALIGNTRACKINGMARKERS_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: AlignTrackingMarkers
AlignTrackingMarkers: $(EXEDIR)/AlignTrackingMarkers

########################################################################
# Specify installation rules for header files, libraries, executables,
# configuration files, and shared files.
########################################################################

SYSTEMPACKAGES = $(sort $(patsubst MY%,,$(PACKAGES_RECEXPAND)))
VRUIAPP_INCLUDEDIRS = -I$$(VRUI_INCLUDEDIR)
VRUIAPP_INCLUDEDIRS += $(sort $(foreach PACKAGENAME,$(SYSTEMPACKAGES),$($(PACKAGENAME)_INCLUDE)))
VRUIAPP_CFLAGS = $(CSYSFLAGS)
VRUIAPP_CFLAGS += $(sort $(foreach PACKAGENAME,$(PACKAGES_RECEXPAND),$($(PACKAGENAME)_CFLAGS)))
VRUIAPP_LDIRS = -L$$(VRUI_LIBDIR)
VRUIAPP_LDIRS += $(sort $(foreach PACKAGENAME,$(SYSTEMPACKAGES),$($(PACKAGENAME)_LIBDIR)))
VRUIAPP_LIBS = $(patsubst lib%,-l%.$(LDEXT),$(LIBRARY_NAMES))
VRUIAPP_LIBS += $(foreach PACKAGENAME,$(SYSTEMPACKAGES),$($(PACKAGENAME)_LIBS))
VRUIAPP_LFLAGS =
ifneq ($(SYSTEM_HAVE_RPATH),0)
  ifneq ($(USE_RPATH),0)
    VRUIAPP_LFLAGS += -Wl,-rpath=$$(VRUI_LIBDIR)
  endif
endif

# Pseudo-target to dump Vrui compiler and linker flags to a makefile fragment
$(MAKEFILEFRAGMENT): PACKAGES = MYVRUI
$(MAKEFILEFRAGMENT): config
	@echo Creating application makefile fragment...
	@echo '# Makefile fragment for Vrui applications' > $(MAKEFILEFRAGMENT)
	@echo '# Autogenerated by Vrui installation on $(shell date)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_VERSION = $(VRUI_VERSION)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_INCLUDEDIR = $(HEADERINSTALLDIR)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_CFLAGS = $(VRUIAPP_INCLUDEDIRS) $(VRUIAPP_CFLAGS)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_LIBDIR = $(LIBINSTALLDIR)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_LINKFLAGS = $(VRUIAPP_LDIRS) $(VRUIAPP_LIBS) $(VRUIAPP_LFLAGS)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_PLUGINFILEEXT = $(PLUGINFILEEXT)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_PLUGINCFLAGS = $(CPLUGINFLAGS)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_PLUGINLINKFLAGS = $(PLUGINLINKFLAGS)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_PLUGINHOSTLINKFLAGS = $(PLUGINHOSTLINKFLAGS)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_TOOLSDIR = $(PLUGININSTALLDIR)/$(VRTOOLSDIREXT)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_VISLETSDIR = $(PLUGININSTALLDIR)/$(VRVISLETSDIREXT)' >> $(MAKEFILEFRAGMENT)
ifeq ($(SYSTEM),DARWIN)
	@echo 'export MACOSX_DEPLOYMENT_TARGET = $(SYSTEM_DARWIN_VERSION)' >> $(MAKEFILEFRAGMENT)
endif

ROGUE_SYSTEMPACKAGES = $(filter %_,$(foreach PACKAGENAME,$(SYSTEMPACKAGES),$(PACKAGENAME)_$($(PACKAGENAME)_PKGNAME)))
ROGUE_LIBDIRS = $(sort $(foreach PACKAGENAME,$(ROGUE_SYSTEMPACKAGES),$($(PACKAGENAME)LIBDIR)))
ROGUE_LIBS = $(foreach PACKAGENAME,$(ROGUE_SYSTEMPACKAGES),$($(PACKAGENAME)LIBS))

# Pseudo-target to create a metadata file for pkg-config
$(PKGCONFIGFILE): PACKAGES = MYVRUI
$(PKGCONFIGFILE): config
	@echo Creating pkg-config meta data file...
	@echo 'prefix=$(INSTALLDIR)' > $(PKGCONFIGFILE)
	@echo 'exec_prefix=$${prefix}' >> $(PKGCONFIGFILE)
	@echo 'libdir=$(LIBINSTALLDIR)' >> $(PKGCONFIGFILE)
	@echo 'includedir=$(HEADERINSTALLDIR)' >> $(PKGCONFIGFILE)
	@echo '' >> $(PKGCONFIGFILE)
	@echo 'Name: Vrui' >> $(PKGCONFIGFILE)
	@echo 'Description: Vrui (Virtual Reality User Interface) development toolkit' >> $(PKGCONFIGFILE)
	@echo 'Requires: $(strip $(foreach PACKAGENAME,$(SYSTEMPACKAGES),$($(PACKAGENAME)_PKGNAME)))' >> $(PKGCONFIGFILE)
	@echo 'Version: $(VRUI_VERSION)' >> $(PKGCONFIGFILE)
ifneq ($(SYSTEM_HAVE_RPATH),0)
  ifneq ($(USE_RPATH),0)
	@echo 'Libs: -L$${libdir} $(strip $(patsubst lib%,-l%.$(LDEXT),$(LIBRARY_NAMES))) $(ROGUE_LIBDIRS) $(ROGUE_LIBS) -Wl,-rpath=$${libdir}' >> $(PKGCONFIGFILE)
  else
	@echo 'Libs: -L$${libdir} $(strip $(patsubst lib%,-l%.$(LDEXT),$(LIBRARY_NAMES))) $(ROGUE_LIBDIRS) $(ROGUE_LIBS)' >> $(PKGCONFIGFILE)
  endif
else
	@echo 'Libs: -L$${libdir} $(strip $(patsubst lib%,-l%.$(LDEXT),$(LIBRARY_NAMES))) $(ROGUE_LIBDIRS) $(ROGUE_LIBS)' >> $(PKGCONFIGFILE)
endif	
	@echo 'Cflags: -I$${includedir} $(strip $(VRUIAPP_CFLAGS))' >> $(PKGCONFIGFILE)

# Sequence to create symlinks for dynamic libraries:
# First argument: library name
define CREATE_SYMLINK
@-rm -f $(LIBINSTALLDIR)/$(call DSONAME,$(1)) $(LIBINSTALLDIR)/$(call LINKDSONAME,$(1))
@cd $(LIBINSTALLDIR) ; ln -s $(call FULLDSONAME,$(1)) $(call DSONAME,$(1))
@cd $(LIBINSTALLDIR) ; ln -s $(call FULLDSONAME,$(1)) $(call LINKDSONAME,$(1))

endef

install:
# Install all header files in HEADERINSTALLDIR:
	@echo Installing header files...
	@install -d $(HEADERINSTALLDIR)/Misc
	@install -m u=rw,go=r $(MISC_HEADERS) $(HEADERINSTALLDIR)/Misc
	@install -d $(HEADERINSTALLDIR)/Plugins
	@install -m u=rw,go=r $(PLUGINS_HEADERS) $(HEADERINSTALLDIR)/Plugins
	@install -d $(HEADERINSTALLDIR)/Realtime
	@install -m u=rw,go=r $(REALTIME_HEADERS) $(HEADERINSTALLDIR)/Realtime
	@install -d $(HEADERINSTALLDIR)/Threads
	@install -m u=rw,go=r $(THREADS_HEADERS) $(HEADERINSTALLDIR)/Threads
	@install -d $(HEADERINSTALLDIR)/Comm
	@install -m u=rw,go=r $(COMM_HEADERS) $(HEADERINSTALLDIR)/Comm
	@install -d $(HEADERINSTALLDIR)/Math
	@install -m u=rw,go=r $(MATH_HEADERS) $(HEADERINSTALLDIR)/Math
	@install -d $(HEADERINSTALLDIR)/Geometry
	@install -m u=rw,go=r $(GEOMETRY_HEADERS) $(HEADERINSTALLDIR)/Geometry
ifeq ($(SYSTEM),DARWIN)
	@install -d $(HEADERINSTALLDIR)/MacOSX
	@install -m u=rw,go=r $(MACOSX_HEADERS) $(HEADERINSTALLDIR)/MacOSX
endif
	@install -d $(HEADERINSTALLDIR)/GL
	@install -m u=rw,go=r $(GLWRAPPERS_HEADERS) $(HEADERINSTALLDIR)/GL
	@install -m u=rw,go=r $(GLSUPPORT_HEADERS) $(HEADERINSTALLDIR)/GL
	@install -d $(HEADERINSTALLDIR)/GL/Extensions
	@install -m u=rw,go=r $(GLSUPPORTEXTENSION_HEADERS) $(HEADERINSTALLDIR)/GL/Extensions
	@install -m u=rw,go=r $(GLXSUPPORT_HEADERS) $(HEADERINSTALLDIR)/GL
	@install -m u=rw,go=r $(GLGEOMETRY_HEADERS) $(HEADERINSTALLDIR)/GL
	@install -d $(HEADERINSTALLDIR)/GLMotif
	@install -m u=rw,go=r $(GLMOTIF_HEADERS) $(HEADERINSTALLDIR)/GLMotif
	@install -d $(HEADERINSTALLDIR)/Images
	@install -m u=rw,go=r $(IMAGES_HEADERS) $(HEADERINSTALLDIR)/Images
	@install -d $(HEADERINSTALLDIR)/Sound
	@install -m u=rw,go=r $(SOUND_HEADERS) $(HEADERINSTALLDIR)/Sound
ifeq ($(SYSTEM),LINUX)
  ifneq ($(strip $(SOUND_LINUX_HEADERS)),)
	@install -d $(HEADERINSTALLDIR)/Sound/Linux
	@install -m u=rw,go=r $(SOUND_LINUX_HEADERS) $(HEADERINSTALLDIR)/Sound/Linux
  endif
endif
	@install -d $(HEADERINSTALLDIR)/AL
	@install -m u=rw,go=r $(ALSUPPORT_HEADERS) $(HEADERINSTALLDIR)/AL
	@install -d $(HEADERINSTALLDIR)/Video
	@install -m u=rw,go=r $(VIDEO_HEADERS) $(HEADERINSTALLDIR)/Video
ifeq ($(SYSTEM),LINUX)
  ifneq ($(strip $(VIDEO_LINUX_HEADERS)),)
	@install -d $(HEADERINSTALLDIR)/Video/Linux
	@install -m u=rw,go=r $(VIDEO_LINUX_HEADERS) $(HEADERINSTALLDIR)/Video/Linux
  endif
endif
	@install -d $(HEADERINSTALLDIR)/SceneGraph
	@install -m u=rw,go=r $(SCENEGRAPH_HEADERS) $(HEADERINSTALLDIR)/SceneGraph
	@install -d $(HEADERINSTALLDIR)/Vrui
	@install -m u=rw,go=r $(VRUI_HEADERS) $(HEADERINSTALLDIR)/Vrui
# Install all library files in LIBINSTALLDIR:
	@echo Installing libraries...
	@install -d $(LIBINSTALLDIR)
	@install $(LIBRARIES) $(LIBINSTALLDIR)
	@echo Configuring run-time linker...
	$(foreach LIBNAME,$(LIBRARY_NAMES),$(call CREATE_SYMLINK,$(LIBNAME)))
# Install all binaries in BININSTALLDIR:
	@echo Installing executables...
	@install -d $(EXECUTABLEINSTALLDIR)
	@install $(EXECUTABLES) $(EXECUTABLEINSTALLDIR)
ifeq ($(SYSTEM),DARWIN)
	@cp Share/MacOSX/runwithx Share/MacOSX/runwithx.tmp
	@sed -i -e "s:_VRUIETCDIR_:$(ETCINSTALLDIR):" Share/MacOSX/runwithx.tmp
	@sed -i -e "s:_VRUIBINDIR_:$(EXECUTABLEINSTALLDIR):" Share/MacOSX/runwithx.tmp
	@install Share/MacOSX/runwithx.tmp $(EXECUTABLEINSTALLDIR)/runwithx
	@rm Share/MacOSX/runwithx.tmp
endif
# Install all plug-ins in PLUGININSTALLDIR:
	@echo Installing plug-ins...
	@install -d $(PLUGININSTALLDIR)/$(VRTOOLSDIREXT)
	@install $(VRTOOLS) $(PLUGININSTALLDIR)/$(VRTOOLSDIREXT)
	@install -d $(PLUGININSTALLDIR)/$(VRVISLETSDIREXT)
	@install $(VRVISLETS) $(PLUGININSTALLDIR)/$(VRVISLETSDIREXT)
	@install -d $(PLUGININSTALLDIR)/$(VRDEVICESDIREXT)
	@install $(VRDEVICES) $(PLUGININSTALLDIR)/$(VRDEVICESDIREXT)
	@install -d $(PLUGININSTALLDIR)/$(VRCALIBRATORSDIREXT)
	@install $(VRCALIBRATORS) $(PLUGININSTALLDIR)/$(VRCALIBRATORSDIREXT)
# Install all configuration files in ETCINSTALLDIR:
	@echo Installing configuration files...
	@install -d $(ETCINSTALLDIR)
	@install -m u=rw,go=r Share/*.cfg $(ETCINSTALLDIR)
ifeq ($(SYSTEM),DARWIN)
	@install -m u=rw,go=r Share/MacOSX/Vrui.xinitrc $(ETCINSTALLDIR)
endif
# Install all shared files in SHAREINSTALLDIR:
	@echo Installing shared files...
	@install -d $(SHAREINSTALLDIR)/GLFonts
	@install -m u=rw,go=r Share/GLFonts/* $(SHAREINSTALLDIR)/GLFonts
	@install -d $(SHAREINSTALLDIR)/Textures
	@install -m u=rw,go=r Share/Textures/* $(SHAREINSTALLDIR)/Textures
	@install -d $(SHAREINSTALLDIR)/Shaders
	@install -d $(SHAREINSTALLDIR)/Shaders/SceneGraph
	@install -m u=rw,go=r Share/Shaders/SceneGraph/* $(SHAREINSTALLDIR)/Shaders/SceneGraph
# Install makefile fragment in SHAREINSTALLDIR:
	@echo Installing application makefile fragment...
	@install -m u=rw,go=r $(MAKEFILEFRAGMENT) $(SHAREINSTALLDIR)
# Install full build system in SHAREINSTALLDIR/make:
	@echo Installing build system...
	@install -d $(SHAREINSTALLDIR)/make
	@install -m u=rw,go=r BuildRoot/* $(SHAREINSTALLDIR)/make
# Install pkg-config metafile in PKGCONFIGINSTALLDIR:
	@echo Installing pkg-config metafile...
	@install -d $(PKGCONFIGINSTALLDIR)
	@install -m u=rw,go=r $(PKGCONFIGFILE) $(PKGCONFIGINSTALLDIR)
# Install documentation in DOCINSTALLDIR:
	@echo Installing documentation...
	@install -d $(DOCINSTALLDIR)
	@install -m u=rw,go=r Documentation/* $(DOCINSTALLDIR)
