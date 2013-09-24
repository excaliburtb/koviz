#ifndef SNAP_PLOT_H
#define SNAP_PLOT_H

#ifdef SNAPGUI

#include <QRubberBand>
#include "qplot/qcustomplot.h"
#include "trickmodel.h"
#include "trickcurve.h"

class SnapPlot : public QCustomPlot
{
  public:
    SnapPlot(QWidget* parent=0);

    TrickCurve *addCurve(TrickModel *model,
                        int tcol, int xcol, int ycol,
                        double valueScaleFactor=1.0);
    bool removeCurve(int index);
    //int clearCurves();
    int curveCount() const { return _curves.size(); }

    void zoomToFit(const QCPRange& xrange=QCPRange());

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

  private:
    QList<TrickCurve*> _curves;
    void _set_interactions();

    QPoint _origin;
    QRubberBand* _rubber_band;

    void _fitXRange();
    void _fitYRange();
};

#endif // SNAPGUI
#endif

