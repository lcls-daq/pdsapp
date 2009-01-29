#include "FexFrameServer.hh"
#include "CameraFexConfig.hh"

#include "pds/camera/DmaSplice.hh"
#include "pds/camera/Frame.hh"
#include "pds/camera/TwoDMoments.hh"
#include "pds/camera/TwoDGaussian.hh"

#include "pds/camera/Camera.hh"
#include "pds/camera/Opal1000.hh"
#include "pds/service/ZcpFragment.hh"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/uio.h>
#include <new>

using namespace Pds;

typedef unsigned short pixel_type;

FexFrameServer::FexFrameServer(const Src& src,
			       PdsLeutron::Opal1000& camera,
			       DmaSplice& splice) :
  _camera(camera),
  _splice(splice),
  _more  (false),
  _xtc   (TypeId::Id_Frame, src),
  _config(0)
{
  int err = ::pipe(_fd);
  if (err)
    printf("Error opening FexFrameServer pipe: %s\n",strerror(errno));
  fd(_fd[0]);
}

FexFrameServer::~FexFrameServer()
{
  ::close(_fd[0]);
  ::close(_fd[1]);
}

void FexFrameServer::Config(const CameraFexConfig& cfg)
{
  _config = &cfg;
//   switch(cfg._algorithm) {
//   case CameraFexConfig::RawImage:
//   case CameraFexConfig::RegionOfInterest:
//   case CameraFexConfig::TwoDGaussianFull:
//   case CameraFexConfig::TwoDGaussianROI:
//   case CameraFexConfig::TwoDGaussianAndFrame:
//   }
}

void FexFrameServer::post()
{
  FrameServerMsg* msg = 
    new FrameServerMsg(FrameServerMsg::NewFrame,
		       _camera.GetFrameHandle(),
		       _camera.CurrentCount,
		       0);
  msg->connect(_msg_queue.reverse());
  ::write(_fd[1],&msg,sizeof(msg));
}

void FexFrameServer::dump(int detail) const
{
}

bool FexFrameServer::isValued() const
{
  return true;
}

const Src& FexFrameServer::client() const
{
  return _xtc.src;
}

const Xtc& FexFrameServer::xtc() const
{
  return _xtc;
}

//
//  Fragment information
//
bool FexFrameServer::more() const
{
  return _more;
}

unsigned FexFrameServer::length() const
{
  return _xtc.extent;
}

unsigned FexFrameServer::offset() const
{
  return _offset;
}

int FexFrameServer::pend(int flag)
{
  return 0;
}

//
//  Apply feature extraction to the input frame and
//  provide the result to the event builder
//
int FexFrameServer::fetch(char* payload, int flags)
{
  FrameServerMsg* msg;
  int length = ::read(_fd[0],&msg,sizeof(msg));
  if (length >= 0) {
    FrameServerMsg* fmsg = _msg_queue.remove();
    _count = fmsg->count;

    //  Is pipe write/read good enough?
    if (msg != fmsg) printf("Overlapping events %d/%d\n",msg->count,fmsg->count);

    Frame frame(*fmsg->handle);
    const unsigned short* frame_data = 
      reinterpret_cast<const unsigned short*>(fmsg->handle->data);
    delete fmsg;

    //
    // perform the feature extraction here
    //
    if (_config->algorithm == CameraFexConfig::RawImage) {
      Xtc* xtc = new (payload) Xtc(_xtc);
      *new(xtc->alloc(sizeof(Frame))) Frame(frame.width, frame.height, frame.depth,
					    frame_data);
      xtc->extent += frame.extent;
      return xtc->extent;
    }
    else if (_config->algorithm == CameraFexConfig::RegionOfInterest) {
      Xtc* xtc = new(payload) Xtc(_xtc);
      Frame& roiFrame = *new (xtc->alloc(sizeof(Frame))) 
	Frame (_config->regionOfInterestStart.column,
	       _config->regionOfInterestEnd.column,
	       _config->regionOfInterestStart.row,
	       _config->regionOfInterestEnd.row,
	       frame.width, frame.height, frame.depth,
	       frame_data);
      xtc->extent += roiFrame.extent;
      return xtc->extent;
    }
    else if (_config->algorithm == CameraFexConfig::TwoDGaussianFull) {
      TwoDMoments moments(frame.width, frame.height, frame_data);

      Xtc* xtc = new(payload) Xtc(TypeId::Id_TwoDGaussian, _xtc.src);
      new(xtc->alloc(sizeof(TwoDGaussian))) TwoDGaussian(moments);
      return xtc->extent;
    }
    else if (_config->algorithm == CameraFexConfig::TwoDGaussianROI) {
      TwoDMoments moments(frame.width,
			  _config->regionOfInterestStart.column,
			  _config->regionOfInterestEnd.column,
			  _config->regionOfInterestStart.row,
			  _config->regionOfInterestEnd.row,
			  frame_data);

      Xtc* xtc = new(payload) Xtc(TypeId::Id_TwoDGaussian, _xtc.src);
      new(xtc->alloc(sizeof(TwoDGaussian))) TwoDGaussian(moments);
      return xtc->extent;
    }
    else if (_config->algorithm == CameraFexConfig::TwoDGaussianAndFrame) {
      Xtc* xtc = new(payload) Xtc(TypeId::Id_Xtc, _xtc.src);

      Xtc& gssxtc = *new(reinterpret_cast<char*>(xtc->alloc(0)))
	Xtc(TypeId::Id_TwoDGaussian, _xtc.src);
      TwoDMoments moments(frame.width,
			  _config->regionOfInterestStart.column,
			  _config->regionOfInterestEnd.column,
			  _config->regionOfInterestStart.row,
			  _config->regionOfInterestEnd.row,
			  frame_data);
      new(gssxtc.alloc(sizeof(TwoDGaussian))) TwoDGaussian(moments);
      xtc->extent += gssxtc.extent;

      Xtc& frmxtc = *new(reinterpret_cast<char*>(xtc->alloc(0)))
	Xtc(TypeId::Id_Frame, _xtc.src);
      Frame& roiFrame = *new (frmxtc.alloc(sizeof(Frame))) 
	Frame (_config->regionOfInterestStart.column,
	       _config->regionOfInterestEnd.column,
	       _config->regionOfInterestStart.row,
	       _config->regionOfInterestEnd.row,
	       frame.width, frame.height, frame.depth,
	       frame_data);
      frmxtc.extent += roiFrame.extent;
      xtc->extent += frmxtc.extent;

      return xtc->extent;
    }
    else if ( _config->algorithm == CameraFexConfig::Sink ) {
      Xtc* xtc = new(payload) Xtc(TypeId::Id_Frame, _xtc.src);
      Frame& emptyFrame = *new(xtc->alloc(sizeof(Frame))) Frame(frame);
      emptyFrame.extent = 0;
      return xtc->extent;
    }
    else {
    }
  }
  else
    printf("FexFrameServer::fetch error: %s\n",strerror(errno));

  return length;
}

//
//  Apply feature extraction to the input frame and
//  provide the result to the (zero-copy) event builder
//
int FexFrameServer::fetch(ZcpFragment& zfo, int flags)
{
  _more = false;
  int length = 0;
  FrameServerMsg* fmsg;

  fmsg = _msg_queue.remove();
  _count = fmsg->count;
  unsigned offset = fmsg->offset;

  FrameServerMsg* msg;
  length = ::read(_fd[0],&msg,sizeof(msg));
  if (length < 0) throw length;
  
  Frame frame(*fmsg->handle);
  const unsigned short* frame_data = reinterpret_cast<const unsigned short*>(fmsg->handle->data);
  
  if (fmsg->type == FrameServerMsg::NewFrame) {
    
    //
    // perform the feature extraction here
    //
    if (_config->algorithm == CameraFexConfig::RawImage) {
      //  move/splice the image
      Frame zcpFrame(frame.width, frame.height, frame.depth, 
		     *fmsg->handle, _splice);
      length = _queue_frame( zcpFrame, fmsg, zfo );
    }
    
    else if (_config->algorithm == CameraFexConfig::RegionOfInterest) {
      //
      //  Reduce the frame to only the region of interest
      //
      Frame roiFrame(_config->regionOfInterestStart.column,
		     _config->regionOfInterestEnd.column,
		     _config->regionOfInterestStart.row,
		     _config->regionOfInterestEnd.row,
		     frame.width, frame.height, frame.depth,
		     *fmsg->handle,_splice);
      length = _queue_frame( roiFrame, fmsg, zfo );
    }
    
    else if (_config->algorithm == CameraFexConfig::TwoDGaussianFull) {
      TwoDMoments moments(frame.width, frame.height, frame_data);
      length = _queue_fex( moments, fmsg, zfo );
    }

    else if (_config->algorithm == CameraFexConfig::TwoDGaussianROI) {
      TwoDMoments moments(frame.width,
			  _config->regionOfInterestStart.column,
			  _config->regionOfInterestEnd.column,
			  _config->regionOfInterestStart.row,
			  _config->regionOfInterestEnd.row,
			  frame_data);
      length = _queue_fex( moments, fmsg, zfo );
    }      

    else if (_config->algorithm == CameraFexConfig::TwoDGaussianAndFrame) {
      TwoDMoments moments(frame.width,
			  _config->regionOfInterestStart.column,
			  _config->regionOfInterestEnd.column,
			  _config->regionOfInterestStart.row,
			  _config->regionOfInterestEnd.row,
			  frame_data);
      Frame roiFrame(_config->regionOfInterestStart.column,
		     _config->regionOfInterestEnd.column,
		     _config->regionOfInterestStart.row,
		     _config->regionOfInterestEnd.row,
		     frame.width, frame.height, frame.depth,
		     *fmsg->handle,_splice);
      length = _queue_fex_and_frame( moments, roiFrame, fmsg, zfo );
    }

    else if (_config->algorithm == CameraFexConfig::Sink) {
      frame.extent = 0;
      length = _queue_frame( frame, fmsg, zfo );
    }

    else {
      length = 0;
    }
  }
  else { // Post_Fragment
    _more = true;
    int framesize = msg->handle->width*msg->handle->height*msg->handle->elsize;
    int remaining = framesize - offset;
    if ((length = zfo.kinsert( _splice.fd(), remaining)) < 0) throw length;
    
    _offset = offset + sizeof(Frame) + sizeof(Xtc);
    if (length != remaining) {
      fmsg->offset += length;
      fmsg->connect(reinterpret_cast<FrameServerMsg*>(&_msg_queue));
      ::write(_fd[1],&fmsg,sizeof(fmsg));
    }
    else
      delete fmsg;
  }
  return length;
}

unsigned FexFrameServer::count() const
{
  return _count;
}

//
//  Zero copy helper functions
//
int FexFrameServer::_queue_frame( const Frame& frame, 
				  FrameServerMsg* fmsg,
				  ZcpFragment& zfo )
{
  _xtc.contains = TypeId::Id_Frame;
  _xtc.extent = sizeof(Xtc) + sizeof(Frame) + frame.extent;

  try {
    int err;
    if ((err=zfo.uinsert( &_xtc, sizeof(Xtc))) != sizeof(Xtc)) throw err;
    
    if ((err=zfo.uinsert( &frame, sizeof(Frame))) != sizeof(Frame)) throw err;
    
    int length;
    if ((length = zfo.kinsert( _splice.fd(), frame.extent)) < 0) throw length;
    if (length != frame.extent) { // fragmentation
      _more   = true;
      _offset = 0;
      fmsg->type   = FrameServerMsg::Fragment;
      fmsg->offset = length;
      fmsg->connect(reinterpret_cast<FrameServerMsg*>(&_msg_queue));
      ::write(_fd[1],&fmsg,sizeof(fmsg));
    }
    else
      delete fmsg;

    return length + sizeof(Frame) + sizeof(Xtc);
  }
  catch (int err) {
    printf("FexFrameServer::_queue_frame error: %s : %d\n",strerror(errno),err);
    delete fmsg;
    return -1;
  }
}

int FexFrameServer::_queue_fex( const TwoDMoments& moments,
				FrameServerMsg* fmsg,
				ZcpFragment& zfo )
{	
  delete fmsg;

  _xtc.contains = TypeId::Id_TwoDGaussian;
  _xtc.extent   = sizeof(Xtc) + sizeof(TwoDGaussian);

  TwoDGaussian payload(moments);

  try {
    int err;
    if ((err=zfo.uinsert( &_xtc, sizeof(Xtc))) < 0) throw err;
    
    if ((err=zfo.uinsert( &payload, sizeof(payload))) < 0) throw err;

    return _xtc.extent;
  }
  catch (int err) {
    printf("FexFrameServer::_queue_frame error: %s : %d\n",strerror(errno),err);
    return -1;
  }
}

int FexFrameServer::_queue_fex_and_frame( const TwoDMoments& moments,
					  const Frame& frame,
					  FrameServerMsg* fmsg,
					  ZcpFragment& zfo )
{	

  Xtc fexxtc(TypeId::Id_TwoDGaussian, _xtc.src);
  fexxtc.extent += sizeof(TwoDGaussian);

  Xtc frmxtc(TypeId::Id_Frame, _xtc.src);
  frmxtc.extent += sizeof(Frame);

  _xtc.contains = TypeId::Id_Xtc;
  _xtc.extent   = sizeof(Xtc) + fexxtc.extent + frmxtc.extent;

  TwoDGaussian fex(moments);

  try {
    int err;
    if ((err=zfo.uinsert( &_xtc  , sizeof(Xtc))) < 0) throw err;

    if ((err=zfo.uinsert( &fexxtc, sizeof(Xtc))) < 0) throw err;
    if ((err=zfo.uinsert( &fex   , sizeof(fex))) < 0) throw err;
    
    if ((err=zfo.uinsert( &frmxtc, sizeof(Xtc))) < 0) throw err;

    int length;
    if ((length = zfo.kinsert( _splice.fd(), frame.extent)) < 0) throw length;
    if (length != frame.extent) { // fragmentation
      _more   = true;
      _offset = 0;
      fmsg->type   = FrameServerMsg::Fragment;
      fmsg->offset = length;
      fmsg->connect(reinterpret_cast<FrameServerMsg*>(&_msg_queue));
      ::write(_fd[1],&fmsg,sizeof(fmsg));
    }
    else
      delete fmsg;

    return _xtc.extent + length - frame.extent;
  }
  catch (int err) {
    printf("FexFrameServer::_queue_frame error: %s : %d\n",strerror(errno),err);
    delete fmsg;
    return -1;
  }
}
