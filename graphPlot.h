#ifndef PUPILEXT_GRAPHPLOT_H
#define PUPILEXT_GRAPHPLOT_H

/**
    @author Moritz Lode
*/

#include <QtWidgets/QWidget>
#include <QtCore/qobjectdefs.h>

#include "qcustomplot.h"
#include "Pupil.h"
#include "worker.h"  // 需要包含ProcessingResult定义
#include <QTimer>
#include <QElapsedTimer>
#include <QGridLayout>
#include <QCloseEvent>

/**
    Custom lineplot graph widget employing the QCustomPlot library for plotting

    GraphPlot(): create graph window and define window title

slots:
    appendData(): Slot for receiving respective data, depends on which data value is selected
*/
class GraphPlot : public QWidget {
    Q_OBJECT

public:

    static uint64 sharedTimestamp; // timestamp that shares every graph so the times match

    explicit GraphPlot(QString plotValue, bool stereoMode=false, bool legend=false, QWidget *parent=0);
    ~GraphPlot() override;

    QSize sizeHint() const override;
    void setupPlotAxis();

    // Getter for plotValue (needed for session state saving)
    QString getPlotValue() const { return plotValue; }

private slots:

    void reset();
    void contextMenuRequest(QPoint pos);
    void enableInteractions();
    void enableYAxisInteraction();

protected:
    void closeEvent(QCloseEvent *event) override;

signals:
    void graphPlotClosed(GraphPlot* plot);

public slots:

    void appendData(quint64 timestamp, const PupilData &pupil, const QString &filename);
    void appendData(quint64 timestamp, const PupilData &pupil, const PupilData &pupilSec, const QString &filename);
    void appendData(const ProcessingResult &result);  // 新增：接收完整的处理结果

    void appendData(const double &fps);
    void appendData(const int &framecount);

    // 新增：接收扩展特征数据
    void appendExtendedFeatureData(const QString &featureName, double value, qint64 timestamp);

private:

    QString plotValue;

    QCustomPlot *customPlot;
    QCPGraph *graph;

    QElapsedTimer timer;
    uint64 incrementedTimestamp;

    bool interaction;
    bool yinteraction;

    int updateDelay;

    // Y轴范围管理 - 只能扩展，不能缩小
    double m_currentMinY;
    double m_currentMaxY;
    bool m_yRangeInitialized;

    // 智能Y轴范围更新 - 只扩展不缩小
    void updateYAxisRangeExpansive(double value);

};

#endif //PUPILEXT_GRAPHPLOT_H
