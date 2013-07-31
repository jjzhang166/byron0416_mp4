#ifndef MP4_H
#define MP4_H


#include <string>
#include "Log.h"


using std::string;

typedef unsigned int UInt32;

#define PRTYPE(type) ((char*)&type)[0] << ((char*)&type)[1] << ((char*)&type)[2] << ((char*)&type)[3]
#define MKTYPE(a, b, c, d) ((a) | (b<<8) | (c<<16) | (d<<24))

struct Atom
{
	UInt32 type;
	size_t offset; /** From 0. */
	size_t size; /** Exclude header size. */
};

class CMp4Demuxer: public CLogger
{
public:
	CMp4Demuxer();
	bool Parse(const string &path);
private:
	ssize_t Read32BE(int fd, UInt32&);
	bool ParseAtom(int fd, Atom a);
	bool ParseDefault(int fd, Atom a);
	bool ParseFtyp(int fd, Atom a);
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
	bool ParseTref(int fd, Atom a);
	bool ParseSdp(int fd, Atom a);
	bool ParseMeta(int fd, Atom a);
private:
	UInt32 m_Brand;
};


#endif
