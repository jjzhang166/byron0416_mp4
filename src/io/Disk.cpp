#include <sys/eventfd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "Disk.h"


const size_t ALIGN_SIZE = 512;
vector<string> CDiskIO::m_Paths;

void CDiskIO::AddPath(const string &path)
{
	if(path[path.size()-1] == '/')
		m_Paths.push_back(path);
	else
		m_Paths.push_back(path+"/");
}

CDiskIO::CDiskIO(CEventEngin *engin):
	CAsyncIO(engin)
{
	m_Fd = -1;
	m_Offset = 0;
	m_Len = 0;
	m_Buf = NULL;
	m_EventFd = -1;
}

CDiskIO::~CDiskIO()
{
	Close();
}

void CDiskIO::AsyncRead(const string &path, size_t offset, size_t len, CEvent *event, CBuffer *buf)
{
	struct iocb cb = {0};
	struct iocb *cbp = &cb;
	void *b = NULL;
	size_t size = 0;
	int ret;

	m_Pre = event;
	m_Buf = buf;

	LOG_INFO("Read file from disk(" << path << ":" << offset << ":" << len << ").");

	size = Open(path);
	if(size > 0)
	{
		if(len==0 || len>size)
			len = size;
	}
	else
		return; // Failed to open file.

	m_Offset = offset;
	m_Len = len;
	offset = offset / ALIGN_SIZE * ALIGN_SIZE;
	size_t l = (m_Offset + len + ALIGN_SIZE - 1) / ALIGN_SIZE * ALIGN_SIZE - offset; // Actually need to read.
	m_Offset -= offset;

	if(true == m_Buf->Alloc(l))
	{
		b = m_Buf->GetData(len);
		m_Buf->SetOffset(m_Offset);
	}
	else
	{
		Close();
		return ReturnEvent(E_ERROR);
	}

	m_EventFd = eventfd(0, EFD_NONBLOCK|EFD_CLOEXEC);
	if(m_EventFd == -1)
	{
		LOG_ERROR("Failed to create event fd(" << errno << ").");
		Close();
		return ReturnEvent(E_ERROR);
	}

	memset(&m_Ctx, 0, sizeof(m_Ctx));
	ret = io_setup(1, &m_Ctx);
	if(ret != 0)
	{
		LOG_ERROR("Failed to initialize io object(" << ret << ").");
		Close();
		return ReturnEvent(E_ERROR);
	}
	io_prep_pread(&cb, m_Fd, b, l, offset);
	io_set_eventfd(&cb, m_EventFd);
	ret = io_submit(m_Ctx, 1, &cbp);
	if(1 != ret)
	{
		LOG_ERROR("Failed to submit io request for " << path << "(" << ret << ").");
		Close();
		return ReturnEvent(E_ERROR);
	}

	if(false == RegisterRD())
	{
		Close();
		return ReturnEvent(E_ERROR);
	}

	LOG_DEBUG("Submit a io request to disk(" << path << ":" << offset << ":" << l << ").");
}

size_t CDiskIO::GetFilePath(string &path)
{
	struct stat fs;

	for(size_t i=0; i<m_Paths.size(); i++)
	{
		string tmp = m_Paths[i] + path;
		if(stat(tmp.c_str(), &fs) == 0)
		{
			path = tmp;

			return fs.st_size;
		}
		else
		{
			if(errno != ENOENT)
				LOG_WARN("Failed to stat file " << tmp << "(" << strerror(errno) << ").");
		}
	}

	return 0;
}

size_t CDiskIO::Open(const string &path)
{
	string full = path;

	size_t size = GetFilePath(full);
	if(size > 0)
	{
		LOG_DEBUG("Open file " << full << ".");
		m_Fd = open(full.c_str(), O_RDONLY|O_DIRECT|O_LARGEFILE|O_NONBLOCK, 0);

		if(m_Fd > 0)
		{
			return size;
		}
		else
		{
			int err = errno;
			LOG_WARN("Failed to open file " << full << "(" << strerror(err) << ").");
			if(err == EACCES)
				ReturnEvent(E_FORBIDDEN);
			else
				ReturnEvent(E_ERROR);

			return 0;
		}
	}
	else
	{
		LOG_WARN("Cannot find " << path << " file in configured paths.");
		ReturnEvent(E_NOTFOUND);

		return 0;
	}
}

void CDiskIO::Close()
{
	if(m_Fd > 0)
	{
		close(m_Fd);
		m_Fd = -1;
		io_destroy(m_Ctx);
	}

	m_Offset = 0;
	m_Len = 0;
	m_Buf = NULL;

	if(m_EventFd > 0)
	{
		Unregister();
		close(m_EventFd);
		m_EventFd = -1;
	}
}

void CDiskIO::OnRead()
{
	struct io_event event = {0};
	struct timespec tms = {0};
	size_t finish = 0;

	while(0 < read(m_EventFd, &finish, sizeof(finish)))
	{
		LOG_DEBUG("Disk io get " << finish << " read events.")

		if(finish == 1)
		{
			int n = io_getevents(m_Ctx, 1, 1, &event, &tms);
			if(n == 1)
			{
				m_Len = m_Len < event.res-m_Offset ? m_Len : event.res-m_Offset;
				m_Buf->SetUsed(m_Len);
				LOG_INFO("Disk io get " << m_Len << " bytes.")
				Close();

				return ReturnEvent(E_OK);
			}
			else
			{
				LOG_WARN("Failed to read file.")
				Close();

				return ReturnEvent(E_ERROR);
			}
		}
		else
		{
			LOG_WARN("Eventfd get(" << finish <<") more than one reading event.");
			Close();

			return ReturnEvent(E_ERROR);
		}
	}
}
