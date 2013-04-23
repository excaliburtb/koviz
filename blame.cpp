#include "blame.h"

#include <stdio.h>
#include <ctype.h>
#include <QDir>
#include <QFile>
#include <QRegExp>
#include <QDebug>

#include <stdio.h>

Jobs::Jobs(const QString &rundir)
{
    //_parse_s_job_execution(rundir);
    _parse_log_userjobs(rundir);
    _parse_log_trickjobs(rundir);

    QList<QPair<double, long> > ovs = _parse_log_frame(rundir);
    for ( int ii = 0 ; ii < 10 ; ++ii ) {
        QPair<double,long> ov = ovs.at(ii);
        double tt = ov.first;
        double ot = ov.second;
        fprintf(stderr,"Spike at time %g of %g\n", tt ,ot/1000000);
        QList<QPair<QString,long> > snaps = snapshot(tt);
        int tidx = _river_userjobs->getIndexAtTime(&tt);
        fprintf(stderr, "    %-10s %-10s     %-50s\n",
                        "thread",
                        "time",
                        "job");
        double total = 0 ;

        //fprintf(stderr,"num jobs= %d\n",_id_to_job.values().length());
        for ( int jj = 0 ; jj < 20 ; ++jj ) {
            QPair<QString,long> snap = snaps.at(jj);
            Job* job = _id_to_job.value(snap.first);
            double t =  job->runtime[tidx]/1000000;
            double delta = 1.0e-3;
            char format[64];
            if ( t < delta ) {
                strcpy(format,"    %-10d %-10lf     %-50s\n");
            } else {
                strcpy(format,"    %-10d %-10.3lf     %-50s\n");
            }

            fprintf(stderr,format,
                    job->thread_id(),t,
                    job->job_name().toAscii().constData()) ;


            // limit printout of jobs
            if ( job->job_name() !=
                 QString("trick_sys.sched.advance_sim_time")) {
                total += t;
            }
            if ( total > 0.75*ot/1000000 && jj > 4 ) {
                break;
            }

        }
        fprintf(stderr, "    tot=%g %g\n", total, 0.75*ot/1000000);
        fprintf(stderr,"\n\n");
    }
}

Jobs::~Jobs()
{
    foreach ( Job* job, _id_to_job.values() ) {
        delete job;
    }

    delete _river_userjobs;
}

bool runTimeGreaterThan(const QPair<QString,long> &a,
                     const QPair<QString,long> &b)
{
    return a.second > b.second;
}

QList<QPair<QString, long> > Jobs::snapshot(double t)
{
    QList<QPair<QString,long> > runtimes;
    int tidx = _river_userjobs->getIndexAtTime(&t);
    foreach ( QString id, _id_to_job.keys() ) {
        Job* job = _id_to_job.value(id);
        long rt = (long)job->runtime[tidx];
        runtimes.append(qMakePair(id,rt));
    }
    qSort(runtimes.begin(), runtimes.end(), runTimeGreaterThan);
    return runtimes;
}

QMap<int,QList<Job*> > Jobs::processor_snapshot(double t)
{
    QMap<int,QList<Job*> > snapshot;
    t =0;
    return snapshot;
}

Job::Job(const QString& job_name,  // e.g. simbus.SimBus::read_ObcsRouter
         const QString& job_num,   // e.g. 1828.00
         int thread_id,            // e.g. 1
         int processor_id,         // e.g. 1
         double freq,              // e.g. 0.100
         double start,             // e.g. 0.0
         double stop,              // e.g. 1.0e37
         const QString& job_class, // e.g.scheduled
         bool is_enabled,          // e.g. 1
         int phase)                // e.g. 60000
    : _job_name(job_name),_job_num(job_num), _thread_id(thread_id),
      _processor_id(processor_id),_freq(freq),_start(start),
      _stop(stop),_job_class(job_class),_is_enabled(is_enabled),
      _phase(phase)
{
}

// Parse long logname and set job members accordingly
// An example logname:
// JOB_schedbus.SimBus##read_ALDS15_ObcsRouter_C1.1828.00(read_simbus_0.100)
Job::Job(const QString& log_jobname) :
    _start(0),_stop(1.0e37),_is_enabled(true),_phase(60000)
{
    QString qname(log_jobname);

    qname.replace("::","##");
    qname.replace(QRegExp("^JOB_"),"");
    int idx1 = qname.lastIndexOf (QChar('('));
    int idx2 = qname.lastIndexOf (QChar(')'));
    int idx3 = qname.lastIndexOf (QChar('_'));
    if ( idx3 > idx1 && isdigit(qname.at(idx3+1).isDigit()) ) {
        // frequency specified e.g. (read_simbus_0.100)
        _job_class = qname.mid(idx1+1,idx3-idx1-1);
        _freq = qname.mid(idx3+1,idx2-idx3-1).toDouble();
    } else {
        // frequency not specified e.g. (derivative)
        _job_class = qname.mid(idx1+1,idx2-idx1-1);
        _freq = -1.0;
    }

    // e.g. 1828.00 from example name
    int idx4 = qname.lastIndexOf(QChar('.'),idx1);
    int idx5 = qname.lastIndexOf(QChar('.'),idx4-1);
    _job_num = qname.mid(idx5+1,idx1-idx5-1);

    // child/thread id
    _thread_id = 0 ;
    QString stid;
    int idx6;
    for ( idx6 = idx5-1 ; idx6 > 0 ; idx6-- ) {
        if ( isdigit(qname.at(idx6).toAscii()) ) {
            stid.prepend(qname.at(idx6));
        } else {
            if ( qname.at(idx6) == 'C' && qname.at(idx6-1) == '_' ) {
                _thread_id = stid.toInt();
                idx6--;
            } else {
                idx6++;
            }
            break;
        }
    }

    _job_name = qname.mid(0,idx6);
}

QString Job::id() const
{
    char buf[256];                     // freq rouned to 3 decimal places
    QString logname = "JOB_";
    logname += _job_name;
    logname = logname.replace("::","##");

     // hope it is thread_id and not processor id (TODO)
    if ( _thread_id != 0 ) {
        logname += "_C";
        QString tid = QString("%1").arg(_thread_id);
        logname += tid;
    }

    logname += ".";
    logname += _job_num;
    logname += "(";
    logname += _job_class;
    if ( _freq > -0.5 ) {
        // Some jobs have no freqency spec in the id
        logname += "_";
        sprintf(buf,"%.3lf",_freq);
        logname += QString(buf);
    }
    logname += ")";

    return logname;
}

bool Jobs::_parse_s_job_execution(const QString &rundir)
{
    bool ret = true;

    QDir dir(rundir);
    if ( ! dir.exists() ) {
        qDebug() << "couldn't find run directory: " << rundir;
        ret = false;
        return(ret);
    }

    QString fname = rundir + QString("/S_job_execution");
    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "couldn't read file: " << fname;
        ret = false;
        return(ret);
    }

    QTextStream in(&file);
    int thread_id = -2;
    while (!in.atEnd()) {
        QString line = in.readLine();
        if ( line.startsWith("=====") )  {
            thread_id = -1;
            continue;
        }
        if ( line.startsWith("Integration Loop") )  {
            thread_id = 0;
            continue;
        }
        if ( line.startsWith("Thread") )  {
            QStringList list = line.split(QRegExp("\\W+"));
            thread_id = list[1].toInt() ;
            continue;
        }
        if ( thread_id < 0 ) continue ;

        QStringList list =  line.split("|") ;
        if ( list.length() != 9 ) continue ;

        QString job_name; QString job_num; double freq; double start;
        double stop; QString job_class; bool is_enabled; int phase;
        int processor_id;

        for ( int ii = 0 ; ii < list.size(); ++ii) {
            QString str = list.at(ii).trimmed();
            switch (ii) {
            case 0: is_enabled = str.toInt() ? true : false; break;
            case 1: processor_id = str.toInt(); break;
            case 2: job_class = str; break;
            case 3: phase = str.toInt(); break;
            case 4: start = str.toDouble(); break;
            case 5: freq = str.toDouble(); break;
            case 6: stop = str.toDouble(); break;
            case 7: job_num = str; break;
            case 8: job_name = str; break;
            };
        }

        Job* job = new Job(job_name, job_num, thread_id, processor_id,
                            freq, start, stop, job_class, is_enabled,phase);

        _id_to_job[job->id()] = job;
    }

    return ret;
}

bool Jobs::_parse_log_userjobs(const QString &rundir)
{
    bool ret = true;

    QDir dir(rundir);
    if ( ! dir.exists() ) {
        qDebug() << "couldn't find run directory: " << rundir;
        ret = false;
        return(ret);
    }

    QString trk = rundir + QString("/log_userjobs.trk");
    QFile file(trk);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "couldn't read file: " << trk;
        ret = false;
        return(ret);
    } else {
        file.close();
    }

    _river_userjobs = new TrickBinaryRiver(trk.toAscii().data());
    std::vector<LOG_PARAM> params = _river_userjobs->getParamList();
    for ( unsigned int ii = 1 ; ii < params.size(); ++ii ) {

        LOG_PARAM param = params.at(ii);

        Job* job = new Job(const_cast<char*>(param.data.name.c_str()));

        QString job_id = job->id();

        if (  _id_to_job.contains(job_id) ) {
            // Since job already created and most likely has more info
            // e.g. parsing S_job_execution has more info e.g. phase
            delete job;
            job = _id_to_job.value(job_id);
        } else {
            // Job (for reasons I don't understand) is not in S_job_execution
            _id_to_job[job_id] = job;
        }

        job->npoints = _river_userjobs->getNumPoints();
        job->timestamps = _river_userjobs->getTimeStamps();
        job->runtime = _river_userjobs->getVals(const_cast<char*>
                                                 (param.data.name.c_str()));

        if ( job->job_name() == "v1553_rws_sim.v1553_TS21_125ms_job" ) {
            double avg = 0 ;
            double sum = 0 ;
            for ( int ii = 0 ; ii < job->npoints ; ++ii ) {
                double rt = job->runtime[ii]/1000000;
                if ( rt < 1 ) {
                    sum += rt;
                }
            }
            avg = sum/job->npoints;
            fprintf(stderr, "Avg runtime for v1553_rws job is %g\n", avg);
        }
    }

    return ret;
}

bool Jobs::_parse_log_trickjobs(const QString &rundir)
{
    bool ret = true;

    QDir dir(rundir);
    if ( ! dir.exists() ) {
        qDebug() << "couldn't find run directory: " << rundir;
        ret = false;
        return(ret);
    }

    QString trk = rundir + QString("/log_trickjobs.trk");
    QFile file(trk);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "couldn't read file: " << trk;
        ret = false;
        return(ret);
    } else {
        file.close();
    }

    _river_trickjobs = new TrickBinaryRiver(trk.toAscii().data());
    std::vector<LOG_PARAM> params = _river_trickjobs->getParamList();
    for ( unsigned int ii = 1 ; ii < params.size(); ++ii ) {

        LOG_PARAM param = params.at(ii);

        Job* job = new Job(const_cast<char*>(param.data.name.c_str()));

        QString job_id = job->id();

        if (  _id_to_job.contains(job_id) ) {
            // Since job already created and most likely has more info
            // e.g. parsing S_job_execution has more info e.g. phase
            delete job;
            job = _id_to_job.value(job_id);
        } else {
            _id_to_job[job_id] = job;
        }

        job->npoints = _river_trickjobs->getNumPoints();
        job->timestamps = _river_trickjobs->getTimeStamps();
        job->runtime = _river_trickjobs->getVals(const_cast<char*>
                                                 (param.data.name.c_str()));
    }

    return ret;
}

bool dlGreaterThan(const QPair<double,long> &a,
                    const QPair<double,long> &b)
{
    return a.second > b.second;
}


QList<QPair<double, long> > Jobs::_parse_log_frame(const QString &rundir)
{
    QList<QPair<double, long> > overruns;

    QDir dir(rundir);
    if ( ! dir.exists() ) {
        qDebug() << "couldn't find run directory: " << rundir;
        return(overruns);
    }

    QString trk = rundir + QString("/log_frame.trk");
    QFile file(trk);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "couldn't read file: " << trk;
        return(overruns);
    } else {
        file.close();
    }

    _river_frame = new TrickBinaryRiver(trk.toAscii().data());
    std::vector<LOG_PARAM> params = _river_frame->getParamList();
    QString param_overrun("real_time.rt_sync.frame_sched_time");
                          //real_time.rt_sync.frame_overrun_time");
    int npoints = _river_frame->getNumPoints();
    double* timestamps = _river_frame->getTimeStamps();
    double* overrun = 0 ;
    for ( unsigned int ii = 1 ; ii < params.size(); ++ii ) {

        LOG_PARAM param = params.at(ii);

        QString qparam(param.data.name.c_str());
        if ( qparam !=  param_overrun ) {
            continue;
        }

        overrun = _river_frame->getVals(const_cast<char*>
                                        (param.data.name.c_str()));
    }

    for ( int ii = 0 ; ii < npoints ; ++ii ) {
        double tt = timestamps[ii];
        long ot = (long) overrun[ii];
        overruns.append(qMakePair(tt,ot));
    }

    qSort(overruns.begin(), overruns.end(), dlGreaterThan);

    return overruns;
}

