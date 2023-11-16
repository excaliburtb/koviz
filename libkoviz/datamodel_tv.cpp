#include "datamodel_tv.h"

TVModel::TVModel(const QString& host, int port, QObject *parent) :
    DataModel(QStringList(), "", parent),
    _host(host),_port(port),
    _timeCol(0), _iteratorTimeIndex(0)
{
    _iteratorTimeIndex = new TVModelIterator(0,this,
                                              _timeCol,_timeCol,_timeCol);
    _init();
}

void TVModel::map()
{
}

void TVModel::unmap()
{
}

ModelIterator *TVModel::begin(int tcol, int xcol, int ycol) const
{
    return new TVModelIterator(0,this,tcol,xcol,ycol);
}

TVModel::~TVModel()
{
    foreach ( TVParam* param, _params ) {
        delete param;
    }
}

const Parameter* TVModel::param(int col) const
{
    return _params.at(col);
}

int TVModel::paramColumn(const QString &paramName) const
{
    int col = 0;
    bool isFound = false;
    foreach ( TVParam* param, _params ) {
        if ( param->name() == paramName ) {
            isFound = true;
            break;
        }
        ++col;
    }
    if ( !isFound ) {
        col = -1;
    }
    return col;
}

int TVModel::indexAtTime(double time)
{
    return _idxAtTimeBinarySearch(_iteratorTimeIndex,0,rowCount()-1,time);
}


int TVModel::_idxAtTimeBinarySearch (TVModelIterator* it,
                                       int low, int high, double time)
{
        if (high <= 0 ) {
                return 0;
        }
        if (low >= high) {
                // Time not found, choose closest near high
                double t1 = it->at(high-1)->t();
                double t2 = it->at(high)->t();
                double t3 = t2;
                it = it->at(high+1);
                if ( !it->isDone() ) {
                    t3 = it->at(high+1)->t();
                }

                int i;
                if ( qAbs(time-t1) < qAbs(time-t2) ) {
                    if ( qAbs(time-t1) < qAbs(time-t3) ) {
                        i = high-1;
                    } else {
                        if ( !it->isDone() ) {
                            i = high+1;
                        } else {
                            i = high;
                        }
                    }
                } else {
                    if ( qAbs(time-t2) < qAbs(time-t3) ) {
                        i = high;
                    } else {
                        if ( !it->isDone() ) {
                            i = high+1;
                        } else {
                            i = high;
                        }
                    }
                }
                return i;
        } else {
                int mid = (low + high)/2;
                if (time == it->at(mid)->t()) {
                        return mid;
                } else if ( time < it->at(mid)->t() ) {
                        return _idxAtTimeBinarySearch(it,
                                                      low, mid-1, time);
                } else {
                        return _idxAtTimeBinarySearch(it,
                                                      mid+1, high, time);
                }
        }
}

int TVModel::rowCount(const QModelIndex &pidx) const
{
    int nRows = 0;

    if ( ! pidx.isValid() ) {
        // nRows is max param.values.size()
        foreach ( TVParam* param, _params ) {
            if ( param->values.size() > nRows ) {
                nRows = param->values.size();
            }
        }
    }

    return nRows;
}

int TVModel::columnCount(const QModelIndex &pidx) const
{
    if ( ! pidx.isValid() ) {
        return _params.size();
    } else {
        return 0;
    }
}

QVariant TVModel::data(const QModelIndex &idx, int role) const
{
    QVariant val;

    Q_UNUSED(role);

    int max = 0;
    foreach (TVParam* param, _params) {
        if ( param->values.size() > max ) {
            max = param->values.size();
        }
    }

    int offset = max - _params.at(idx.column())->values.size();

    int nValues = _params.at(idx.column())->values.size();
    if ( idx.row() >= offset && idx.row() < offset+nValues ) {
        val = _params.at(idx.column())->values.at(idx.row()-offset);
    }

    return val;
}

// Only appending supported
bool TVModel::insertRows(int row, int count, const QModelIndex &parent)
{
    bool isInsertedRows = false;

    if ( ! parent.isValid() && count > 0 ) {
        int nRows = rowCount(parent);
        if ( row == nRows ) {
            // Append rows
            beginInsertRows(parent,row,row+count-1);
            QList<QVariant> emptyValues;
            for ( int i = 0; i < count; ++i ) {
                QVariant emptyValue;
                emptyValues.append(emptyValue);
            }
            foreach ( TVParam* param, _params ) {
                param->values.append(emptyValues);
            }
            isInsertedRows = true;
            endInsertRows();
        } else {
            fprintf(stderr, "koviz [bad scoobs]: TVModel::insertRows "
                            "append not supported!\n");
            exit(-1);
        }
    }

    return isInsertedRows;
}

void TVModel::addParam(const QString &paramName, const QString& unit)
{
    TVParam* param = new TVParam(paramName,unit);

    _params.append(param);

    QString msg = QString("trick.var_add(\"%1\")\n").arg(paramName);
    _vsSocketParamValues.write(msg.toUtf8());
}

// Returns the alloced size (num elements) of the parameter
int TVModel::paramSize(const QString &paramName)
{
    int sz = -1;

    QEventLoop loop;

    // Use a QTimer for a possible timeout waiting on vs server
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(2000);

    // If the event loop times out, break out of the event loop
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

    // If the socket is ready to read, break out of the event loop
    connect(&_vsSocketParamSizes,SIGNAL(readyRead()), &loop, SLOT(quit()));

    // Send the msg
    QString msg = QString("sys.stdout.write("
                          "str(trick.get_size(trick.get_address(\"%1\")))"
                          ")\n").arg(paramName);
    _vsSocketParamSizes.write(msg.toUtf8());

    // Loop until socket read or timeout
    loop.exec();

    if ( timer.isActive() ) {
        // Message received before timer timeout
        while (_vsSocketParamSizes.bytesAvailable() > 0) {
            QByteArray line1 = _vsSocketParamSizes.readLine();
            Q_UNUSED(line1);
            QByteArray line2 = _vsSocketParamSizes.readLine();
            QString msg(line2);
            bool ok;
            sz = msg.toDouble(&ok);
            if ( !ok ) {
                sz = -1;
            }
        }
    } else {
        // Timeout occured.  No message from var server, so returning -1
        fprintf(stderr, "koviz [error]: No msg from var server!\n");
    }

    return sz;
}

void TVModel::_init()
{
    connect(&_vsSocketParamValues,SIGNAL(readyRead()),
            this,SLOT(_vsReadParamValues()));

    _vsSocketParamValues.connectToHost(_host,_port);
    if (!_vsSocketParamValues.waitForConnected(1000)) {
        fprintf(stderr, "koviz [error]: "
                        "Could not connect to trick var server on"
                        "host=%s port=%d!\n",
                _host.toLatin1().constData(),_port);
        return;
    }

    addParam("time", "s");

    connect(&_vsSocketParamSizes,SIGNAL(readyRead()),
            this,SLOT(_vsReadParamSizes()));
    _vsSocketParamSizes.connectToHost(_host,_port);
    if (!_vsSocketParamSizes.waitForConnected(1000)) {
        fprintf(stderr, "koviz [error]: "
                        "Could not connect to trick var server on"
                        "host=%s port=%d!\n",
                _host.toLatin1().constData(),_port);
        return;
    }
    QString msg = QString("trick.var_set_send_stdio(True)\n");
    _vsSocketParamSizes.write(msg.toUtf8());
}

void TVModel::_vsReadParamValues()
{
    while (_vsSocketParamValues.bytesAvailable() > 0) {

        QList<QVariant> values = _vsReadParamValuesLine();
        if (_params.size() < values.size() ) {
            fprintf(stderr, "koviz [bad scoobs]: TVModel::_vsRead() "
                    "num values read greater than num params!!!\n");
            exit(-1);
        }

        bool isChange = false;
        int i = 0;
        foreach (QVariant value, values) {
            if ( _params.at(i)->values.isEmpty() ) {
                isChange = true;
                break;
            }
            if ( _params.at(i)->values.last() != value ) {
                isChange = true;
                break;
            }
            ++i;
        }

        if ( isChange ) {
            int start = rowCount();
            int end = start;
            beginInsertRows(QModelIndex(),start,end);
            i = 0;
            foreach (QVariant value, values) {
                _params.at(i++)->values.append(value);
            }
            endInsertRows();
        }
    }
}

QList<QVariant> TVModel::_vsReadParamValuesLine()
{
    QList<QVariant> values;

    QByteArray bytes = _vsSocketParamValues.readLine();
    QString msg(bytes);
    QStringList fields = msg.split('\t');

    int i = 0;
    foreach ( QString field, fields ) {
        if ( i == 0 ) { // msg type
            if ( field != "0" ) {
                break;
            } else {
                ++i;
                continue;
            }
        }
        int j = field.indexOf('{') ;
        if ( j >= 0 ) {
            field = field.left(j).trimmed();
        }

        QVariant v(field);
        values.append(v);
        ++i;
    }

    return values;
}
