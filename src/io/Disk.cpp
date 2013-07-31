#include <sys/eventfd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "Disk.h"


const size_t ALIGN_SIZE = 512;

CDiskIO::CDiskIO(CEventEngin *engin):
	CAsyncIO(engin)
{
	m_Fd = -1;
	m_EventFd = -1;
	m_Offset = 0;
	m_Len = 0;
	m_Buf = NULL;
}

CDiskIO::~CDiskIO()
{
	Close();
}

void CDiskIO::AsyncRead(const string &path, size_t offset, size_t len, CEvent *event, CBuffer *buf)
{
	struct iocb cb = {0};
	struct iocb *cbp = &cb;
	struct stat fs;
	int ret;

	m_Pre = event;
	m_Buf = buf;

	if(stat(path.c_str(), &fs) == 0)
	{
		if(Open(path) == false)
			return;
	}
	else
	{
		int err = errno;
		LOG_WARN("Failed to stat file " << path << "(" << strerror(errno) << ").");
		if(err == EACCES)
			return ReturnSubEvent(E_FORBIDDEN);
		else
			return ReturnSubEvent(E_NOTFOUND);
	}

	if(len==0 || len>size_t(fs.st_size))
		len = fs.st_size;

	m_Offset = offset;
	m_Len = len;
	offset = offset / ALIGN_SIZE * ALIGN_SIZE;
	size_t l = (m_Offset + len + ALIGN_SIZE - 1) / ALIGN_SIZE * ALIGN_SIZE - offset;
	m_Offset -= offset;

	if(false == m_Buf->Alloc(l))
	{
		Close();

		return ReturnSubEvent(E_ERROR);
	}
	void *b = m_Buf->GetData(len);
	m_Buf->SetOffset(m_Offset);

	m_EventFd = eventfd(0, EFD_NONBLOCK|EFD_CLOEXEC);
	if(m_EventFd == -1)
	{
		LOG_ERROR("Failed to create event fd.(" << errno << ")");
		Close();

		return ReturnSubEvent(E_ERROR);
	}

	memset(&m_Ctx, 0, sizeof(m_Ctx));
	ret = io_setup(1, &m_Ctx);
	if(ret != 0)
	{
		LOG_ERROR("Failed to initialize io object.(" << ret << ")");
		Close();

		return ReturnSubEvent(E_ERROR);
	}
	io_prep_pread(&cb, m_Fd, b, l, offset);
	io_set_eventfd(&cb, m_EventFd);
	ret = io_submit(m_Ctx, 1, &cbp);
	if(1 != ret)
	{
		LOG_ERROR("Failed to submit io request for " << path << "(" << ret << ").");
		Close();

		return ReturnSubEvent(E_ERROR);
	}

	LOG_INFO("Submit a io request to disk(" << path << ":" << offset << ":" << m_Len << ").");

	if(false == RegisterRD())
	{
		Close();

		return ReturnSubEvent(E_ERROR);
	}
}

bool CDiskIO::Open(const string &path)
{
	m_Fd = open(path.c_str(), O_RDONLY|O_DIRECT|O_LARGEFILE|O_NONBLOCK, 0);
	if(m_Fd == -1)
	{
		int err = errno;
		LOG_WARN("Failed to open file " << path << "(" << strerror(err) << ").");
		if(err == EACCES)
			ReturnSubEvent(E_FORBIDDEN);
		else
			ReturnSubEvent(E_NOTFOUND);

		return false;
	}

	return true;
}

void CDiskIO::Close()
{
	LOG_DEBUG("Close this io object.");

	if(m_Fd > 0)
	{
		io_destroy(m_Ctx);

		close(m_Fd);
		m_Fd = -1;
	}

	if(m_EventFd > 0)
	{
		Unregister();
		close(m_EventFd);
		m_EventFd = -1;
	}

	m_Offset = 0;
	m_Len = 0;
	m_Buf = NULL;
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

				return ReturnSubEvent(E_OK);
			}
			else
			{
				LOG_WARN("Failed to read file.")
				return ReturnSubEvent(E_ERROR);
			}
		}
		else
		{
			LOG_WARN("Eventfd get(" << finish <<") more than one reading event.");
			return ReturnSubEvent(E_ERROR);
		}
	}
}
