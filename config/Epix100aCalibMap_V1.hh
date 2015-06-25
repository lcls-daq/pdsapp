#ifndef PdsConfigDb_Epix100aCalibMap_V1_hh
#define PdsConfigDb_Epix100aCalibMap_V1_hh

#include "ndarray/ndarray.h"
#include "pdsapp/config/Parameters.hh"

#include <QtGui/QButtonGroup>
#include <QtGui/QLabel>
#include <QtGui/QDialog>
#include <QtCore/QObject>

#include <stdint.h>

class QWidget;
class QVBoxLayout;

namespace Pds_ConfigDb {
  namespace V1 {
    class Epix100aCalibDisplay;

    class Epix100aCalibMapDialog : public QDialog {
      Q_OBJECT

      public:
        Epix100aCalibMapDialog(ndarray<uint8_t, 2>& m, QWidget* parent=0);
        virtual ~Epix100aCalibMapDialog();

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
        Epix100aCalibDisplay* _display;
    };
  }
}

#endif
