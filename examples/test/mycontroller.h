#pragma once

#include <QApplication>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QDialog>
#include <QComboBox>
#include <QToolButton>
#include <QPushButton>
#include <QBoxLayout>
#include <QTreeWidget>
#include <QAction>
#include <QDesktopWidget>
#include <QTextDocument>
#include <QCalendarWidget>
#include <QTimeLine>
#include "objectcontroller.h"

class MyController  : public QDialog
{
	Q_OBJECT

public:
	MyController(QWidget* parent = 0);
	~MyController();
private slots:
	void createAndControl();
private:
	QComboBox* theClassCombo;
	ObjectController* theController;
	QStringList theClassNames;
	QObject* theControlledObject;
};
