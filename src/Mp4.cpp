#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "Mp4.h"


CMp4Demuxer::CMp4Demuxer()
{
}

bool CMp4Demuxer::Parse(const string &path)
{
	Atom atom = {0, 0, 0};
	struct stat st;

	if(0 == stat(path.c_str(), &st))
		atom.size = st.st_size;
	else
		return false;

	int fd = open(path.c_str(), O_RDONLY|O_LARGEFILE|O_NONBLOCK, 0);
	if(fd > 0)
	{
		bool ret = ParseDefault(fd, atom);
		close(fd);
		if(ret == false)
			return false;
		else
			return true;
	}
	else
	{
		LOG_ERROR("Failed to open " << path << "(" << strerror(errno) << ").");
		return false;
	}
}

void CMp4Demuxer::Send(const string &path)
{
	int fd = open(path.c_str(), O_RDONLY|O_LARGEFILE|O_NONBLOCK, 0);
	vector<CSample> &s = m_VideoHint;
	for(size_t i=0; i<s.size(); i++)
	{
		CSample sample = s[i];
		lseek(fd,  sample.m_Offset, SEEK_SET);

		LOG_TRACE("Get rtp sample: st/" << sample.m_Timestamp << " dur/" << sample.m_Duration << " offset/" << sample.m_Offset << " len/" << sample.m_Len);

		UInt16 count, reserved;
		Read16BE(fd, count);
		Read16BE(fd, reserved);
		LOG_TRACE("This sample have " << count << " rtp packets.");
		for(size_t j=0; j<count; j++)
		{
			CPacketEntry e;

			read(fd, &e, sizeof(CPacketEntry));
			e.m_Count = htons(e.m_Count);
			LOG_TRACE("This packet have " << count << " entry.");

			if(e.m_Flags & 0x00000004)
			{
				LOG_TRACE("This entry have " << count << " bytes extra data.");
				UInt32 len;
				Read32BE(fd, len);
				lseek(fd, len-4, SEEK_CUR);
			}

			int type;
			read(fd, &type, 1);
			LOG_TRACE("This entry type " << type << ".");
			if(type == 0)
			{
			}
			else if(type == 1)
			{
			}
			else if(type == 2)
			{
				char index;
				UInt16 len;
				UInt32 num;
				UInt32 offset;
				UInt32 skip;

				read(fd, &index, 1);
				read(fd, &len, 2);
				read(fd, &num, 4);
				read(fd, &offset, 4);
				read(fd, &skip, 4);
			}
			else if(type == 3)
			{
			}
			else
			{
			}
		}
	}
	close(fd);
}

ssize_t CMp4Demuxer::Read16BE(int fd, UInt16 &value)
{
	ssize_t n = read(fd, &value, 2);
	value = htons(value);

	return n;
}

ssize_t CMp4Demuxer::Read16BE(int fd, UInt16 &value, size_t offset)
{
	ssize_t n = pread(fd, &value, 2, offset);
	value = htons(value);

	return n;
}

ssize_t CMp4Demuxer::Read32BE(int fd, UInt32 &value)
{
	ssize_t n = read(fd, &value, 4);
	value = htonl(value);

	return n;
}

bool CMp4Demuxer::ParseAtom(int fd, Atom atom)
{
	bool ret = false;

	switch(atom.type)
	{
		case MKTYPE('f', 't', 'y', 'p'):
			{
				ret = ParseFtyp(fd, atom);
				break;
			}
		case MKTYPE('m', 'o', 'o', 'v'):
			{
				ret = ParseDefault(fd, atom);
				break;
			}
		case MKTYPE('m', 'v', 'h', 'd'):
			{
				ret = true;
				break;
			}
		case MKTYPE('t', 'r', 'a', 'k'):
			{
				ret = ParseTrak(fd, atom);
				break;
			}
		case MKTYPE('t', 'k', 'h', 'd'):
			{
				ret = ParseTkhd(fd, atom);
				break;
			}
		case MKTYPE('m', 'd', 'i', 'a'):
			{
				ret = ParseDefault(fd, atom);
				break;
			}
		case MKTYPE('m', 'd', 'h', 'd'):
			{
				ret = true;
				break;
			}
		case MKTYPE('h', 'd', 'l', 'r'):
			{
				ret = ParseHdlr(fd, atom);
				break;
			}
		case MKTYPE('m', 'i', 'n', 'f'):
			{
				ret = ParseDefault(fd, atom);
				break;
			}
		case MKTYPE('v', 'm', 'h', 'd'):
			{
				ret = ParseVmhd(fd, atom);
				break;
			}
		case MKTYPE('s', 'm', 'h', 'd'):
			{
				ret = ParseSmhd(fd, atom);
				break;
			}
		case MKTYPE('d', 'i', 'n', 'f'):
			{
				ret = ParseDefault(fd, atom);
				break;
			}
		case MKTYPE('d', 'r', 'e', 'f'):
			{
				ret = ParseDref(fd, atom);
				break;
			}
		case MKTYPE('s', 't', 'b', 'l'):
			{
				ret = ParseDefault(fd, atom);
				break;
			}
		case MKTYPE('s', 't', 's', 'd'):
			{
				ret = ParseStsd(fd, atom);
				break;
			}
		case MKTYPE('s', 't', 't', 's'):
			{
				ret = ParseStts(fd, atom);
				break;
			}
		case MKTYPE('s', 't', 's', 'z'):
			{
				ret = ParseStsz(fd, atom);
				break;
			}
		case MKTYPE('s', 't', 's', 'c'):
			{
				ret = ParseStsc(fd, atom);
				break;
			}
		case MKTYPE('s', 't', 'c', 'o'):
			{
				ret = ParseStco(fd, atom);
				break;
			}
		case MKTYPE('s', 't', 's', 's'):
			{
				ret = ParseStss(fd, atom);
				break;
			}
		case MKTYPE('e', 'd', 't', 's'):
			{
				ret = ParseDefault(fd, atom);
				break;
			}
		case MKTYPE('e', 'l', 's', 't'):
			{
				ret = ParseElst(fd, atom);
				break;
			}
		case MKTYPE('h', 'm', 'h', 'd'):
			{
				ret = ParseHmhd(fd, atom);
				break;
			}
		case MKTYPE('t', 'r', 'e', 'f'):
			{
				ret = ParseDefault(fd, atom);
				break;
			}
		case MKTYPE('h', 'i', 'n', 't'):
			{
				ret = ParseHint(fd, atom);
				break;
			}
		case MKTYPE('u', 'd', 't', 'a'):
			{
				ret = ParseDefault(fd, atom);
				break;
			}
		case MKTYPE('h', 'n', 't', 'i'):
			{
				ret = ParseDefault(fd, atom);
				break;
			}
		case MKTYPE('s', 'd', 'p', ' '):
			{
				ret = ParseSdp(fd, atom);
				break;
			}
		case MKTYPE('m', 'e', 't', 'a'):
			{
				ret = ParseMeta(fd, atom);
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
				LOG_DEBUG("Unknown atom " << PRTYPE(atom.type));
			}
	}

	return ret;
}

bool CMp4Demuxer::ParseDefault(int fd, Atom atom)
{
	size_t readed = 0;

	while(readed < atom.size)
	{
		Atom a;

		ssize_t ret = read(fd, &a.size, 4);
		if(ret == 4)
		{
			a.size = htonl(a.size);
			read(fd, &a.type, sizeof(a.type));
			a.size -= 8;
			readed += 8;

			if(a.size == 1)
			{
				if(8 == read(fd, &a.size, 8))
				{
					a.size -= 8;
					readed += 8;
				}
				else
					return false;
			}

			LOG_TRACE("Read atom " << PRTYPE(a.type) << " with size " << a.size << ".");

			a.offset = atom.offset + readed;
			if(true == ParseAtom(fd, a))
			{
				readed += a.size;
				lseek(fd, atom.offset+readed, SEEK_SET);
			}
			else
				return false;
		}
		else
			return false;
	}

	return true;
}


bool CMp4Demuxer::ParseFtyp(int fd, Atom atom)
{
	int ret = read(fd, &m_Brand, 4);
	if(ret == 4)
	{
		if(m_Brand == MKTYPE('m', 'p', '4', '2'))
		{
			LOG_DEBUG("Major brand mp42.");

			return true;
		}
		else
		{
			LOG_WARN("Unknown major brand " << PRTYPE(m_Brand));

			return false;
		}
	}
	else
		return false;
}

bool CMp4Demuxer::ParseTrak(int fd, Atom a)
{
	m_TrackType = 0;
	m_TrackID = 0;
	m_ReferID = 0;
	m_SampleDur.clear();
	m_SampleSize.clear();
	m_ChunkCapacity.clear();
	m_ChunkOffset.clear();

	bool ret = ParseDefault(fd, a);

	vector<CSample> *m_Samples; 
	if(m_TrackType == MKTYPE('v', 'i', 'd', 'e'))
	{
		m_Samples = &m_VideoSamples;
	}
	else if(m_TrackType == MKTYPE('s', 'o', 'u', 'n'))
	{
		m_Samples = &m_AudioSamples;
	}
	else if(m_TrackType == MKTYPE('h', 'i', 'n', 't'))
	{
		if(m_TrackID == 3)
			m_Samples = &m_VideoHint;
		if(m_TrackID == 4)
			m_Samples = &m_AudioHint;
	}
	else
		return true;

	size_t ts = 0;
	size_t cidx = 0;
	size_t coffset = m_ChunkOffset[cidx];
	size_t ccapacity = m_ChunkCapacity[cidx];
	size_t offset = 0;

	for(size_t i=0; i<m_SampleDur.size(); i++)
	{
		CSample sample;

		sample.m_Timestamp = ts;
		sample.m_Duration = m_SampleDur[i];

		sample.m_Offset = coffset + offset;
		sample.m_Len = m_SampleSize[i];
		offset += sample.m_Len;

		ccapacity--;
		if(ccapacity == 0)
		{
			cidx++;
			coffset = m_ChunkOffset[cidx];
			if(cidx < m_ChunkCapacity.size())
				ccapacity = m_ChunkCapacity[cidx];
			else
				ccapacity = m_ChunkCapacity[m_ChunkCapacity.size()-1];
			offset = 0;
		}

		ts += m_SampleDur[i];

		m_Samples->push_back(sample);
		LOG_TRACE("Find sample " << i << ": st/" << sample.m_Timestamp << " dur/" << sample.m_Duration << hex << " offset/" << sample.m_Offset << dec << " len/" << sample.m_Len);
	}

	return ret;
}

bool CMp4Demuxer::ParseTkhd(int fd, Atom atom)
{
	UInt32 version;

	Read32BE(fd, version);
	version = version >> 24;

	if(version == 0)
	{
		UInt32 create;
		UInt32 modify;

		Read32BE(fd, create);
		Read32BE(fd, modify);
	}
	else if(version == 1)
	{
		UInt64 create;
		UInt64 modify;

		read(fd, &create, 8);
		read(fd, &modify, 8);
	}
	else
	{
		assert(false);

		return false;
	}

	Read32BE(fd, m_TrackID);
	LOG_TRACE("Get track id " << m_TrackID << ".");

	return true;
}

bool CMp4Demuxer::ParseHdlr(int fd, Atom atom)
{
	UInt32 version;
	UInt32 preDefined;

	read(fd, &version, 4);
	read(fd, &preDefined, 4);
	read(fd, &m_TrackType, 4);

	LOG_DEBUG("Track for " << PRTYPE(m_TrackType));

	return true;
}

bool CMp4Demuxer::ParseVmhd(int fd, Atom a)
{
	return true;
}

bool CMp4Demuxer::ParseSmhd(int fd, Atom a)
{
	return true;
}

bool CMp4Demuxer::ParseDref(int fd, Atom a)
{
	return true;
}

bool CMp4Demuxer::ParseStsd(int fd, Atom a)
{
	return true;
}

bool CMp4Demuxer::ParseStts(int fd, Atom a)
{
	UInt32 full;
	UInt32 count;

	read(fd, &full, 4);
	read(fd, &count, 4);
	count = htonl(count);

	for(UInt32 i=0; i<count; i++)
	{
		UInt32 count;
		UInt32 delta;

		Read32BE(fd, count);
		Read32BE(fd, delta);
		LOG_TRACE("\tSTTS: sample count " << count << ", delta " << delta << ".");

		/** Save information */
		for(size_t j=0; j<count; j++)
		{
			m_SampleDur.push_back(delta);
		}
	}

	return true;
}

bool CMp4Demuxer::ParseStsz(int fd, Atom a)
{
	UInt32 full;
	UInt32 size;
	UInt32 count;

	read(fd, &full, 4);
	read(fd, &size, 4);
	size = htonl(size);
	read(fd, &count, 4);
	count = htonl(count);

	LOG_TRACE("\tSTSZ: sample count " << count << ", sample size " << size << ".");

	if(size != 0)
	{
		for(size_t i=0; i<count; i++)
			m_SampleSize.push_back(size);
	}
	else
	{
		for(UInt32 i=0; i<count; i++)
		{
			read(fd, &size, 4);
			size = htonl(size);
			LOG_TRACE("\tSTSZ: sample " << i << " size " << size << ".");

			m_SampleSize.push_back(size);
		}
	}

	return true;
}

bool CMp4Demuxer::ParseStsc(int fd, Atom a)
{
	UInt32 full;
	UInt32 count;

	read(fd, &full, 4);
	Read32BE(fd, count);

	for(UInt32 i=0; i<count; i++)
	{
		UInt32 firstChunk;
		UInt32 samplesPerChunk;
		UInt32 sampleDescriptionIndex;

		Read32BE(fd, firstChunk);
		Read32BE(fd, samplesPerChunk);
		Read32BE(fd, sampleDescriptionIndex);
		LOG_TRACE("\tSTSC: first " << firstChunk << ". per " << samplesPerChunk << ", index " << sampleDescriptionIndex << ".");

		m_ChunkCapacity.push_back(samplesPerChunk);
	}

	return true;
}

bool CMp4Demuxer::ParseStco(int fd, Atom a)
{
	UInt32 full;
	UInt32 count;

	read(fd, &full, 4);
	Read32BE(fd, count);

	for(UInt32 i=0; i<count; i++)
	{
		UInt32 offset;

		Read32BE(fd, offset);
		LOG_TRACE("\tSTCO: chunk " << i+1 << " with offset " << hex << offset << ".");

		m_ChunkOffset.push_back(offset);
	}

	return true;
}

bool CMp4Demuxer::ParseStss(int fd, Atom a)
{
	UInt32 full;
	UInt32 count;

	read(fd, &full, 4);
	Read32BE(fd, count);

	for(UInt32 i=0; i<count; i++)
	{
		UInt32 num;

		Read32BE(fd, num);
		LOG_TRACE("\tSTSS: I Frame " << num << ".");
	}

	return true;
}

bool CMp4Demuxer::ParseElst(int fd, Atom a)
{
	return true;
}

bool CMp4Demuxer::ParseHmhd(int fd, Atom a)
{
	return true;
}

bool CMp4Demuxer::ParseHint(int fd, Atom a)
{
	Read32BE(fd, m_ReferID);
	LOG_TRACE("Find a hint track for track " << m_ReferID << ".");

	return true;
}

bool CMp4Demuxer::ParseSdp(int fd, Atom a)
{
	const size_t LEN = 2048;
	char buf[LEN] = {0};

	read(fd, buf, LEN);
	LOG_DEBUG("\tSDP: " << buf);

	return true;
}

bool CMp4Demuxer::ParseMeta(int fd, Atom a)
{
	return true;
	UInt32 version;

	Read32BE(fd, version);

	a.offset += 4;
	a.size   -= 4;

	return ParseDefault(fd, a);
}
