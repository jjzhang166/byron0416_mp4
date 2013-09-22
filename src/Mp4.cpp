#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h> //rand
#include <arpa/inet.h> //ntohs, ntohl, htonl
#include "Mp4.h"


#define PRTYPE(type) ((char*)&type)[0] << ((char*)&type)[1] << ((char*)&type)[2] << ((char*)&type)[3]

#pragma pack(1)
struct CPacketEntry
{
	UInt32 m_RelativeTime;
	unsigned char m_Header[4];
	UInt16 m_Flags;
	UInt16 m_Count;
};
#pragma pack()


/** CTrack */

CTrack::CTrack():
	m_ID(0), m_Type(0), m_Timescale(0), m_Key(false), m_Refer(0), m_SSRC(rand()), m_SampleReaded(0)
{
}


/** CMp4Demuxer */

CMp4Demuxer::CMp4Demuxer():
	m_Fd(-1)
{
}

CMp4Demuxer::~CMp4Demuxer()
{
	Close();
}

bool CMp4Demuxer::Parse(const string &file)
{
	Atom atom = {0, 0, 0};
	struct stat st;

	if(0 == stat(file.c_str(), &st))
		atom.size = st.st_size;
	else
		return false;

	m_Fd = open(file.c_str(), O_RDONLY|O_LARGEFILE|O_NONBLOCK, 0);
	if(m_Fd > 0)
	{
		return ParseDefault(atom, NULL);
	}
	else
	{
		LOG_ERROR("Failed to open " << file << "(" << strerror(errno) << ").");

		return false;
	}
}

void CMp4Demuxer::Close()
{
	if(m_Fd > 0)
	{
		close(m_Fd);
		m_Fd = -1;
	}
	m_Tracks.clear();
}

size_t CMp4Demuxer::GetDuration()
{
	return m_Duration;
}

size_t CMp4Demuxer::GetTrackID(vector<size_t> &ids)
{
	map<size_t, CTrack>::iterator iter;

	for(iter=m_Tracks.begin(); iter!=m_Tracks.end(); iter++)
	{
		CTrack &track = iter->second;

		if(track.m_Type == MKTYPE('h', 'i', 'n', 't'))
			ids.push_back(track.m_ID);
	}

	return ids.size();
}

string CMp4Demuxer::GetSdp(size_t id)
{
	map<size_t, CTrack>::iterator iter = m_Tracks.find(id);

	if(iter != m_Tracks.end())
		return iter->second.m_Sdp;
	else
		return "";
}

bool CMp4Demuxer::GetRtpSample(size_t id, CRtpSample &rtp)
{
	map<size_t, CTrack>::iterator iter = m_Tracks.find(id);

	if(iter != m_Tracks.end())
	{
		CTrack &trk = iter->second;

		if(trk.m_SampleReaded < trk.m_Samples.size())
		{
			CSample &sample = trk.m_Samples[trk.m_SampleReaded];
			UInt16 count=0;
			UInt16 reserved = 0;

			//LOG_TRACE("Get rtp sample from track-" << id << ": st/" << sample.m_Timestamp << " dur/" << sample.m_Duration << " offset/" << sample.m_Offset << " len/" << sample.m_Len << ".");

			rtp.m_Timestamp = sample.m_Timestamp * 1000 / trk.m_Timescale;

			if(-1 == lseek(m_Fd, sample.m_Offset, SEEK_SET))
				return false;
			if(false == Read16BE(count))
				return false;
			if(false == Read16BE(reserved))
				return false;
			//LOG_TRACE(" This sample have " << count << " rtp packets.");

			for(size_t i=0; i<count; i++)
			{
				CRtpPacket packet;
				char *data = packet.m_Packet;
				CPacketEntry entry;
				size_t offset = 0;

				if(sizeof(CPacketEntry) != read(m_Fd, &entry, sizeof(CPacketEntry)))
					return false;
				entry.m_Count = ntohs(entry.m_Count);
				//LOG_TRACE("  This packet have " << entry.m_Count << " entries.");

				if(entry.m_Header[0] != 0x80)
				{
					//LOG_WARN("Get a unknown version 0x" << std::hex << int(entry.m_Header[0]) << " rtp packet.");
					entry.m_Header[0] = 0x80;
				}

				if(entry.m_Flags & 0x00000004) // Skip extra data.
				{
					UInt32 len;
					if(false == Read32BE(len))
						return false;
					if(-1 == lseek(m_Fd, len-4, SEEK_CUR))
						return false;
					//LOG_TRACE("This packet have " << len << " bytes extra data.");
				}

				// Copy rtp header
				memcpy(data+offset, &entry.m_Header, 4);
				offset += 4;
				// Copy rtp timestamp.
				UInt32 ts = htonl(sample.m_Timestamp);
				memcpy(data+offset, &ts, 4);
				offset += 4;
				// Copy rtp SSRC.
				memcpy(data+offset, &trk.m_SSRC, 4);
				offset += 4;

				for(size_t n=0; n<entry.m_Count; n++)
				{
					char type = 0;

					if(1 != read(m_Fd, &type, 1))
						return false;
					//LOG_TRACE("   The type of this entry is " << int(type) << ".");

					if(type == 0)
					{
						assert(false);
					}
					else if(type == 1)
					{
						char pad[14];
						char cn;

						if(1 != read(m_Fd, &cn, 1))
							return false;
						if(cn != read(m_Fd, data+offset, cn))
							return false;
						offset += cn;
						if(14-cn != read(m_Fd, pad, 14-cn))
							return false;
						//LOG_TRACE("    Copy " << int(cn) << " bytes.");
					}
					else if(type == 2)
					{
						char index = 0;
						UInt16 len;
						UInt32 num;
						UInt32 off;
						UInt32 skip; // Ignore.

						if(1 != read(m_Fd, &index, 1))
							return false;
						if(false == Read16BE(len))
							return false;
						if(false == Read32BE(num))
							return false;
						if(false == Read32BE(off))
							return false;
						if(false == Read32BE(skip))
							return false;
						if(index == 0)
						{
							map<size_t, CTrack>::iterator iter = m_Tracks.find(trk.m_Refer);
							if(iter != m_Tracks.end())
								off += iter->second.m_Samples[num-1].m_Offset;
							else
								return false;
						}
						else
							off += trk.m_Samples[num-1].m_Offset;
						if(len != pread(m_Fd, data+offset, len, off))
							return false;
						offset += len;
						//LOG_TRACE("    Copy " << len << " bytes form sample " << num-1 << " with offset " << off << " in track " << int(index) << ".");
					}
					else if(type == 3)
					{
						assert(false);

						return false;
					}
					else
					{
						assert(false);

						return false;
					}
				}
				packet.m_Len = offset;
				rtp.m_Packets.push_back(packet);
			}
			trk.m_SampleReaded ++;

			return true;
		}
		else
			return true; // The end.
	}
	else
		return false;
}

inline bool CMp4Demuxer::Read16BE(UInt16 &value)
{
	ssize_t n = read(m_Fd, &value, sizeof(value));
	value = ntohs(value);

	return n == sizeof(value);
}

inline bool CMp4Demuxer::Read32BE(UInt32 &value)
{
	ssize_t n = read(m_Fd, &value, sizeof(value));
	value = ntohl(value);

	return n == sizeof(value);
}

bool CMp4Demuxer::ParseDefault(Atom atom, CTrack *track)
{
	size_t readed = 0;

	while(readed < atom.size)
	{
		Atom a;
		UInt32 size = 0;

		if(true == Read32BE(size))
		{
			a.size = size;

			if(a.size == 1) //Ignore the large file.
			{
				LOG_WARN("Cannot parse large mp4 file.");

				return false;
			}

			if(sizeof(a.type) != read(m_Fd, &a.type, sizeof(a.type)))
				return false;

			LOG_DEBUG("Read atom " << PRTYPE(a.type) << " with size " << a.size << ".");

			a.size -= 8;
			readed += 8;
			a.offset = atom.offset + readed;
			if(true == ParseAtom(a, track))
			{
				readed += a.size;
				if(-1 == lseek(m_Fd, atom.offset+readed, SEEK_SET))
					return false;
			}
			else
				return false;
		}
		else
			return false;
	}

	return true;
}

bool CMp4Demuxer::ParseAtom(Atom atom, CTrack *track)
{
	bool ret = false;

	switch(atom.type)
	{
		case MKTYPE('f', 't', 'y', 'p'):
			{
				ret = ParseFtyp(atom, track);
				break;
			}
		case MKTYPE('m', 'o', 'o', 'v'):
			{
				ret = ParseDefault(atom, track);
				break;
			}
		case MKTYPE('m', 'v', 'h', 'd'):
			{
				ret = ParseMvhd(atom, track);
				break;
			}
		case MKTYPE('t', 'r', 'a', 'k'):
			{
				ret = ParseTrak(atom, track);
				break;
			}
		case MKTYPE('t', 'k', 'h', 'd'):
			{
				ret = ParseTkhd(atom, track);
				break;
			}
		case MKTYPE('m', 'd', 'i', 'a'):
			{
				ret = ParseDefault(atom, track);
				break;
			}
		case MKTYPE('m', 'd', 'h', 'd'):
			{
				ret = ParseMdhd(atom, track);
				break;
			}
		case MKTYPE('h', 'd', 'l', 'r'):
			{
				ret = ParseHdlr(atom, track);
				break;
			}
		case MKTYPE('m', 'i', 'n', 'f'):
			{
				ret = ParseDefault(atom, track);
				break;
			}
		case MKTYPE('v', 'm', 'h', 'd'):
			{
				ret = true;
				break;
			}
		case MKTYPE('s', 'm', 'h', 'd'):
			{
				ret = true;
				break;
			}
		case MKTYPE('d', 'i', 'n', 'f'):
			{
				ret = ParseDefault(atom, track);
				break;
			}
		case MKTYPE('d', 'r', 'e', 'f'):
			{
				ret = true;
				break;
			}
		case MKTYPE('s', 't', 'b', 'l'):
			{
				ret = ParseDefault(atom, track);
				break;
			}
		case MKTYPE('s', 't', 's', 'd'):
			{
				ret = true;
				break;
			}
		case MKTYPE('s', 't', 't', 's'):
			{
				ret = ParseStts(atom, track);
				break;
			}
		case MKTYPE('s', 't', 's', 'z'):
			{
				ret = ParseStsz(atom, track);
				break;
			}
		case MKTYPE('s', 't', 's', 'c'):
			{
				ret = ParseStsc(atom, track);
				break;
			}
		case MKTYPE('s', 't', 'c', 'o'):
			{
				ret = ParseStco(atom, track);
				break;
			}
		case MKTYPE('s', 't', 's', 's'):
			{
				ret = ParseStss(atom, track);
				break;
			}
		case MKTYPE('e', 'd', 't', 's'):
			{
				ret = ParseDefault(atom, track);
				break;
			}
		case MKTYPE('e', 'l', 's', 't'):
			{
				ret = true;
				break;
			}
		case MKTYPE('h', 'm', 'h', 'd'):
			{
				ret = true;
				break;
			}
		case MKTYPE('t', 'r', 'e', 'f'):
			{
				ret = ParseDefault(atom, track);
				break;
			}
		case MKTYPE('h', 'i', 'n', 't'):
			{
				ret = ParseHint(atom, track);
				break;
			}
		case MKTYPE('u', 'd', 't', 'a'):
			{
				ret = ParseDefault(atom, track);
				break;
			}
		case MKTYPE('h', 'n', 't', 'i'):
			{
				ret = ParseDefault(atom, track);
				break;
			}
		case MKTYPE('s', 'd', 'p', ' '):
			{
				ret = ParseSdp(atom, track);
				break;
			}
		case MKTYPE('m', 'e', 't', 'a'):
			{
				ret = true;
				break;
			}
		case MKTYPE('m', 'd', 'a', 't'):
			{
				ret = true;
				break;
			}
		case MKTYPE('f', 'r', 'e', 'e'):
			{
				ret = true;
				break;
			}
		default:
			{
				ret = true;
				LOG_DEBUG("Unknown atom " << PRTYPE(atom.type) << ".");
			}
	}

	return ret;
}

bool CMp4Demuxer::ParseFtyp(Atom atom, CTrack *track)
{
	UInt32 brand;

	int ret = read(m_Fd, &brand, sizeof(brand));
	if(ret == sizeof(brand))
	{
		if(brand == MKTYPE('m', 'p', '4', '2'))
		{
			LOG_DEBUG("\tMajor brand mp42.");

			return true;
		}
		else
		{
			LOG_WARN("Unknown major brand " << PRTYPE(brand));

			return false;
		}
	}
	else
		return false;
}

bool CMp4Demuxer::ParseMvhd(Atom atom, CTrack *track)
{
	UInt32 version;

	if(true == Read32BE(version))
		version = version >> 24;
	else
		return false;

	if(version == 1)
	{
		/*
		UInt64 create;
		UInt64 modify;
		UInt32 timescale;
		UInt64 duration;

		if(sizeof(create) != read(fd, &create, sizeof(create))
			return false;
		if(sizeof(modify) != read(fd, &modify, sizeof(modify))
			return false;
		*/

		assert(false);
		return false;
	}
	else if(version == 0)
	{
		UInt32 create;
		UInt32 modify;
		UInt32 timescale;
		UInt32 duration;

		if(false == Read32BE(create))
			return false;
		if(false == Read32BE(modify))
			return false;
		if(false == Read32BE(timescale))
			return false;
		if(false == Read32BE(duration))
			return false;

		m_Duration = duration * 1000 / timescale;
	}

	return true;
}

bool CMp4Demuxer::ParseTrak(Atom atom, CTrack *track)
{
	CTrack trk;

	if(false == ParseDefault(atom, &trk))
		return false;

	size_t ts = 0; // Timestamp
	size_t cidx = 0;
	size_t coffset = trk.m_ChunkOffset[cidx];
	size_t ccapacity = trk.m_ChunkCapacity[cidx];
	size_t offset = 0;

	for(size_t i=0; i<trk.m_SampleDur.size(); i++)
	{
		CSample sample;

		sample.m_Timestamp = ts;
		sample.m_Duration = trk.m_SampleDur[i];
		sample.m_Offset = coffset + offset;
		sample.m_Len = trk.m_SampleSize[i];

		offset += sample.m_Len;
		ts += trk.m_SampleDur[i];

		ccapacity--;
		if(ccapacity == 0)
		{
			cidx++;
			if(cidx < trk.m_ChunkOffset.size()) //cidx will be overflow when push back the latest sample.
				coffset = trk.m_ChunkOffset[cidx];
			if(cidx < trk.m_ChunkCapacity.size())
				ccapacity = trk.m_ChunkCapacity[cidx];
			else
				ccapacity = trk.m_ChunkCapacity[trk.m_ChunkCapacity.size()-1];
			offset = 0;
		}

		trk.m_Samples.push_back(sample);
		//LOG_TRACE("Find sample " << i << ": st/" << sample.m_Timestamp << " dur/" << sample.m_Duration << " offset/" << sample.m_Offset << " len/" << sample.m_Len);
	}

	m_Tracks.insert(std::pair<size_t, CTrack>(trk.m_ID, trk));

	return true;
}

bool CMp4Demuxer::ParseTkhd(Atom atom, CTrack *track)
{
	UInt32 version;

	if(true == Read32BE(version))
		version = version >> 24;
	else
		return false;

	if(version == 1)
	{
		UInt64 create;
		UInt64 modify;

		read(m_Fd, &create, 8);
		read(m_Fd, &modify, 8);
	}
	else
	{
		UInt32 create;
		UInt32 modify;

		Read32BE(create);
		Read32BE(modify);
	}

	if(true == Read32BE(track->m_ID))
	{
		LOG_DEBUG("\tGet track with id " << track->m_ID << ".");
	}
	else
		return false;

	return true;
}

bool CMp4Demuxer::ParseMdhd(Atom atom, CTrack *track)
{
	UInt32 version;

	if(true == Read32BE(version))
		version = version >> 24;
	else
		return false;

	if(version == 1)
	{
		UInt64 create;
		UInt64 modify;

		read(m_Fd, &create, 8);
		read(m_Fd, &modify, 8);
	}
	else
	{
		UInt32 create;
		UInt32 modify;

		read(m_Fd, &create, 4);
		read(m_Fd, &modify, 4);
	}

	if(true == Read32BE(track->m_Timescale))
	{
		LOG_DEBUG("\tGet timescale " << track->m_Timescale << ".");
	}
	else
		return false;

	return true;
}

bool CMp4Demuxer::ParseHdlr(Atom atom, CTrack *track)
{
	UInt32 version;
	UInt32 preDefined;

	if(4 != read(m_Fd, &version, 4))
		return false;
	if(4 != read(m_Fd, &preDefined, 4))
		return false;
	if(4 != read(m_Fd, &track->m_Type, 4))
		return false;

	LOG_DEBUG("\tTrack for " << PRTYPE(track->m_Type));

	return true;
}

bool CMp4Demuxer::ParseStts(Atom atom, CTrack *track)
{
	UInt32 version;
	UInt32 count;

	if(false == Read32BE(version))
		return false;

	if(true == Read32BE(count))
	{
		LOG_DEBUG("\tSTTS: Get " << count << " entries.");

		for(UInt32 i=0; i<count; i++)
		{
			UInt32 n;
			UInt32 delta;

			if(false == Read32BE(n))
				return false;
			if(false == Read32BE(delta))
				return false;
			//LOG_TRACE("\tSTTS: " << n << " samples with the same delta " << delta << ".");

			/** Save information */
			for(UInt32 j=0; j<n; j++)
				track->m_SampleDur.push_back(delta);
		}

		return true;
	}
	else
		return false;
}

bool CMp4Demuxer::ParseStsz(Atom atom, CTrack *track)
{
	UInt32 version;
	UInt32 size;
	UInt32 count;

	if(false == Read32BE(version))
		return false;
	if(false == Read32BE(size))
		return false;
	if(false == Read32BE(count))
		return false;

	if(size != 0)
	{
		LOG_TRACE("\tSTSZ: " << count << " samples with the same size " << size << ".");
		for(size_t i=0; i<count; i++)
			track->m_SampleSize.push_back(size);
	}
	else
	{
		for(UInt32 i=0; i<count; i++)
		{
			if(false == Read32BE(size))
				return false;
			//LOG_TRACE("\tSTSZ: Sample " << i << " with size " << size << ".");

			track->m_SampleSize.push_back(size);
		}
	}

	return true;
}

bool CMp4Demuxer::ParseStsc(Atom atom, CTrack *track)
{
	UInt32 version;
	UInt32 count;

	if(false == Read32BE(version))
		return false;
	if(false == Read32BE(count))
		return false;

	for(UInt32 i=0; i<count; i++)
	{
		UInt32 firstChunk;
		UInt32 samplesPerChunk;
		UInt32 sampleDescriptionIndex;

		if(false == Read32BE(firstChunk))
			return false;
		if(false == Read32BE(samplesPerChunk))
			return false;
		if(false == Read32BE(sampleDescriptionIndex))
			return false;
		//LOG_TRACE("\tSTSC: first " << firstChunk << ", per " << samplesPerChunk << ", index " << sampleDescriptionIndex << ".");

		if(track->m_ChunkCapacity.empty() == true)
			track->m_ChunkCapacity.push_back(samplesPerChunk);
		else
		{
			size_t num = track->m_ChunkCapacity[track->m_ChunkCapacity.size()-1];

			while(track->m_ChunkCapacity.size() < firstChunk-1)
				track->m_ChunkCapacity.push_back(num);

			track->m_ChunkCapacity.push_back(samplesPerChunk);
		}
	}

	return true;
}

bool CMp4Demuxer::ParseStco(Atom atom, CTrack *track)
{
	UInt32 version;
	UInt32 count;

	if(false == Read32BE(version))
		return false;
	if(false == Read32BE(count))
		return false;

	LOG_DEBUG("\tSTCO: Get " << count << " chunks.");

	for(UInt32 i=0; i<count; i++)
	{
		UInt32 offset;

		if(false == Read32BE(offset))
			return false;

		//LOG_TRACE("\tSTCO: Chunk " << i+1 << " with offset " << offset << ".");

		track->m_ChunkOffset.push_back(offset);
	}

	return true;
}

bool CMp4Demuxer::ParseStss(Atom atom, CTrack *track)
{
	UInt32 version;
	UInt32 count;

	if(false == Read32BE(version))
		return false;
	if(false == Read32BE(count))
		return false;

	for(UInt32 i=0; i<count; i++)
	{
		UInt32 num;

		if(false == Read32BE(num))
			return false;
		else
		{
			track->m_Keys.push_back(num);
			//LOG_TRACE("\tSTSS: There is a I Frame with sample number " << num << ".");
		}
	}

	track->m_Key = true;

	return true;
}

bool CMp4Demuxer::ParseHint(Atom atom, CTrack *track)
{
	if(false == Read32BE(track->m_Refer))
		return false;
	LOG_DEBUG("\tFind a hint track for track " << track->m_Refer << ".");

	return true;
}

bool CMp4Demuxer::ParseSdp(Atom atom, CTrack *track)
{
	const size_t LEN = 2048;
	char buf[LEN] = {0};

	if(0 < read(m_Fd, buf, LEN))
	{
		track->m_Sdp = buf;
		LOG_DEBUG("\tSDP: " << buf);

		return true;
	}
	else
		return false;
}
