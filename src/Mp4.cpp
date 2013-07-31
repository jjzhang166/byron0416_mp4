#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h> //htonl
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
				ret = ParseDefault(fd, atom);
				break;
			}
		case MKTYPE('t', 'k', 'h', 'd'):
			{
				ret = true;
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
				ret = ParseTref(fd, atom);
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

bool CMp4Demuxer::ParseHdlr(int fd, Atom atom)
{
	UInt32 version;
	UInt32 preDefined;
	UInt32 handle;

	read(fd, &version, 4);
	read(fd, &preDefined, 4);
	read(fd, &handle, 4);

	LOG_DEBUG("Track for " << PRTYPE(handle));

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
		UInt32 samples;
		UInt32 delta;

		read(fd, &samples, 4);
		samples = htonl(samples);
		read(fd, &delta, 4);
		delta = htonl(delta);
		LOG_TRACE("\tSTTS: sample count " << samples << ", delta " << delta << ".");
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

	if(size != 0)
	{
		LOG_TRACE("\tSTSZ: sample count " << count << ", sample size " << size << ".");
	}
	else
	{
		for(UInt32 i=0; i<count; i++)
		{
			read(fd, &size, 4);
			size = htonl(size);
			LOG_TRACE("\tSTSZ: sample count 1, sample size " << size << ".");
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
		LOG_TRACE("\tSTCO: chunk " << i+1 << " with offset " << offset << ".");
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

bool CMp4Demuxer::ParseTref(int fd, Atom a)
{
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
