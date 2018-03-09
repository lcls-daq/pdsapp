#ifndef PdsConfigDb_Epix10kaCalibMap_hh
#define PdsConfigDb_Epix10kaCalibMap_hh

#include "ndarray/ndarray.h"
#include "pdsapp/config/Parameters.hh"

#include <QtGui/QButtonGroup>
#include <QtGui/QLabel>
#include <QtGui/QDialog>
#include <QtCore/QObject>

#include <stdint.h>

class QWidget;
class QVBoxLayout;

namespace Pds_ConfigDb
{
  class Epix10kaCalibDisplay;

  class Epix10kaCalibMapDialog : public QDialog {
    Q_OBJECT

    public:
      Epix10kaCalibMapDialog(ndarray<uint8_t, 2>& m, QWidget* parent=0);
      virtual ~Epix10kaCalibMapDialog();

      void show_map ();

    signals:
      void dummy();


    private slots:
      void clearClicked();
      void exportClicked();
      void importClicked();
      void inQTthreadPlease();

    private:
      QPushButton* clearButton;
      QPushButton* exportButton;
      QPushButton* importButton;
      QPushButton* quitButton;

    private:
      ndarray<uint8_t, 2>& _map;
      Epix10kaCalibDisplay* _display;
  };
}

#endif
