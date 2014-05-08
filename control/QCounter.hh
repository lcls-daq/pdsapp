#include <QtCore/QString>
#include <QtGui/QLabel>

namespace Pds {
  class QCounter {
  public:
    QCounter() : _widget(new QLabel(0)), _count(0) {}
    ~QCounter() {}
  public:
    QLabel* widget() const { return _widget; }
  public:
    unsigned long long value() const { return _count; }
    void reset    () { _count=0; }
    void increment() { _count++; }
    void decrement() { _count--; }
    void increment(unsigned n) { _count+=n; }
    void insert   (unsigned n) { _count|=n; }
    void insert   (unsigned n, unsigned d) { _count=n; _count<<=32; _count|=d; }
    void update_bytes() { 
      unsigned long long c = _count;
      QString unit;
      if      (c < 10000ULL   )    { unit=QString("bytes"); }
      else if (c < 10000000ULL)    { c /= 1000ULL; unit=QString("kBytes"); }
      else if (c < 10000000000ULL) { c /= 1000000ULL; unit=QString("MBytes"); }
      else                         { c /= 1000000000ULL; unit=QString("GBytes"); }
      _widget->setText(QString("%1 %2").arg(c).arg(unit)); 
    }
    void update_count() { _widget->setText(QString::number(_count)); }
    void update_time () { _widget->setText(QString("%1:%2:%3").arg(_count/3600).arg((_count%3600)/60).arg(_count%60)); }
    void update_mask () { _widget->setText(QString::number(_count,0x10)); }
    void update_frac () { _widget->setText(QString("%1/%2").arg(QString::number(_count>>32)).arg(QString::number(_count&0xffffffff))); }
    unsigned long long get_count() { return _count; }
  private:
    QLabel* _widget;
    unsigned long long _count;
  };
};  

