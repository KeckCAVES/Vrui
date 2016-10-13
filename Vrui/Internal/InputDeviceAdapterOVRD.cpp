/***********************************************************************
InputDeviceAdapterOVRD - Class to connect Oculus VR's Rift tracking
daemon to a Vrui application.
Copyright (c) 2015-2016 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Vrui/Internal/InputDeviceAdapterOVRD.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string>
#include <Misc/SizedTypes.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/Time.h>
#include <Misc/ValueCoder.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Threads/Thread.h>
#include <IO/FixedMemoryFile.h>
#include <Comm/TCPPipe.h>
#include <Math/Math.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <Vrui/Vrui.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/InputGraphManager.h>

namespace Vrui {

namespace {

/***********************************************************************
Helper classes and functions. These are pretty ugly; deal with it.
***********************************************************************/

/***********************************************************************
Functions to communicate with Oculus VR's tracking daemon via its TCP
socket:
***********************************************************************/

inline void writeString(const std::string& string,IO::File& pipe)
	{
	/* Write string size and string data to pipe: */
	pipe.write<Misc::UInt16>(string.size());
	pipe.write<char>(string.data(),string.size());
	}

inline std::string readString(IO::File& pipe)
	{
	/* Read string length: */
	size_t stringLen=pipe.read<Misc::UInt16>();
	std::string result;
	result.reserve(stringLen);
	for(size_t i=0;i<stringLen;++i)
		result.push_back(pipe.read<char>());
	return result;
	}

inline void writeMessage(const IO::FixedMemoryFile& message,IO::File& pipe)
	{
	/* Write the size of the message: */
	pipe.write<Misc::UInt32>(message.getWriteSize());
	
	/* Write the message contents: */
	pipe.writeRaw(message.getMemory(),message.getWriteSize());
	
	/* Send the message on its way: */
	pipe.flush();
	}

inline void readMessage(IO::FixedMemoryFile& message,IO::File& pipe)
	{
	/* Read the size of the message: */
	size_t size=pipe.read<Misc::UInt32>();
	
	/* Read the message contents: */
	pipe.readRaw(message.getMemory(),size);
	message.setReadDataSize(size);
	}

inline void writeRPCMessage(Misc::UInt8 header0,Misc::UInt8 header1,const IO::FixedMemoryFile& message,IO::File& pipe)
	{
	/* Write the size of the message including the RPC header: */
	pipe.write<Misc::UInt32>(message.getWriteSize()+(unsigned int)(sizeof(Misc::UInt8)*2));
	
	/* Write the RPC header: */
	pipe.write(header0);
	pipe.write(header1);
	
	/* Write the message contents: */
	pipe.writeRaw(message.getMemory(),message.getWriteSize());
	
	/* Send the message on its way: */
	pipe.flush();
	}

inline void readRPCMessage(Misc::UInt8& header0,Misc::UInt8& header1,IO::FixedMemoryFile& message,IO::File& pipe)
	{
	/* Read the size of the message including the RPC header: */
	size_t size=pipe.read<Misc::UInt32>();
	
	/* Read the RPC header: */
	pipe.read(header0);
	pipe.read(header1);
	size-=sizeof(Misc::UInt8)*2;
	
	/* Read the message contents: */
	pipe.readRaw(message.getMemory(),size);
	message.setReadDataSize(size);
	}

template <class ResultType>
inline
void
readRPCResult(
	Misc::UInt8& header0,
	Misc::UInt8& header1,
	ResultType& result,
	IO::FixedMemoryFile& message,
	IO::File& pipe)
	{
	/* Read the size of the message including the RPC header: */
	size_t size=pipe.read<Misc::UInt32>();
	
	/* Read the RPC header: */
	pipe.read(header0);
	pipe.read(header1);
	size-=sizeof(Misc::UInt8)*2;
	
	/* Read the message contents: */
	pipe.readRaw(message.getMemory(),size);
	message.setReadDataSize(size);
	
	/* Read the result packet: */
	result.read(message);
	}

template <class PacketType>
inline
void
writePacket(
	const PacketType& packet,
	IO::FixedMemoryFile& message,
	IO::File& pipe)
	{
	/* Write the packet into the message: */
	message.clear();
	packet.write(message);
	
	/* Write the message to the pipe: */
	writeMessage(message,pipe);
	}

template <class PacketType>
inline
void
readPacket(
	PacketType& packet,
	IO::FixedMemoryFile& message,
	IO::File& pipe)
	{
	/* Read the next message from the pipe: */
	readMessage(message,pipe);
	
	/* Read the packet from the message: */
	packet.read(message);
	}

template <class PacketType>
inline
void
writeRPCPacket(
	Misc::UInt8 header0,
	Misc::UInt8 header1,
	const PacketType& packet,
	IO::FixedMemoryFile& message,
	IO::File& pipe)
	{
	/* Write the RPC header into the message: */
	message.clear();
	message.write(header0);
	message.write(header1);
	
	/* Write the packet into the message: */
	packet.write(message);
	
	/* Write the message to the pipe: */
	writeMessage(message,pipe);
	}

template <class PacketType>
inline
void
readRPCPacket(
	Misc::UInt8& header0,
	Misc::UInt8& header1,
	PacketType& packet,
	IO::FixedMemoryFile& message,
	IO::File& pipe)
	{
	/* Read the next message from the pipe: */
	readMessage(message,pipe);
	
	/* Read the RPC header from the message: */
	message.read(header0);
	message.read(header1);
	
	/* Read the packet from the message: */
	packet.read(message);
	}

/***********************************************************************
Classes encapsulating ovrd protocol messages:
***********************************************************************/

struct ProtocolPacket // Base class for protocol packets
	{
	/* Elements: */
	public:
	std::string packetType; // String identifying the type of this packet
	
	/* Constructors and destructors: */
	public:
	ProtocolPacket(const char* sPacketType)
		:packetType(sPacketType)
		{
		}
	
	/* Methods: */
	void write(IO::File& file) const
		{
		/* Write the packet type string: */
		size_t ptLen=packetType.size();
		file.write(Misc::UInt16(ptLen));
		file.write<char>(packetType.data(),ptLen);
		}
	void read(IO::File& file)
		{
		/* Read the packet type string: */
		size_t ptLen=file.read<Misc::UInt16>();
		packetType.reserve(ptLen);
		for(size_t i=0;i<ptLen;++i)
			packetType.push_back(file.read<char>());
		}
	};

struct Connect:public ProtocolPacket // Packet sent from client to server to initiate a connection, or from server to client to acknowledge a connection
	{
	/* Elements: */
	public:
	Misc::UInt16 version[3]; // Protocol version (major, minor, patch)
	Misc::UInt16 sdkVersion[7]; // SDK version (product, major, minor, requested minor, patch, build, feature)
	
	/* Constructors and destructors: */
	Connect(bool request,bool initialize)
		:ProtocolPacket(initialize?(request?"OculusVR_Hello":"OculusVR_Authorized"):"")
		{
		if(initialize)
			{
			/* Put in the version numbers for SDK 0.5.0.1: */
			version[0]=1U;
			version[1]=3U;
			version[2]=0U;
			sdkVersion[0]=0U;
			sdkVersion[1]=5U;
			sdkVersion[2]=0U;
			sdkVersion[3]=0U;
			sdkVersion[4]=1U;
			sdkVersion[5]=0U;
			sdkVersion[6]=0U;
			}
		else
			{
			for(int i=0;i<3;++i)
				version[i]=0U;
			for(int i=0;i<7;++i)
				sdkVersion[i]=0U;
			}
		}
	
	/* Methods: */
	void write(IO::File& file) const // Writes connect request or reply to a file or pipe
		{
		ProtocolPacket::write(file);
		file.write(version,3);
		file.write(sdkVersion,7);
		}
	void read(IO::File& file) // Reads connect request or reply from a file or pipe
		{
		ProtocolPacket::read(file);
		file.read(version,3);
		file.read(sdkVersion,7);
		}
	};

struct InitialServerState:public ProtocolPacket
	{
	/* Elements: */
	public:
	Misc::UInt8 state; // Server state
	
	/* Constructors and destructors: */
	InitialServerState(bool initialize,unsigned int sState =0)
		:ProtocolPacket(initialize?"InitialServerState_1":"")
		{
		if(initialize)
			{
			state=sState;
			}
		else
			{
			state=0;
			}
		}
	
	/* Methods: */
	void write(IO::File& file) const // Writes initial server state message to a file or pipe
		{
		ProtocolPacket::write(file);
		file.write(state);
		}
	void read(IO::File& file) // Reads initial server state message from a file or pipe
		{
		ProtocolPacket::read(file);
		file.read(state);
		}
	};

struct HmdDetect:public ProtocolPacket
	{
	/* Constructors and destructors: */
	public:
	HmdDetect(bool initialize)
		:ProtocolPacket(initialize?"Hmd_Detect_1":"")
		{
		}
	
	/* Methods: */
	void write(IO::File& file) const // Writes HMD detect request to a file or pipe
		{
		ProtocolPacket::write(file);
		}
	void read(IO::File& file) // Reads HMD detect request from a file or pipe
		{
		ProtocolPacket::read(file);
		}
	};

struct HmdDetectResult
	{
	/* Elements: */
	public:
	Misc::UInt32 numHmds;
	
	/* Methods: */
	void read(IO::File& file)
		{
		file.read(numHmds);
		}
	};

struct HmdCreate:public ProtocolPacket
	{
	/* Elements: */
	public:
	Misc::UInt32 index; // Index of HMD to create
	Misc::UInt32 pid; // Process ID of client process
	
	/* Constructors and destructors: */
	public:
	HmdCreate(bool initialize,unsigned int sIndex =0)
		:ProtocolPacket(initialize?"Hmd_Create_1":"")
		{
		if(initialize)
			{
			index=sIndex;
			pid=getpid();
			}
		else
			{
			index=0;
			pid=0;
			}
		}
	
	/* Methods: */
	void write(IO::File& file) const // Writes HMD create request to a file or pipe
		{
		ProtocolPacket::write(file);
		file.write(index);
		file.write(pid);
		}
	void read(IO::File& file) // Reads HMD detect create from a file or pipe
		{
		ProtocolPacket::read(file);
		file.read(index);
		file.read(pid);
		}
	};

struct HmdCreateResult
	{
	/* Elements: */
	public:
	Misc::UInt32 hmdId;
	std::string hmdSharedMemoryName;
	std::string camSharedMemoryName;
	
	/* Methods: */
	void read(IO::File& file)
		{
		file.read(hmdId);
		hmdSharedMemoryName=readString(file);
		camSharedMemoryName=readString(file);
		}
	};

struct HmdGetHmdInfo:public ProtocolPacket
	{
	/* Elements: */
	public:
	Misc::UInt32 hmdId; // Network ID of HMD to query
	
	/* Constructors and destructors: */
	public:
	HmdGetHmdInfo(bool initialize,unsigned int sHmdId =0)
		:ProtocolPacket(initialize?"Hmd_GetHmdInfo_1":"")
		{
		if(initialize)
			{
			hmdId=sHmdId;
			}
		else
			{
			hmdId=0;
			}
		}
	
	/* Methods: */
	void write(IO::File& file) const // Writes HMD create request to a file or pipe
		{
		ProtocolPacket::write(file);
		file.write(hmdId);
		}
	void read(IO::File& file) // Reads HMD detect create from a file or pipe
		{
		ProtocolPacket::read(file);
		file.read(hmdId);
		}
	};

struct HmdGetHmdInfoResult
	{
	/* Elements: */
	public:
	std::string productName;
	std::string manufacturer;
	Misc::UInt32 version;
	Misc::UInt32 hmdType;
	Misc::UInt32 numPixels[2];
	Misc::UInt32 deviceNumber;
	Misc::UInt32 nativeSize[2];
	Misc::UInt32 rotation;
	Misc::Float32 screenSize[2];
	Misc::Float32 screenGap;
	Misc::Float32 centerFromTop;
	Misc::Float32 lensSeparation;
	Misc::UInt32 windowPos[2];
	Misc::UInt32 shutterType;
	Misc::Float32 vsyncInterval;
	Misc::Float32 firstScanLineDelta;
	Misc::Float32 lastFirstScanLineDelta;
	Misc::Float32 pixelSettleTime;
	Misc::Float32 pixelPersistence;
	std::string displayDeviceName;
	Misc::UInt32 displayId;
	std::string serialNumber;
	bool inCompatibilityMode;
	Misc::UInt32 vendorId,productId;
	Misc::Float32 farZ;
	Misc::Float32 horizFov;
	Misc::Float32 nearZ;
	Misc::Float32 vertFov;
	Misc::UInt32 firmwareMajor,firmwareMinor;
	Misc::Float32 pelOffsetR[2];
	Misc::Float32 pelOffsetB[2];
	
	/* Methods: */
	void read(IO::File& file)
		{
		productName=readString(file);
		manufacturer=readString(file);
		file.read(version);
		file.read(hmdType);
		file.read(numPixels,2);
		file.read(deviceNumber);
		file.read(nativeSize,2);
		file.read(rotation);
		file.read(screenSize,2);
		file.read(screenGap);
		file.read(centerFromTop);
		file.read(lensSeparation);
		file.read(windowPos,2);
		file.read(shutterType);
		file.read(vsyncInterval);
		file.read(firstScanLineDelta);
		file.read(lastFirstScanLineDelta);
		file.read(pixelSettleTime);
		file.read(pixelPersistence);
		displayDeviceName=readString(file);
		file.read(displayId);
		serialNumber=readString(file);
		inCompatibilityMode=file.read<Misc::UInt8>()!=0;
		file.read(vendorId);
		file.read(productId);
		file.read(farZ);
		file.read(horizFov);
		file.read(nearZ);
		file.read(vertFov);
		file.read(firmwareMajor);
		file.read(firmwareMinor);
		file.read(pelOffsetR,2);
		file.read(pelOffsetB,2);
		}
	};

struct HmdSetEnabledCaps:public ProtocolPacket
	{
	/* Embedded classes: */
	public:
	enum DeviceCapabilities
		{
    NoMirrorToWindow=0x2000,
    DisplayOff=0x0040,
    LowPersistence=0x0080,
    DynamicPrediction=0x0200,
    NoVSync=0x1000
		};
	
	/* Elements: */
	Misc::UInt32 hmdId; // Network ID of HMD to set/query
	Misc::UInt32 enabledCaps; // Enabled capabilities
	
	/* Constructors and destructors: */
	HmdSetEnabledCaps(bool initialize,unsigned int sHmdId,unsigned int sEnabledCaps)
		:ProtocolPacket(initialize?"Hmd_SetEnabledCaps_1":"")
		{
		if(initialize)
			{
			hmdId=sHmdId;
			enabledCaps=sEnabledCaps;
			}
		else
			{
			hmdId=0;
			enabledCaps=0;
			}
		}
	
	/* Methods: */
	void write(IO::File& file) const
		{
		ProtocolPacket::write(file);
		file.write(hmdId);
		file.write(enabledCaps);
		}
	void read(IO::File& file)
		{
		ProtocolPacket::read(file);
		file.read(hmdId);
		file.read(enabledCaps);
		}
	};

struct HmdSetEnabledCapsResult
	{
	/* Elements: */
	public:
	Misc::UInt32 enabledCaps;
	
	/* Methods: */
	void read(IO::File& file)
		{
		file.read(enabledCaps);
		}
	};

struct HmdConfigureTracking:public ProtocolPacket
	{
	/* Embedded classes: */
	public:
	enum TrackingCapabilities
		{
    Orientation=0x10,
    YawDriftCorrection=0x20,
    Position=0x40
		};
	
	/* Elements: */
	Misc::UInt32 hmdId; // Network ID of HMD to set/query
	Misc::UInt32 supportedCaps; // Caps supported by the client?
	Misc::UInt32 requiredCaps; // Caps required by the client?
	
	/* Constructors and destructors: */
	public:
	HmdConfigureTracking(bool initialize,unsigned int sHmdId=0,unsigned int sSupportedCaps =0,unsigned int sRequiredCaps =0)
		:ProtocolPacket(initialize?"Hmd_ConfigureTracking_1":"")
		{
		if(initialize)
			{
			hmdId=sHmdId;
			supportedCaps=sSupportedCaps;
			requiredCaps=sRequiredCaps;
			}
		else
			{
			hmdId=0;
			supportedCaps=0;
			requiredCaps=0;
			}
		}
	
	/* Methods: */
	void write(IO::File& file) const // Writes HMD create request to a file or pipe
		{
		ProtocolPacket::write(file);
		file.write(hmdId);
		file.write(supportedCaps);
		file.write(requiredCaps);
		}
	void read(IO::File& file) // Reads HMD detect create from a file or pipe
		{
		ProtocolPacket::read(file);
		file.read(hmdId);
		file.read(supportedCaps);
		file.read(requiredCaps);
		}
	};

struct HmdRelease:public ProtocolPacket
	{
	/* Elements: */
	public:
	Misc::UInt32 hmdId; // Network ID of HMD to release
	
	/* Constructors and destructors: */
	public:
	HmdRelease(bool initialize,unsigned int sHmdId =0)
		:ProtocolPacket(initialize?"Hmd_Release_1":"")
		{
		if(initialize)
			{
			hmdId=sHmdId;
			}
		else
			{
			hmdId=0;
			}
		}
	
	/* Methods: */
	void write(IO::File& file) const // Writes HMD create request to a file or pipe
		{
		ProtocolPacket::write(file);
		file.write(hmdId);
		}
	void read(IO::File& file) // Reads HMD detect create from a file or pipe
		{
		ProtocolPacket::read(file);
		file.read(hmdId);
		}
	};

/***************************************************
Classes to access ovrd's shared memory IPC protocol:
***************************************************/

template <class ScalarParam>
class OVRONTransform // Memory layout of an orthonormal transformation (rigid body transformation) as used by Oculus VR
	{
	/* Embedded classes: */
	public:
	typedef ScalarParam Scalar;
	typedef Geometry::Point<ScalarParam,3> Point;
	typedef Geometry::Vector<ScalarParam,3> Vector;
	typedef Geometry::Rotation<ScalarParam,3> Rotation;
	typedef Geometry::OrthonormalTransformation<ScalarParam,3> ONTransform; // Compatible transformation type
	
	/* Elements: */
	private:
	Rotation rotation; // At least their quaternion component order matches mine...
	Vector translation;
	
	/* Constructors and destructors: */
	public:
	OVRONTransform(void) // Creates identity transformation
		:rotation(Rotation::identity),
		 translation(Vector::zero)
		{
		}
	OVRONTransform(const ONTransform& source) // Converts from a real orthonormal transformation
		:rotation(source.getRotation()),
		 translation(source.getTranslation())
		{
		}
	
	/* Methods: */
	operator ONTransform(void) const // Convert to real orthonormal transformation
		{
		return ONTransform(translation,rotation);
		}
	const Rotation& getRotation(void) const
		{
		return rotation;
		}
	const Vector& getTranslation(void) const
		{
		return translation;
		}
	Point transform(const Point& point) const // Transforms a point
		{
		return rotation.transform(point)+translation;
		}
	Vector transform(const Vector& vector) const // Transforms a vector
		{
		return rotation.transform(vector);
		}
	};

template <class ValueParam,size_t paddedSizeParam>
struct Padding
	{
	/* Embedded classes: */
	public:
	typedef ValueParam Value;
	static const size_t paddedSize=paddedSizeParam;
	
	/* Elements: */
	ValueParam value; // The original value
	Misc::UInt8 padding[paddedSizeParam-sizeof(Value)];
	
	/* Methods: */
	operator Value(void) const
		{
		return value;
		}
	};

template <class ValueParam>
class AtomicReader // Tiny class for memory-fenced reads from a POD, currently gcc on x86 / x86_64 only
	{
	/* Embedded classes: */
	public:
	typedef ValueParam Value;
	
	/* Elements: */
	private:
	volatile Value value;
	
	/* Methods: */
	public:
	Value loadAcquire(void) const
		{
		/* gcc method of inserting a compiler fence: */
		asm volatile ("" : : : "memory");
		
		return value;
		}
	};

template <class ValueParam,class PaddingParam =ValueParam>
class DoubleBuffer // Class for lockless data exchange using a double buffer, using John Carmack's method
	{
	/* Embedded classes: */
	public:
	typedef ValueParam Value;
	typedef PaddingParam Padding;
	
	/* Elements: */
	private:
	AtomicReader<int> updateBegin,updateEnd;
	Padding slots[2];
	
	/* Constructors and destructors: */
	public:
	DoubleBuffer(void)
		{
		}
	
	/* Methods: */
	Value getState(void) const
		{
		Value result;
		
		while(true)
			{
			/* Try reading complete value from the first buffer slot: */
			int end=updateEnd.loadAcquire();
			result=slots[end&1];
			int begin=updateBegin.loadAcquire();
			
			/* Check if the value was partly overwritten while we were reading it: */
			if(end==begin)
				break;
			
			/* If it didn't work at first, try and try again: */
			result=slots[(~begin)&1];
			int begin2=updateBegin.loadAcquire();
			if(begin2==begin)
				break;
			}
		
		return result;
		}
	};

struct CameraState // Camera state communicated through service's shared memory segment
	{
	/* Elements: */
	public:
	OVRONTransform<Misc::Float64> cameraToWorld;
	Misc::UInt32 statusFlags;
	Misc::UInt32 pad0;
	};

struct HMDState // HMD state communicated through service's shared memory segment
	{
	/* Embedded classes: */
	public:
	typedef Misc::Float64 Scalar;
	typedef Geometry::Vector<Scalar,3> Vector;
	typedef Geometry::Vector<Misc::Float32,3> Vectorf;
	
	/* Elements: */
	public:
	OVRONTransform<Scalar> imuToWorld;
	Vector imuAngularVelocity;
	Vector imuLinearVelocity;
	Vector imuAngularAcceleration;
	Vector imuLinearAcceleration;
	Misc::Float64 imuTime;
	
	Vectorf rawAccelerometer;
	Vectorf rawGyroscope;
	Vectorf rawMagnetometer;
	Misc::Float32 rawTemperature;
	Misc::Float64 rawTime;
	
	OVRONTransform<Scalar> cameraToWorld; // Deprecated
	
	Misc::UInt32 statusFlags;
	Misc::UInt32 pad0;
	
	OVRONTransform<Scalar> cpfToImu; // Transformation from center pupil frame to IMU frame
	};

}

class SharedMemory // Class representing a block of shared memory mapped into the process' address space
	{
	/* Elements: */
	private:
	Misc::UInt8* memory; // Base pointer to the mapped shared memory segment
	size_t size; // Size of mapped shared memory segment
	
	/* Constructors and destructors: */
	public:
	SharedMemory(const char* sharedMemoryName)
		:memory(0),size(0)
		{
		/* Create the OS-level name of the shared memory segment: */
		std::string name="/";
		name.append(sharedMemoryName);
		
		/* Open the shared memory segment: */
		int fd=shm_open(name.c_str(),O_RDONLY,0);
		if(fd<0)
			Misc::throwStdErr("SharedMemory::SharedMemory: Unable to access shared memory segment %s",sharedMemoryName);
		
		/* Query the shared memory segment's size: */
		struct stat sharedMemoryStats;
		if(fstat(fd,&sharedMemoryStats)<0)
			{
			close(fd);
			Misc::throwStdErr("SharedMemory::SharedMemory: Unable to query size of shared memory segment %s",sharedMemoryName);
			}
		size=size_t(sharedMemoryStats.st_size);
		
		/* Map the shared memory segment into the process' address space: */
		void* address=mmap(0,size,PROT_READ,MAP_SHARED,fd,0);
		if(address==(void*)-1)
			{
			close(fd);
			Misc::throwStdErr("SharedMemory::SharedMemory: Unable to map shared memory segment %s",sharedMemoryName);
			}
		
		/* Get a byte-sized pointer to the shared memory segment: */
		memory=static_cast<Misc::UInt8*>(address);
		
		/* Close the shared memory segment's file handle; no longer needed while memory is mapped: */
		close(fd);
		}
	~SharedMemory(void)
		{
		/* Unmap the shared memory segment: */
		munmap(memory,size);
		}
	
	/* Methods: */
	size_t getSize(void) const // Returns the size of the shared memory segment
		{
		return size;
		}
	template <class ValueParam>
	const ValueParam* getValue(ptrdiff_t offset) const // Accesses a variable at a byte-based offset in shared memory
		{
		return reinterpret_cast<const ValueParam*>(memory+offset);
		}
	};

/***************************************
Methods of class InputDeviceAdapterOVRD:
***************************************/

void* InputDeviceAdapterOVRD::ovrdProtocolListenerThreadMethod(void)
	{
	while(keepListening)
		{
		/* Wait for the next message: */
		if(serverPipe->waitForData(Misc::Time(1,0)))
			{
			/* Read and ignore the message: */
			Misc::UInt8 rpcHeader0,rpcHeader1;
			readRPCMessage(rpcHeader0,rpcHeader1,*messageBuffer,*serverPipe);
			}
		}
	
	return 0;
	}

InputDeviceAdapterOVRD::InputDeviceAdapterOVRD(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection)
	:InputDeviceAdapter(sInputDeviceManager),
	 hmdId(~0x0U),
	 cameraTransform(configFileSection.retrieveValue<TrackerState>("./cameraTransform",TrackerState::identity)),
	 postTransform(configFileSection.retrieveValue<TrackerState>("./postTransform",TrackerState::identity))
	{
	/* Connect to the OVR tracking daemon: */
	serverPipe=new Comm::TCPPipe(0,30322);
	serverPipe->setEndianness(Misc::LittleEndian);
	
	/* Create a temporary message buffer: */
	messageBuffer=new IO::FixedMemoryFile(4096);
	messageBuffer->setEndianness(Misc::BigEndian);
	
	/* Send a connect request: */
	Connect connectReq(true,true);
	writePacket(connectReq,*messageBuffer,*serverPipe);
	
	/* Receive a connect reply: */
	Connect connectRep(false,false);
	readPacket(connectRep,*messageBuffer,*serverPipe);
	if(connectRep.packetType!="OculusVR_Authorized")
		Misc::throwStdErr("Vrui::InputDeviceAdapterOVRD: OVR tracking daemon rejected connection");
	
	/*********************************************************************
	Exchange RPC messages with the OVR tracking daemon to set up tracking:
	*********************************************************************/
	
	Misc::UInt8 rpcHeader0,rpcHeader1;
	
	/* Receive the initial server state: */
	InitialServerState iss(false);
	readRPCPacket(rpcHeader0,rpcHeader1,iss,*messageBuffer,*serverPipe);
	
	/* Send an HMD detection request: */
	HmdDetect hmdDetect(true);
	writeRPCPacket(0x00U,0x01U,hmdDetect,*messageBuffer,*serverPipe);
	
	/* Receive the HMD detection reply: */
	HmdDetectResult hmdDetectResult;
	readRPCResult(rpcHeader0,rpcHeader1,hmdDetectResult,*messageBuffer,*serverPipe);
	if(hmdDetectResult.numHmds==0)
		Misc::throwStdErr("Vrui::InputDeviceAdapterOVRD: No Oculus Rift HMDs connected");
	
	/* Send an HMD creation request for the first HMD: */
	HmdCreate hmdCreate(true,0);
	writeRPCPacket(0x00U,0x01U,hmdCreate,*messageBuffer,*serverPipe);
	
	/* Read the HMD creation reply: */
	HmdCreateResult hmdCreateResult;
	readRPCResult(rpcHeader0,rpcHeader1,hmdCreateResult,*messageBuffer,*serverPipe);
	hmdId=hmdCreateResult.hmdId;
	
	/* Get information for the first HMD: */
	HmdGetHmdInfo hmdGetHmdInfo(true,hmdId);
	writeRPCPacket(0x00U,0x01U,hmdGetHmdInfo,*messageBuffer,*serverPipe);
	
	/* Read the HMD information reply: */
	HmdGetHmdInfoResult hmdGetHmdInfoResult;
	readRPCResult(rpcHeader0,rpcHeader1,hmdGetHmdInfoResult,*messageBuffer,*serverPipe);
	
	/* Enable tracking capabilities on first HMD: */
	unsigned int hmdCaps=HmdSetEnabledCaps::LowPersistence;
	if(configFileSection.retrieveValue<bool>("./dynamicPrediction",false))
		hmdCaps|=HmdSetEnabledCaps::DynamicPrediction;
	HmdSetEnabledCaps hmdSetEnabledCaps(true,hmdId,hmdCaps);
	writeRPCPacket(0x00U,0x01U,hmdSetEnabledCaps,*messageBuffer,*serverPipe);
	
	/* Read enable result: */
	HmdSetEnabledCapsResult hmdSetEnabledCapsResult;
	readRPCResult(rpcHeader0,rpcHeader1,hmdSetEnabledCapsResult,*messageBuffer,*serverPipe);
	
	/* Configure tracking: */
	unsigned int hmdTrackingMode=HmdConfigureTracking::Orientation|HmdConfigureTracking::YawDriftCorrection|HmdConfigureTracking::Position;
	HmdConfigureTracking hmdConfigureTracking(true,hmdId,hmdTrackingMode,0);
	writeRPCPacket(0x00U,0x01U,hmdConfigureTracking,*messageBuffer,*serverPipe);
	
	/* Read configuration result: */
	readRPCMessage(rpcHeader0,rpcHeader1,*messageBuffer,*serverPipe);
	
	/* Handle any additional protocol messages in a background thread: */
	keepListening=true;
	ovrdProtocolListenerThread.start(this,&InputDeviceAdapterOVRD::ovrdProtocolListenerThreadMethod);
	
	/**********************************************************
	Connect to the OVR tracking daemon's shared memory regions:
	**********************************************************/
	
	/* Open HMD and camera sensor state shared memory regions: */
	hmdMem=new SharedMemory(hmdCreateResult.hmdSharedMemoryName.c_str());
	camMem=new SharedMemory(hmdCreateResult.camSharedMemoryName.c_str());
	
	/******************************************
	Initialize Vrui input device adapter state:
	******************************************/
	
	/* Allocate new adapter state arrays: */
	numInputDevices=1;
	inputDevices=new InputDevice*[numInputDevices];
	
	/* Create new input device: */
	std::string deviceName=configFileSection.retrieveString("./name","OculusRift");
	inputDevices[0]=inputDeviceManager->createInputDevice(deviceName.c_str(),InputDevice::TRACK_POS|InputDevice::TRACK_DIR|InputDevice::TRACK_ORIENT,0,0,true);
	inputDevices[0]->setDeviceRay(configFileSection.retrieveValue<Vector>("./deviceRayDirection",Vector(0,1,0)),configFileSection.retrieveValue<Scalar>("./deviceRayStart",-getInchFactor()));
	
	/* Initialize the new device's glyph from the current configuration file section: */
	Glyph& deviceGlyph=inputDeviceManager->getInputGraphManager()->getInputDeviceGlyph(inputDevices[0]);
	deviceGlyph.configure(configFileSection,"./deviceGlyphType","./deviceGlyphMaterial");
	}

InputDeviceAdapterOVRD::~InputDeviceAdapterOVRD(void)
	{
	/* Release the shared memory regions: */
	delete hmdMem;
	delete camMem;
	
	/* Stop the background listening thread: */
	keepListening=false;
	ovrdProtocolListenerThread.join();
	
	Misc::UInt8 rpcHeader0,rpcHeader1;
	
	/* Release the HMD: */
	HmdRelease hmdRelease(true,hmdId);
	writeRPCPacket(0x00U,0x01U,hmdRelease,*messageBuffer,*serverPipe);
	
	/* Read release result: */
	readRPCMessage(rpcHeader0,rpcHeader1,*messageBuffer,*serverPipe);
	}

void InputDeviceAdapterOVRD::updateInputDevices(void)
	{
	/* Map double-buffered state readers to the shared memory segments: */
	typedef DoubleBuffer<HMDState,Padding<HMDState,512> > HMDStateReader;
	const HMDStateReader* hmdStateReader=hmdMem->getValue<HMDStateReader>(0);
	
	// typedef DoubleBuffer<CameraState,Padding<CameraState,512> > CamStateReader;
	// const CamStateReader* camStateReader=camMem->getValue<CamStateReader>(0);
	
	/* Read the most recent HMD tracking state: */
	HMDState hmdState=hmdStateReader->getState();
	TrackerState imuToWorld=TrackerState(hmdState.imuToWorld);
	
	/* Convert imu translation vector from meters to physical Vrui units: */
	imuToWorld.getTranslation()*=getMeterFactor();
	
	/* Transform the HMD tracking state with the camera and post-transformations: */
	imuToWorld.leftMultiply(cameraTransform);
	imuToWorld*=postTransform;
	imuToWorld.renormalize();
	inputDevices[0]->setTransformation(imuToWorld);
	
	/* Copy transformed linear and angular acceleration: */
	inputDevices[0]->setLinearVelocity(hmdState.imuLinearVelocity*getMeterFactor());
	inputDevices[0]->setAngularVelocity(hmdState.imuAngularVelocity);
	}

TrackerState InputDeviceAdapterOVRD::peekTrackerState(int deviceIndex)
	{
	/* Map double-buffered state readers to the shared memory segments: */
	typedef DoubleBuffer<HMDState,Padding<HMDState,512> > HMDStateReader;
	const HMDStateReader* hmdStateReader=hmdMem->getValue<HMDStateReader>(0);
	
	// typedef DoubleBuffer<CameraState,Padding<CameraState,512> > CamStateReader;
	// const CamStateReader* camStateReader=camMem->getValue<CamStateReader>(0);
	
	/* Read the most recent HMD tracking state: */
	HMDState hmdState=hmdStateReader->getState();
	TrackerState imuToWorld=TrackerState(hmdState.imuToWorld);
	
	/* Convert imu translation vector from meters to physical Vrui units: */
	imuToWorld.getTranslation()*=getMeterFactor();
	
	/* Transform the HMD tracking state with the camera and post-transformations: */
	imuToWorld.leftMultiply(cameraTransform);
	imuToWorld*=postTransform;
	imuToWorld.renormalize();
	
	return imuToWorld;
	}

}
