#define __STDC_LIMIT_MACROS
#include <new>

#include "TimeToolConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/EventLogic.hh"
#include "pdsapp/config/QtConcealer.hh"

#include "pdsdata/psddl/timetool.ddl.h"

#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QFileDialog>
#include <QtGui/QPushButton>
#include <QtGui/QStackedWidget>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>

#include <stdint.h>

using namespace Pds_ConfigDb;

static const char* xy_names[] = { "X","Y",NULL };

using Pds::Camera::FrameCoord;

typedef Pds::TimeTool::ConfigV3 TT;

namespace Pds_ConfigDb
{  
  class ROI : public Parameter {
  public:
    ROI(const char* label,
        const char* a,
        const char* b) : 
      Parameter(label),
      _sw(new QStackedWidget),
      _lo(0,0,0,0x1fff),
      _hi(0,0x1ff,0,0x1fff) 
    {
      QString qa = QString("%1 %2 Range").arg(_label).arg(a);
      _sw->addWidget(new QLabel(qa));
      QString qb = QString("%1 %2 Range").arg(_label).arg(b);
      _sw->addWidget(new QLabel(qb));
    }
  public:
    QLayout* initialize(QWidget* p) {
      QHBoxLayout* h = new QHBoxLayout;
      h->addWidget(_sw);
      h->addLayout(_lo.initialize(p));
      h->addLayout(_hi.initialize(p));
      return h;
    }
    void update() {
      _lo.update();
      _hi.update();
      flush();
    }
    void flush() {
      if (_lo.value > _hi.value) {
        unsigned t=_lo.value;
        _lo.value=_hi.value;
        _hi.value=t;
      }
      _lo.flush();
      _hi.flush();
    }
    void enable(bool l) {
      _lo.enable(l);
      _hi.enable(l);
    }
  public:
    QStackedWidget*      _sw;
    NumericInt<unsigned> _lo;
    NumericInt<unsigned> _hi;
  };

  class TimeToolConfig::Private_Data : public Parameter {
  public:
    Private_Data() : 
      _beam_logic ("Beam",true,162),
      _laser_logic("Laser",true,62),
      _use_full_roi("Use Full ROIs",false),
      _use_fit  ("Use Fit",false),
      _proj_axis("Projection Axis",TT::X,xy_names),
      _time_axis("","X","Y"),
      _sig_roi  ("Signal"  ,"Y","X"),
      _use_ref  ("Use Separate Reference Region",false),
      _ref_roi  ("Reference","Y","X"),
      _ref_conv ("Reference Convergence (1/N)",1.,0.,1.),
      _use_sb   ("Subtract Sideband Region",false),
      _sb_roi   ("Sideband","Y","X"),
      _sb_conv  ("Sideband Convergence (1/N)",0.05,0.,1.),
      _sig_cut  ("Signal Minimum Projected Value",0,-(1<<20),(1<<20)),
      _fit_iter ("Maximum Fit Iterations",100,0,UINT32_MAX),
      _fit_fact ("Fit Weights Scale Factor",0.1,0.,1.),
      _record_image("Image",true),
      _record_proj ("Projections",false),
      _base_name("Output PV name","TTSPEC",32),
      _weights  ("Filter Weights"), 
      _cal_poly ("Pixel-Time Calib Polynomial"),
      _fit_pars ("Fit Parameter Initial Values")
    {
      _weights .value.clear();
      _weights .value.push_back(1);
      _cal_poly.value.clear();
      _cal_poly.value.push_back(0);
      _cal_poly.value.push_back(1);
      _fit_pars.value.clear();
      _fit_pars.value.push_back(1.);
      _fit_pars.value.push_back(2.);
      _fit_pars.value.push_back(0.);
      _fit_pars.value.push_back(0.);

      pList.insert(&_beam_logic);
      pList.insert(&_laser_logic);
      pList.insert(&_use_full_roi);
      pList.insert(&_use_fit);
      pList.insert(&_proj_axis);
      pList.insert(&_time_axis);
      pList.insert(&_sig_roi);
      pList.insert(&_ref_roi);
      pList.insert(&_sb_roi);
      pList.insert(&_sb_conv);
      pList.insert(&_ref_conv);
      pList.insert(&_sig_cut);
      pList.insert(&_use_sb);
      pList.insert(&_use_ref);
      pList.insert(&_fit_iter);
      pList.insert(&_fit_fact);
      pList.insert(&_record_image);
      pList.insert(&_record_proj);
      pList.insert(&_base_name);
      pList.insert(&_weights);
      pList.insert(&_cal_poly);
      pList.insert(&_fit_pars);
    }
    ~Private_Data() {}
  public:
    void flush () { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->flush(); }
    void update() { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->update(); }
    void enable(bool l) { 
      for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->enable(l); 
      //      _record_proj.enable(false);
    }
  public:
    QLayout* initialize(QWidget* p) {
      QVBoxLayout* layout = new QVBoxLayout;
      layout->addLayout(_beam_logic .initialize(p));
      layout->addLayout(_laser_logic.initialize(p));
      layout->addLayout(_proj_axis.initialize(p));
      layout->addLayout(_time_axis.initialize(p));
      layout->addLayout(_use_full_roi.initialize(p));
      layout->addLayout(_sig_roi  .initialize(p));
      layout->addStretch();
      layout->addLayout(_use_sb   .initialize(p));
      layout->addLayout(_sb_concealer.add(_sb_roi .initialize(p)));
      layout->addLayout(_sb_concealer.add(_sb_conv.initialize(p)));
      layout->addStretch();
      layout->addLayout(_use_ref   .initialize(p));
      layout->addLayout(_ref_concealer.add(_ref_roi .initialize(p)));
      layout->addStretch();
      layout->addLayout(_ref_conv .initialize(p));
      layout->addLayout(_sig_cut  .initialize(p));
      layout->addStretch();
      layout->addLayout(_use_fit  .initialize(p));
      layout->addLayout(_fit_concealer.add(_fit_iter.initialize(p)));
      layout->addLayout(_fit_concealer.add(_fit_fact.initialize(p)));
      layout->addStretch();
      { QHBoxLayout* h = new QHBoxLayout;
        h->addWidget(new QLabel("Record"));
        h->addLayout(_record_image.initialize(p));
        h->addLayout(_record_proj .initialize(p));
        layout->addLayout(h); }
      layout->addLayout(_weights  .initialize(p));
      layout->addLayout(_cal_poly .initialize(p));
      layout->addLayout(_fit_concealer.add(_fit_pars .initialize(p)));
      layout->addLayout(_base_name.initialize(p));

      if (Parameter::allowEdit()) {
        QObject::connect(_proj_axis._input, SIGNAL(currentIndexChanged(int)), _time_axis._sw, SLOT(setCurrentIndex(int)));
        QObject::connect(_proj_axis._input, SIGNAL(currentIndexChanged(int)), _sig_roi  ._sw, SLOT(setCurrentIndex(int)));
        QObject::connect(_proj_axis._input, SIGNAL(currentIndexChanged(int)), _sb_roi   ._sw, SLOT(setCurrentIndex(int)));
        QObject::connect(_use_sb._input, SIGNAL(toggled(bool)), &_sb_concealer, SLOT(show(bool)));
        QObject::connect(_use_ref._input, SIGNAL(toggled(bool)), &_ref_concealer, SLOT(show(bool)));
        QObject::connect(_use_fit._input, SIGNAL(toggled(bool)), &_fit_concealer, SLOT(show(bool)));
      }
      _sb_concealer.show (_use_sb .value=false);
      _ref_concealer.show(_use_ref.value=false);
      _fit_concealer.show(_use_fit.value=false);
      //      _record_proj.enable(false);

      return layout;
    }
    int pull(void *from)
    {
      const TT& c =
        *reinterpret_cast<const TT*>(from);
      _beam_logic .set(c.beam_logic());
      _laser_logic.set(c.laser_logic());
      _use_full_roi.value = c.use_full_roi();
      _use_fit     .value = c.use_fit();
      TT::Axis a = (TT::Axis)c.project_axis();
      _proj_axis.value = a;
      _time_axis._lo.value = (a==TT::X) ? c.sig_roi_lo().column() : c.sig_roi_lo().row();
      _time_axis._hi.value = (a==TT::X) ? c.sig_roi_hi().column() : c.sig_roi_hi().row();
      _sig_roi  ._lo.value = (a==TT::Y) ? c.sig_roi_lo().column() : c.sig_roi_lo().row();
      _sig_roi  ._hi.value = (a==TT::Y) ? c.sig_roi_hi().column() : c.sig_roi_hi().row();
      _ref_roi  ._lo.value = (a==TT::Y) ? c.ref_roi_lo().column() : c.ref_roi_lo().row();
      _ref_roi  ._hi.value = (a==TT::Y) ? c.ref_roi_hi().column() : c.ref_roi_hi().row();
      _sb_roi  ._lo.value = (a==TT::Y) ? c.sb_roi_lo().column() : c.sb_roi_lo().row();
      _sb_roi  ._hi.value = (a==TT::Y) ? c.sb_roi_hi().column() : c.sb_roi_hi().row();

      _sb_conv .value = c.sb_convergence();
      _ref_conv.value = c.ref_convergence();
      _sig_cut .value = c.signal_cut();

      _weights.value.resize(c.weights().shape()[0]);
      std::copy(c.weights().begin(),c.weights().end(),_weights.value.begin());

      _cal_poly.value.resize(c.calib_poly().shape()[0]);
      std::copy(c.calib_poly().begin(),c.calib_poly().end(),_cal_poly.value.begin());

      _fit_pars.value.resize(c.fit_params().shape()[0]);
      std::copy(c.fit_params().begin(),c.fit_params().end(),_fit_pars.value.begin());

      _use_sb      .value = c.subtract_sideband();
      _use_ref     .value = c.use_reference_roi();
      _fit_iter    .value = c.fit_max_iterations();
      _fit_fact    .value = c.fit_weights_factor();
      _record_image.value = c.write_image();
      _record_proj .value = c.write_projections();

      std::string base_name(c.base_name());

      strcpy(_base_name.value,base_name.c_str());

      if (!Parameter::allowEdit()) {
        _time_axis._sw->setCurrentIndex(int(a));
        _sig_roi  ._sw->setCurrentIndex(int(a));
        _sb_roi   ._sw->setCurrentIndex(int(a));
        _sb_concealer .show(_use_sb .value);
        _ref_concealer.show(_use_ref.value);
        _fit_concealer.show(_use_fit.value);
      }

      return c._sizeof();
    }
    int push(void *to)
    {
      FrameCoord sig_roi_lo, sig_roi_hi, sb_roi_lo, sb_roi_hi, ref_roi_lo, ref_roi_hi;
      if (_proj_axis.value == TT::X) {
        sig_roi_lo = FrameCoord(_time_axis._lo.value,
                                _sig_roi._lo.value);
        sig_roi_hi = FrameCoord(_time_axis._hi.value,
                                _sig_roi._hi.value);
        sb_roi_lo  = FrameCoord(_time_axis._lo.value,
                                _sb_roi._lo.value);
        sb_roi_hi  = FrameCoord(_time_axis._hi.value,
                                _sb_roi._hi.value);
        ref_roi_lo = FrameCoord(_time_axis._lo.value,
                                _ref_roi._lo.value);
        ref_roi_hi = FrameCoord(_time_axis._hi.value,
                                _ref_roi._hi.value);
      }
      else {
        sig_roi_lo = FrameCoord(_sig_roi._lo.value,
                                _time_axis._lo.value);
        sig_roi_hi = FrameCoord(_sig_roi._hi.value,
                                _time_axis._hi.value);
        sb_roi_lo  = FrameCoord(_sb_roi._lo.value,
                                _time_axis._lo.value);
        sb_roi_hi  = FrameCoord(_sb_roi._hi.value,
                                _time_axis._hi.value);
        ref_roi_lo = FrameCoord(_ref_roi._lo.value,
                                _time_axis._lo.value);
        ref_roi_hi = FrameCoord(_ref_roi._hi.value,
                                _time_axis._hi.value);
      }

      ndarray<const Pds::TimeTool::EventLogic,1> beam_logic  = _beam_logic.get();
      ndarray<const Pds::TimeTool::EventLogic,1> laser_logic = _laser_logic.get();

      TT& c = *new(to) TT(_proj_axis   .value,
                          _use_full_roi.value,
                          _use_fit     .value,
                          _record_image.value,
                          _record_proj .value,
                          _use_sb      .value,
                          _use_ref     .value,
                          _weights.value.size(), 
                          _cal_poly.value.size(),
                          _fit_pars.value.size(),
                          strlen(_base_name.value)+1,
                          beam_logic.size(),
                          laser_logic.size(),
                          _sig_cut.value,
                          _fit_iter.value,
                          _fit_fact.value,
                          sig_roi_lo,
                          sig_roi_hi,
                          sb_roi_lo,
                          sb_roi_hi,
                          _sb_conv.value,
                          ref_roi_lo,
                          ref_roi_hi,
                          _ref_conv.value,
                          beam_logic.data(),
                          laser_logic.data(),
                          _weights.value.data(),
                          _cal_poly.value.data(),
                          _fit_pars.value.data(),
                          _base_name.value);
      return c._sizeof();
    }

    int dataSize() const
    {
      TT c(_beam_logic .get().size(),
           _laser_logic.get().size(),
           _weights .value.size(),
           _cal_poly.value.size(),
           _fit_pars.value.size(),
           strlen(_base_name.value)+1);
      return c._sizeof();
    }

    bool validate() {
      bool v=true;
      if (_use_sb .value) v &= _check_region(_sb_roi ,"Sideband");
      if (_use_ref.value) v &= _check_region(_ref_roi,"Reference");
      return v;
    }
  private:
    bool _check_region(const ROI& roi, const char* n) const {
      bool v=true;
      if ((roi._hi.value-roi._lo.value)!=
          (_sig_roi._hi.value-_sig_roi._lo.value)) {
          QMessageBox::warning(_sig_roi._lo._input,
                               QString("%1 region error").arg(n),
                               QString("%1 and signal region sizes differ").arg(n));
          v=false;
      }
      if (!((roi ._hi.value<_sig_roi._lo.value) ||
            (roi ._lo.value>_sig_roi._hi.value))) {
          QMessageBox::warning(_sig_roi._lo._input,
                               QString("%1 region error").arg(n),
                               QString("%1 and signal regions overlap").arg(n));
          v=false;
      }
      return v;
    }
  public:
    Pds::LinkedList<Parameter> pList;
    EventLogic _beam_logic;
    EventLogic _laser_logic;
    CheckValue             _use_full_roi;
    CheckValue             _use_fit;
    Enumerated<TT::Axis>   _proj_axis;
    ROI                    _time_axis;
    ROI                    _sig_roi;
    CheckValue             _use_ref;
    ROI                    _ref_roi;
    NumericFloat<double>   _ref_conv;
    CheckValue             _use_sb;
    ROI                    _sb_roi;
    NumericFloat<double>   _sb_conv;
    NumericInt  <int>      _sig_cut;
    NumericInt  <uint32_t> _fit_iter;
    NumericFloat<double>   _fit_fact;
    CheckValue   _record_image;
    CheckValue   _record_proj;
    TextParameter _base_name;
    Poly<double> _weights;
    Poly<double> _cal_poly;
    Poly<double> _fit_pars;
    QtConcealer  _sb_concealer;
    QtConcealer  _ref_concealer;
    QtConcealer  _fit_concealer;
  };
} // namespace Pds_ConfigDb


TimeToolConfig::TimeToolConfig():
Serializer("TimeToolConfig"), _private_data(new Private_Data)
{
  pList.insert(_private_data);
}

int TimeToolConfig::readParameters(void *from)
{
  return _private_data->pull(from);
}

int TimeToolConfig::writeParameters(void *to)
{
  return _private_data->push(to);
}

int TimeToolConfig::dataSize() const
{
  return _private_data->dataSize();
}

bool TimeToolConfig::validate() 
{
  return _private_data->validate();
}

#include "Parameters.icc"

template class Enumerated<Pds::TimeTool::ConfigV3::Axis>;
