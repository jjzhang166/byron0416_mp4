#ifndef MP4_H
#define MP4_H


#include <map>
#include "Log.h"
#include "Rtp.h"


using std::map;

typedef uint16_t     UInt16;
typedef unsigned int UInt32;
typedef size_t       UInt64;

#define MKTYPE(a, b, c, d) ((a) | (b<<8) | (c<<16) | (d<<24))

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

class CTrack
{
public:
	CTrack();
public:
	UInt32 m_ID;
	UInt32 m_Type;
	UInt32 m_Timescale;
	UInt32 m_Refer; //For hint.
	UInt32 m_SSRC; //For hint.
	string m_Sdp; //For hint.
	vector<size_t> m_SampleDur; //temp
	vector<size_t> m_SampleSize; //temp
	vector<size_t> m_ChunkCapacity; //temp
	vector<size_t> m_ChunkOffset; //temp
	vector<CSample> m_Samples;
	size_t m_SampleReaded;
};

class CMp4Demuxer: public CRtpSource, public CLogger
{
public:
	CMp4Demuxer();
	~CMp4Demuxer();
	bool Parse(const string&);
	void Close();
	/** CRtpSource */
	size_t GetDuration();
	size_t GetTrackID(vector<size_t>&);
	string GetSdp(size_t);
	bool GetRtpSample(size_t, CRtpSample&);
private:
	bool Read16BE(UInt16&);
	bool Read32BE(UInt32&);
	bool ParseDefault(Atom, CTrack*);
	bool ParseAtom(Atom, CTrack*);
	bool ParseFtyp(Atom, CTrack*);
	bool ParseMvhd(Atom, CTrack*);
	bool ParseTrak(Atom, CTrack*);
	bool ParseTkhd(Atom, CTrack*);
	bool ParseMdhd(Atom, CTrack*);
	bool ParseHdlr(Atom, CTrack*);
	bool ParseStts(Atom, CTrack*);
	bool ParseStsz(Atom, CTrack*);
	bool ParseStsc(Atom, CTrack*);
	bool ParseStco(Atom, CTrack*);
	bool ParseStss(Atom, CTrack*);
	bool ParseHint(Atom, CTrack*);
	bool ParseSdp (Atom, CTrack*);
private:
	int m_Fd;
	size_t m_Duration;
	map<size_t, CTrack> m_Tracks;
};


#endif
