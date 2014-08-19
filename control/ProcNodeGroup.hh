#ifndef Pds_ProcNodeGroup_hh
#define Pds_ProcNodeGroup_hh

#include "pdsapp/control/NodeGroup.hh"

class QCheckBox;
class QComboBox;
class QPushButton;
class QFileDialog;
class QPalette;
class QHBoxLayout;

namespace Pds {

  class ProcNodeGroup : public NodeGroup {
    Q_OBJECT
  public:
    ProcNodeGroup(const QString& label, 
		  QWidget*       parent, 
		  unsigned       platform, 
		  int            iUseReadoutGroup = 0, 
		  bool           useTransient=false);
    ~ProcNodeGroup();
  public:
    bool    useL3F   () const;
    bool    useVeto  () const;
    QString inputData() const;
  protected:
    int order(const NodeSelect& node, const QString& text);
  private:
    void    _read_pref ();
    void    _write_pref() const;
  public slots:
    void select_file();
    void action_change(int);
  private:
    QHBoxLayout* _l3f_layout;
    QCheckBox*   _l3f_box;
    QPushButton* _l3f_data;
    QComboBox*   _l3f_action;
    QFileDialog* _input_data;
    QPalette*    _palette[3];
    bool         _triggered;
  };
}; // namespace Pds

#endif
