#ifndef Pds_Validators_hh
#define Pds_Validators_hh

#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>
#include <stdint.h>

class QLineEdit;

namespace Pds_ConfigDb {

  class Parameter;

  class IntValidator : public QIntValidator {
    Q_OBJECT
  public:
    IntValidator(Parameter& p, QLineEdit& l,
                 int vlo, int vhi);
    IntValidator(Parameter& p, QLineEdit& l,
		 unsigned& vlo, unsigned& vhi);
    ~IntValidator();
  public:
    virtual void fixup(QString&) const;
    QValidator::State validate(QString&,int&) const;
  public slots:
    void validChange();  
  private:
    Parameter& _p;
    bool       _unsigned;
    unsigned   _rlo, _rhi;
  };

  class HexValidator : public QValidator {
    Q_OBJECT
  public:
    HexValidator(Parameter& p, QLineEdit& l,
		 uint64_t vlo, uint64_t vhi);
    ~HexValidator();
  public:
    void fixup(QString&) const;
    QValidator::State validate(QString&,int&) const;
  public slots:
    void validChange();
  private:
    Parameter& _p;
    uint64_t   _rlo, _rhi;
  };

  class DoubleValidator : public QDoubleValidator {
    Q_OBJECT
  public:
    DoubleValidator(Parameter& p, QLineEdit& l,
		    double vlo, double vhi);
    ~DoubleValidator();
  public:
    void fixup(QString&) const;
  public slots:
    void validChange();  
  private:
    Parameter& _p;
  };

};

#endif
