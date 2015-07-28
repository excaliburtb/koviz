#ifndef PLOTBOOKMODEL_H
#define PLOTBOOKMODEL_H

#include <QStandardItemModel>
#include <QStandardItem>
#include "libsnapdata/montemodel.h"
#include "libsnaprt/utils.h"

class PlotBookModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit PlotBookModel( MonteModel* monteModel,
                            QObject *parent = 0);
    explicit PlotBookModel( MonteModel* monteModel,
                            int rows, int columns, QObject * parent = 0 );

    virtual QVariant data(const QModelIndex &idx,
                          int role = Qt::DisplayRole) const;

    virtual bool setData(const QModelIndex &index,
                         const QVariant &value, int role = Qt::EditRole);

    QStandardItem* addChild(QStandardItem* parentItem,
                            const QString& childTitle,
                            const QVariant &childValue=QVariant());

    bool isIndex(const QModelIndex& idx,
                  const QString& itemText) const;

    QModelIndex getIndex(const QModelIndex& startIdx,
                        const QString& searchItemText,
                        const QString &expectedStartIdxText=QString()) const;

    QModelIndexList pageIdxs() const ;

    QModelIndexList plotIdxs(const QModelIndex& _pageIdx) const ;

    QModelIndexList curveIdxs(const QModelIndex& curvesIdx) const ;

signals:
    
public slots:

private:
    MonteModel* _monteModel;
    void _initModel();
    QModelIndex _pageIdx(const QModelIndex& idx) const ;
    QModelIndex _plotIdx(const QModelIndex& idx) const ;

};

#endif // PLOTBOOKMODEL_H
