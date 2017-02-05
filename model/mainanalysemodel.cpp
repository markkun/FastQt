#include "mainanalysemodel.h"

MainAnalyseModel::MainAnalyseModel(QObject * parent)
    :QAbstractListModel(parent)
{
    mTimer = new QTimer(this);
    //   mSignalMapper = new QSignalMapper(this);
    //    connect(mSignalMapper,SIGNAL(mapped(int)),this,SLOT(updated(int)));
    connect(mTimer,SIGNAL(timeout()),this,SLOT(timeUpdated()));

    mTimer->setInterval(1000);

    QThreadPool::globalInstance()->setMaxThreadCount(2);
}

int MainAnalyseModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return mRunners.size();
}

int MainAnalyseModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 6;
}

QVariant MainAnalyseModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if ( role == Qt::DisplayRole)
    {
        if (index.column() == NameColumn){
            QFileInfo info(mRunners.at(index.row())->filename());
            return info.fileName();
        }

        if (index.column() == StatusColumn)
        {
            switch (mRunners.at(index.row())->status())
            {
            case AnalysisRunner::Waiting : return tr("Waiting"); break;
            case AnalysisRunner::Canceled : return tr("Canceled"); break;
            case AnalysisRunner::Running : return tr("Running"); break;
            case AnalysisRunner::Finished : return tr("Finished"); break;
            }

        }

        if (index.column() == SizeColumn)
            return mRunners.at(index.row())->humanFileSize();


        if (index.column() == ProgressColumn)
            return mRunners.at(index.row())->progression();

        if (index.column() == ReadsColumn)
            return mRunners.at(index.row())->sequenceCount();


        if (index.column() == TimeColumn){
            QTime time;
            time.setHMS(0,0,0);
            time = time.addMSecs(mRunners.at(index.row())->duration());
            return time.toString();
        }
    }

    if (role == Qt::DecorationRole)
    {
        if (index.column() == StatusColumn)
        {

            switch (mRunners.at(index.row())->status())
            {
            case AnalysisRunner::Waiting : return QFontIcon::icon(0xf017, Qt::lightGray); break;
            case AnalysisRunner::Canceled: return QFontIcon::icon(0xf071,Qt::darkRed); break;
            case AnalysisRunner::Running : return QFontIcon::icon(0xf085,Qt::darkGray); break;
            case AnalysisRunner::Finished: return QFontIcon::icon(0xf00c,Qt::darkGreen); break;
            }
        }
    }

    if (role == Qt::TextColorRole)
    {
        if (index.column() == StatusColumn)
        {

            switch (mRunners.at(index.row())->status())
            {
            case AnalysisRunner::Waiting : return QColor(Qt::lightGray); break;
            case AnalysisRunner::Canceled: return QColor(Qt::darkRed); break;
            case AnalysisRunner::Running : return QColor(Qt::darkGray); break;
            case AnalysisRunner::Finished: return QColor(Qt::darkGreen); break;
            }
        }
    }



    return QVariant();

}

QVariant MainAnalyseModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case NameColumn : return tr("Filename");break;
            case StatusColumn: return tr("Status");break;
            case SizeColumn: return tr("File size"); break;
            case ProgressColumn : return tr("Progress"); break;
            case ReadsColumn : return tr("Reads");break;
            case TimeColumn: return tr("Time"); break;
            }
        }
    }
    return QVariant();
}

void MainAnalyseModel::addFile(const QString &filename)
{
    beginInsertRows(QModelIndex(), 0,0);

    AnalysisRunner * runner = new AnalysisRunner(filename);
    runner->addAnalysis(new BasicStatsAnalysis);
    runner->addAnalysis(new PerBaseQualityAnalysis);
    runner->addAnalysis(new PerSequenceQualityAnalysis);
    //    runner->addAnalysis(new PerBaseContentAnalysis);  // create result crash with small fastq
    runner->addAnalysis(new OverRepresentedSeqsAnalysis);
    runner->addAnalysis(new PerBaseNContentAnalysis);
    runner->addAnalysis(new PerSequenceGCContent);
    runner->addAnalysis(new LengthDistributionAnalysis);

    mRunners.append(runner);
    endInsertRows();

    emit layoutChanged();

    QThreadPool::globalInstance()->start(runner);

    if (!mTimer->isActive())
        mTimer->start();

}

bool MainAnalyseModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (count == 0)
        return false;

    beginRemoveRows(parent,row, row+count-1);

    for(int i=0; i<count; ++i) {
        delete mRunners.takeAt(row);
    }

    endRemoveRows();

    return true;
}

AnalysisRunner *MainAnalyseModel::runner(const QModelIndex &index)
{
    if (!index.isValid())
        return Q_NULLPTR;

    return mRunners.at(index.row());
}



void MainAnalyseModel::timeUpdated()
{
    if (rowCount() == 0)
        return;


    QModelIndex top    = index(0, 1);
    QModelIndex bottom = index(rowCount(),columnCount());

    emit dataChanged(top, bottom);

}