#ifndef SESSION_H
#define SESSION_H

#include <QString>
#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegExp>
#include <float.h>

class Session
{
public:
    Session(const QString& sessionFileName);

    QStringList runs() const;
    QStringList dps() const;
    QString device() const;
    QString presentation() const;
    double timeMatchTolerance() const;
    double frequency() const;
    QString title1() const { return _titles.at(0); }
    QString title2() const { return _titles.at(1); }
    QString title3() const { return _titles.at(2); }
    QString title4() const { return _titles.at(3); }
    QString color1() const { return _colors.at(0); }
    QString color2() const { return _colors.at(1); }
    QString color3() const { return _colors.at(2); }
    QString color4() const { return _colors.at(3); }
    QString color5() const { return _colors.at(4); }
    QString color6() const { return _colors.at(5); }
    QString color7() const { return _colors.at(6); }
    QString legend1() const { return _legendLabels.at(0); }
    QString legend2() const { return _legendLabels.at(1); }
    QString legend3() const { return _legendLabels.at(2); }
    QString legend4() const { return _legendLabels.at(3); }
    QString legend5() const { return _legendLabels.at(4); }
    QString legend6() const { return _legendLabels.at(5); }
    QString legend7() const { return _legendLabels.at(6); }
    QString foreground() const { return _fg; }
    QString background() const { return _bg; }
    double start() const { return _start; }
    double stop()  const { return _stop; }
    QString orient() const { return _orient; }

private:
    Session();

    QStringList _runs;
    QStringList _dps;
    QString _device;
    QString _presentation;
    double _timeMatchTolerance;
    double _frequency;
    QStringList _titles;
    QStringList _colors;
    QStringList _legendLabels;
    QString _fg;
    QString _bg;
    double _start;
    double _stop;
    QString _orient;
};

#endif // SESSION_H
