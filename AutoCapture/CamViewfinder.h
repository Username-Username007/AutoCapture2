#pragma once
#include <qcameraviewfinder.h>
class CamViewfinder :
    public QCameraViewfinder
{
    Q_OBJECT

public:
    CamViewfinder():QCameraViewfinder()
    {
        
    }
    virtual void closeEvent(QCloseEvent *ce) {
		emit onClose();
        qDebug() << "The viewfinder is closed.";
    }
signals:
    void onClose();
};

