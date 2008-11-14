#include "FrameServer.hh"

#include "pds/camera/Camera.hh"
#include "pds/camera/Opal1000.hh"

#include "pds/service/ZcpFragment.hh"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/uio.h>

using namespace Pds;

const int Post_NewFrame = 1;
const int Post_Fragment = 2;

FrameServer::FrameServer(const Src& src,
			 Opal1000&  camera) :
  _camera(camera),
  _more(false),
  _xtc (TypeId::Id_Frame, src)
{
  int err = ::pipe(_fd);
  if (err)
    printf("Error opening FrameServer pipe: %s\n",strerror(errno));
  fd(_fd[0]);
}

FrameServer::~FrameServer()
{
  ::close(_fd[0]);
  ::close(_fd[1]);
}

void FrameServer::post()
{
  int msg = Post_NewFrame;
  iovec iov[1];
  iov[0].iov_base = &msg  ; iov[0].iov_len = sizeof(msg);
  ::writev(_fd[1],iov,1);
}

void FrameServer::dump(int detail) const
{
}

bool FrameServer::isValued() const
{
  return true;
}

const Src& FrameServer::client() const
{
  return _xtc.src;
}

const Xtc& FrameServer::xtc() const
{
  return _xtc;
}

bool FrameServer::more() const
{
  return _more;
}

unsigned FrameServer::length() const
{
  return _length;
}

unsigned FrameServer::offset() const
{
  return _offset;
}

int FrameServer::pend(int flag)
{
  return 0;
}

int FrameServer::fetch(char* payload, int flags)
{
  int newsize;
  int msg;
  int length = ::read(_fd[0],&msg,sizeof(msg));
  if (length >= 0) {
    Frame* pFrame;
    pFrame = _camera.GetFrame();
    _count = _camera.CurrentCount;
    newsize = pFrame->width*pFrame->height*pFrame->elsize;
    
    Xtc* xtc = new (payload) Xtc(_xtc);
    memcpy(xtc->alloc(sizeof(Frame)),pFrame,sizeof(Frame));
    memcpy(xtc->alloc(newsize),pFrame->data,newsize);
    length = _xtc.extent = xtc->extent;
  }
  else
    printf("FrameServer::fetch error: %s\n",strerror(errno));

  printf("Received frame %d size %d\n",_count,length);
  return length;
}

int FrameServer::fetch(ZcpFragment& zfo, int flags)
{
  _more = false;

  int msg;
  int length = ::read(_fd[0],&msg,sizeof(msg));
  if (length < 0) goto read_err;

  Frame* pFrame;
  if (msg == Post_NewFrame) {
    pFrame = _camera.GetFrame();
    _count = _camera.CurrentCount;
    int newsize = pFrame->width*pFrame->height*pFrame->elsize;

    _xtc.extent = sizeof(Xtc) + newsize;

    if (zfo.uinsert( &_xtc, sizeof(Xtc)) < 0) goto read_err;
    if (zfo.uinsert( pFrame, sizeof(Frame)) < 0) goto read_err;
    if ((length = zfo.uinsert( pFrame->data, newsize)) < 0) goto read_err;
    if (length != newsize) { // fragmentation
      _more   = true;
      _offset = 0;
      _next   = length + sizeof(Frame);
      msg     = Post_Fragment;
      iovec iov[3];
      iov[0].iov_base = &msg   ; iov[0].iov_len = sizeof(msg);
      iov[1].iov_base = &pFrame; iov[1].iov_len = sizeof(pFrame);
      iov[2].iov_base = &_count; iov[1].iov_len = sizeof(_count);
      ::writev(_fd[1],iov,3);
    }
    else
      delete pFrame;
    length += sizeof(Frame);
  }
  else { // Post_Fragment
    length = ::read(_fd[0],&pFrame,sizeof(pFrame));
    if (length < 0) goto read_err;
    
    length = ::read(_fd[0],&_count,sizeof(_count));
    if (length < 0) goto read_err;

    _more = true;
    int remaining = _xtc.sizeofPayload()-_next;
    if ((length = zfo.uinsert( reinterpret_cast<char*>(pFrame->data)+_next-sizeof(Frame), remaining)) < 0)
      goto read_err;

    _offset = _next;
    _next  += length;
    if (length != remaining) {
      iovec iov[3];
      iov[0].iov_base = &msg   ; iov[0].iov_len = sizeof(msg);
      iov[1].iov_base = &pFrame; iov[1].iov_len = sizeof(pFrame);
      iov[2].iov_base = &_count; iov[1].iov_len = sizeof(_count);
      ::writev(_fd[1],iov,3);
    }
    else
      delete pFrame;
  }
  return length;

 read_err:
  printf("FrameServer::fetchz error: %s\n",strerror(errno));
  return length;
}

unsigned FrameServer::count() const
{
  return _count;
}
