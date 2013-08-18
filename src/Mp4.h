#ifndef MP4_H
#define MP4_H


#include <vector>
#include <string>
#include <arpa/inet.h> //htonl
#include "Log.h"
#include "Socket.h"


using namespace std;

typedef uint16_t     UInt16;
typedef unsigned int UInt32;
typedef size_t       UInt64;

#define PRTYPE(type) ((char*)&type)[0] << ((char*)&type)[1] << ((char*)&type)[2] << ((char*)&type)[3]
#define MKTYPE(a, b, c, d) ((a) | (b<<8) | (c<<16) | (d<<24))

struct CPacketEntry
{
	UInt32 m_RelativeTime;
	UInt32 m_HeaderInfo;
	UInt16 m_Flags;
	UInt16 m_Count;
};

struct Atom
{
	UInt32 type;
	size_t offset; /** From 0. */
	size_t size; /** Exclude header size. */
};

class CSample
{
public:
	size_t m_Timestamp;
	size_t m_Duration;
	size_t m_Offset;
	size_t m_Len;
};

class CMp4Demuxer: public CLogger
{
public:
	CMp4Demuxer();
	bool Parse(const string &path);
	void Send(const string &path);
private:
	ssize_t Read16BE(int fd, UInt16&);
	ssize_t Read16BE(int fd, UInt16&, size_t);
	ssize_t Read32BE(int fd, UInt32&);
	bool ParseAtom(int fd, Atom a);
	bool ParseDefault(int fd, Atom a);
	bool ParseFtyp(int fd, Atom a);
	bool ParseTrak(int fd, Atom a);
	bool ParseTkhd(int fd, Atom a);
	bool ParseMdhd(int fd, Atom a);
	bool ParseHdlr(int fd, Atom a);
	bool ParseVmhd(int fd, Atom a);
	bool ParseSmhd(int fd, Atom a);
	bool ParseDref(int fd, Atom a);
	bool ParseStsd(int fd, Atom a);
	bool ParseStts(int fd, Atom a);
	bool ParseStsz(int fd, Atom a);
	bool ParseStsc(int fd, Atom a);
	bool ParseStco(int fd, Atom a);
	bool ParseStss(int fd, Atom a);
	bool ParseElst(int fd, Atom a);
	bool ParseHmhd(int fd, Atom a);
	bool ParseHint(int fd, Atom a);
	bool ParseSdp(int fd, Atom a);
	bool ParseMeta(int fd, Atom a);
public:
	UInt32 m_Brand;
	UInt32 m_TrackType;
	UInt32 m_TrackID;
	UInt32 m_ReferID;
	vector<size_t> m_SampleDur;
	vector<size_t> m_SampleSize;
	vector<size_t> m_ChunkCapacity;
	vector<size_t> m_ChunkOffset;
	vector<CSample> m_VideoSamples;
	vector<CSample> m_AudioSamples;
	vector<CSample> m_VideoHint;
	vector<CSample> m_AudioHint;
	CUdp m_Udp;
};


#endif
