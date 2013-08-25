#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h> //htonl
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
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
	m_SampleReaded(0)
{
}


/** CMp4Demuxer */

CMp4Demuxer::CMp4Demuxer():
	m_Fd(-1)
{
}

CMp4Demuxer::~CMp4Demuxer()
{
	if(m_Fd > 0)
		close(m_Fd);
}

bool CMp4Demuxer::Parse(const string &path)
{
	Atom atom = {0, 0, 0};
	struct stat st;

	if(0 == stat(path.c_str(), &st))
		atom.size = st.st_size;
	else
		return false;

	m_Fd = open(path.c_str(), O_RDONLY|O_LARGEFILE|O_NONBLOCK, 0);
	if(m_Fd > 0)
	{
		return ParseDefault(atom, NULL);
	}
	else
	{
		LOG_ERROR("Failed to open " << path << "(" << strerror(errno) << ").");

		return false;
	}
}

size_t CMp4Demuxer::GetHintID(vector<size_t> &ids)
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

bool CMp4Demuxer::GetRtpSample(size_t id, CRtpSample &rtp)
{
	map<size_t, CTrack>::iterator iter = m_Tracks.find(id);

	if(iter != m_Tracks.end())
	{
		CTrack &trk = iter->second;

		if(trk.m_SampleReaded < trk.m_Samples.size())
		{
			CSample &sample = trk.m_Samples[trk.m_SampleReaded];

			rtp.m_Timestamp = sample.m_Timestamp * 1000 / trk.m_Timescale;
			lseek(m_Fd, sample.m_Offset, SEEK_SET);
			LOG_TRACE("Get rtp sample from track " << id << ": st/" << sample.m_Timestamp << " dur/" << sample.m_Duration << " offset/" << sample.m_Offset << " len/" << sample.m_Len << ".");

			UInt16 count=0;
			UInt16 reserved = 0;
			Read16BE(count);
			Read16BE(reserved);
			LOG_TRACE(" This sample have " << count << " rtp packets.");

			for(size_t i=0; i<count; i++)
			{
				CRtpPacket packet;
				char *data = packet.m_Packet;
				CPacketEntry entry;
				size_t offset = 0;

				read(m_Fd, &entry, sizeof(CPacketEntry));
				entry.m_Count = ntohs(entry.m_Count);
				LOG_TRACE("  This packet have " << entry.m_Count << " entries.");

				if(entry.m_Header[0] != 0x80)
				{
					//LOG_WARN("Get a unknown version rtp packet with " << std::hex << int(entry.m_Header[0]));
					entry.m_Header[0] = 0x80;
				}

				assert(entry.m_Flags == 0);
				if(entry.m_Flags & 0x00000004)
				{
					UInt32 len;
					Read32BE(len);
					lseek(m_Fd, len-4, SEEK_CUR); // Skip extra data.
					LOG_DEBUG("This packet have " << len << " bytes extra data.");
				}

				// Copy rtp header
				memcpy(data+offset, &entry.m_Header, 4);
				offset += 4;
				// Copy rtp timestamp.
				UInt32 ts = htonl(rtp.m_Timestamp);
				memcpy(data+offset, &ts, 4);
				offset += 4;
				// Copy rtp SSRC.
				memcpy(data+offset, &trk.m_SSRC, 4);
				offset += 4;

				for(size_t n=0; n<entry.m_Count; n++)
				{
					char type = 0;

					read(m_Fd, &type, 1);
					LOG_TRACE("   The type of this entry is " << int(type) << ".");

					if(type == 0)
					{
						assert(false);
					}
					else if(type == 1)
					{
						char pad[14];
						char cn;

						read(m_Fd, &cn, 1);
						read(m_Fd, data+offset, cn);
						offset += cn;
						read(m_Fd, pad, 14-cn);
						LOG_TRACE("    Copy " << int(cn) << " bytes.");
					}
					else if(type == 2)
					{
						char index = 0;
						UInt16 len;
						UInt32 num;
						UInt32 off;
						UInt32 skip; // Ignore.

						read(m_Fd, &index, 1);
						Read16BE(len);
						Read32BE(num);
						Read32BE(off);
						read(m_Fd, &skip, 4);
						if(index == 0)
						{
							map<size_t, CTrack>::iterator iter = m_Tracks.find(trk.m_Refer);
							off += iter->second.m_Samples[num-1].m_Offset;
						}
						else
							off += trk.m_Samples[num-1].m_Offset;
						pread(m_Fd, data+offset, len, off);
						offset += len;
						LOG_TRACE("    Copy " << len << " bytes form sample " << num-1 << " with offset " << off << " in track " << int(index) << ".");
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
		}
		else
			return false;
	}
	else
		return false;

	return true;
}

inline ssize_t CMp4Demuxer::Read16BE(UInt16 &value)
{
	ssize_t n = read(m_Fd, &value, sizeof(value));
	value = ntohs(value);

	return n;
}

inline ssize_t CMp4Demuxer::Read32BE(UInt32 &value)
{
	ssize_t n = read(m_Fd, &value, sizeof(value));
	value = ntohl(value);

	return n;
}

bool CMp4Demuxer::ParseDefault(Atom atom, CTrack *track)
{
	size_t readed = 0;

	while(readed < atom.size)
	{
		Atom a;
		UInt32 size = 0;

		ssize_t ret = Read32BE(size); //4
		a.size = size;
		if(ret == 4)
		{
			read(m_Fd, &a.type, sizeof(a.type));
			a.size -= 8;
			readed += 8;

			if(a.size == 1)
			{
				if(8 == read(m_Fd, &a.size, 8)) //8
				{
					a.size -= 8;
					readed += 8;
				}
				else
					return false;
			}

			LOG_DEBUG("Read atom " << PRTYPE(a.type) << " with size " << a.size << ".");

			a.offset = atom.offset + readed;
			if(true == ParseAtom(a, track))
			{
				readed += a.size;
				lseek(m_Fd, atom.offset+readed, SEEK_SET);
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
				ret = true;
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

	int ret = read(m_Fd, &brand, 4);
	if(ret == 4)
	{
		if(brand == MKTYPE('m', 'p', '4', '2'))
		{
			LOG_DEBUG("Major brand mp42.");

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

bool CMp4Demuxer::ParseTrak(Atom atom, CTrack *track)
{
	CTrack trk;

	if(false == ParseDefault(atom, &trk))
		return false;

	size_t ts = 0;
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

		ccapacity--;
		if(ccapacity == 0)
		{
			cidx++;
			coffset = trk.m_ChunkOffset[cidx];
			if(cidx < trk.m_ChunkCapacity.size())
				ccapacity = trk.m_ChunkCapacity[cidx];
			else
				ccapacity = trk.m_ChunkCapacity[trk.m_ChunkCapacity.size()-1];
			offset = 0;
		}

		ts += trk.m_SampleDur[i];

		trk.m_Samples.push_back(sample);
		LOG_TRACE("Find sample " << i << ": st/" << sample.m_Timestamp << " dur/" << sample.m_Duration << " offset/" << sample.m_Offset << " len/" << sample.m_Len);
	}

	m_Tracks.insert(std::pair<size_t, CTrack>(trk.m_ID, trk));

	return true;
}

bool CMp4Demuxer::ParseTkhd(Atom atom, CTrack *track)
{
	UInt32 version;

	Read32BE(version);
	version = version >> 24;

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

	Read32BE(track->m_ID);
	LOG_DEBUG("Get track id " << track->m_ID << ".");

	return true;
}

bool CMp4Demuxer::ParseMdhd(Atom atom, CTrack *track)
{
	UInt32 version;

	Read32BE(version);
	version = version >> 24;

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

	Read32BE(track->m_Timescale);
	LOG_DEBUG("Get timescale " << track->m_Timescale << ".");

	return true;
}

bool CMp4Demuxer::ParseHdlr(Atom atom, CTrack *track)
{
	UInt32 version;
	UInt32 preDefined;

	read(m_Fd, &version, 4);
	read(m_Fd, &preDefined, 4);
	read(m_Fd, &track->m_Type, 4);

	LOG_DEBUG("Track for " << PRTYPE(track->m_Type));

	return true;
}

bool CMp4Demuxer::ParseStts(Atom atom, CTrack *track)
{
	UInt32 version;
	UInt32 count;

	read(m_Fd, &version, 4);
	Read32BE(count);
	LOG_DEBUG("\tSTTS: Get " << count << " entries.");

	for(UInt32 i=0; i<count; i++)
	{
		UInt32 n;
		UInt32 delta;

		Read32BE(n);
		Read32BE(delta);
		LOG_TRACE("\tSTTS: " << n << " samples with the same delta " << delta << ".");

		/** Save information */
		for(UInt32 j=0; j<n; j++)
			track->m_SampleDur.push_back(delta);
	}

	return true;
}

bool CMp4Demuxer::ParseStsz(Atom atom, CTrack *track)
{
	UInt32 version;
	UInt32 size;
	UInt32 count;

	read(m_Fd, &version, 4);
	Read32BE(size);
	Read32BE(count);

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
			Read32BE(size);
			LOG_TRACE("\tSTSZ: Sample " << i << " with size " << size << ".");

			track->m_SampleSize.push_back(size);
		}
	}

	return true;
}

bool CMp4Demuxer::ParseStsc(Atom atom, CTrack *track)
{
	UInt32 version;
	UInt32 count;

	read(m_Fd, &version, 4);
	Read32BE(count);

	for(UInt32 i=0; i<count; i++)
	{
		UInt32 firstChunk;
		UInt32 samplesPerChunk;
		UInt32 sampleDescriptionIndex;

		Read32BE(firstChunk);
		Read32BE(samplesPerChunk);
		Read32BE(sampleDescriptionIndex);
		LOG_TRACE("\tSTSC: first " << firstChunk << ", per " << samplesPerChunk << ", index " << sampleDescriptionIndex << ".");

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

	read(m_Fd, &version, 4);
	Read32BE(count);
	LOG_DEBUG("\tSTCO: Get " << count << " chunks.");

	for(UInt32 i=0; i<count; i++)
	{
		UInt32 offset;

		Read32BE(offset);
		LOG_TRACE("\tSTCO: Chunk " << i+1 << " with offset " << offset << ".");

		track->m_ChunkOffset.push_back(offset);
	}

	return true;
}

bool CMp4Demuxer::ParseStss(Atom atom, CTrack *track)
{
	UInt32 version;
	UInt32 count;

	read(m_Fd, &version, 4);
	Read32BE(count);

	for(UInt32 i=0; i<count; i++)
	{
		UInt32 num;

		Read32BE(num);
		LOG_TRACE("\tSTSS: There is a I Frame with sample number " << num << ".");
	}

	return true;
}

bool CMp4Demuxer::ParseHint(Atom atom, CTrack *track)
{
	Read32BE(track->m_Refer);
	LOG_DEBUG("Find a hint track for track " << track->m_Refer << ".");

	return true;
}

bool CMp4Demuxer::ParseSdp(Atom atom, CTrack *track)
{
	const size_t LEN = 2048;
	char buf[LEN] = {0};

	read(m_Fd, buf, LEN);
	track->m_Sdp = buf;
	LOG_DEBUG("\tSDP: " << buf);

	return true;
}
