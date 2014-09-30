#ifndef PdsConfigDb_Epix100aPixelMap_hh
#define PdsConfigDb_Epix100aPixelMap_hh

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
  class Epix100aPixelDisplay;

  class Epix100aPixelMapDialog : public QDialog {
    Q_OBJECT

    public:
      Epix100aPixelMapDialog(ndarray<uint16_t, 2>& m, QWidget* parent=0);
      virtual ~Epix100aPixelMapDialog();

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
      ndarray<uint16_t, 2>& _map;
      Epix100aPixelDisplay* _display;
  };
}

#endif
