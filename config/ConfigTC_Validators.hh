#ifndef Pds_CfgValidators_hh
#define Pds_CfgValidators_hh

#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>

class QLineEdit;

namespace ConfigGui {

  class Parameter;

  class IntValidator : public QIntValidator {
    Q_OBJECT
  public:
    IntValidator(Parameter& p, QLineEdit& l,
		 int vlo, int vhi);
    ~IntValidator();
  public:
    void fixup(QString&) const;
  public slots:
    void validChange();  
  private:
    Parameter& _p;
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
