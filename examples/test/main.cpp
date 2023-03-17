#include <QApplication>
#include <QDate>
#include <QLocale>
#include <QScrollArea>
#include <QGridLayout>
#include <QLabel>
#include <QIcon>
#include <QMap>
#include <QDoubleSpinBox>

#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"

#include "qteditorfactory.h"
#include "qtbuttonpropertybrowser.h"
#include "qtgroupboxpropertybrowser.h"

#include "mainwindow.h"
#include "mycontroller.h"


class DecoratedDoublePropertyManager : public QtDoublePropertyManager
{
	Q_OBJECT
public:
	DecoratedDoublePropertyManager(QObject* parent = 0);
	~DecoratedDoublePropertyManager();

	QString prefix(const QtProperty* property) const;
	QString suffix(const QtProperty* property) const;
public Q_SLOTS:
	void setPrefix(QtProperty* property, const QString& prefix);
	void setSuffix(QtProperty* property, const QString& suffix);
Q_SIGNALS:
	void prefixChanged(QtProperty* property, const QString& prefix);
	void suffixChanged(QtProperty* property, const QString& suffix);
protected:
	QString valueText(const QtProperty* property) const;
	virtual void initializeProperty(QtProperty* property);
	virtual void uninitializeProperty(QtProperty* property);
private:
	struct Data {
		QString prefix;
		QString suffix;
	};
	QMap<const QtProperty*, Data> propertyToData;
};

DecoratedDoublePropertyManager::DecoratedDoublePropertyManager(QObject* parent)
	: QtDoublePropertyManager(parent)
{
}

DecoratedDoublePropertyManager::~DecoratedDoublePropertyManager()
{
}

QString DecoratedDoublePropertyManager::prefix(const QtProperty* property) const
{
	if (!propertyToData.contains(property))
		return QString();
	return propertyToData[property].prefix;
}

QString DecoratedDoublePropertyManager::suffix(const QtProperty* property) const
{
	if (!propertyToData.contains(property))
		return QString();
	return propertyToData[property].suffix;
}

void DecoratedDoublePropertyManager::setPrefix(QtProperty* property, const QString& prefix)
{
	if (!propertyToData.contains(property))
		return;

	DecoratedDoublePropertyManager::Data data = propertyToData[property];
	if (data.prefix == prefix)
		return;

	data.prefix = prefix;
	propertyToData[property] = data;

	emit propertyChanged(property);
	emit prefixChanged(property, prefix);
}

void DecoratedDoublePropertyManager::setSuffix(QtProperty* property, const QString& suffix)
{
	if (!propertyToData.contains(property))
		return;

	DecoratedDoublePropertyManager::Data data = propertyToData[property];
	if (data.suffix == suffix)
		return;

	data.suffix = suffix;
	propertyToData[property] = data;

	emit propertyChanged(property);
	emit suffixChanged(property, suffix);
}

QString DecoratedDoublePropertyManager::valueText(const QtProperty* property) const
{
	QString text = QtDoublePropertyManager::valueText(property);
	if (!propertyToData.contains(property))
		return text;

	DecoratedDoublePropertyManager::Data data = propertyToData[property];
	text = data.prefix + text + data.suffix;

	return text;
}

void DecoratedDoublePropertyManager::initializeProperty(QtProperty* property)
{
	propertyToData[property] = DecoratedDoublePropertyManager::Data();
	QtDoublePropertyManager::initializeProperty(property);
}

void DecoratedDoublePropertyManager::uninitializeProperty(QtProperty* property)
{
	propertyToData.remove(property);
	QtDoublePropertyManager::uninitializeProperty(property);
}


class DecoratedDoubleSpinBoxFactory : public QtAbstractEditorFactory<DecoratedDoublePropertyManager>
{
	Q_OBJECT
public:
	DecoratedDoubleSpinBoxFactory(QObject* parent = 0);
	~DecoratedDoubleSpinBoxFactory();
protected:
	void connectPropertyManager(DecoratedDoublePropertyManager* manager);
	QWidget* createEditor(DecoratedDoublePropertyManager* manager, QtProperty* property,
		QWidget* parent);
	void disconnectPropertyManager(DecoratedDoublePropertyManager* manager);
private slots:

	void slotPrefixChanged(QtProperty* property, const QString& prefix);
	void slotSuffixChanged(QtProperty* property, const QString& prefix);
	void slotEditorDestroyed(QObject* object);
private:
	/* We delegate responsibilities for QtDoublePropertyManager, which is a base class
	   of DecoratedDoublePropertyManager to appropriate QtDoubleSpinBoxFactory */
	QtDoubleSpinBoxFactory* originalFactory;
	QMap<QtProperty*, QList<QDoubleSpinBox*> > createdEditors;
	QMap<QDoubleSpinBox*, QtProperty*> editorToProperty;
};

DecoratedDoubleSpinBoxFactory::DecoratedDoubleSpinBoxFactory(QObject* parent)
	: QtAbstractEditorFactory<DecoratedDoublePropertyManager>(parent)
{
	originalFactory = new QtDoubleSpinBoxFactory(this);
}

DecoratedDoubleSpinBoxFactory::~DecoratedDoubleSpinBoxFactory()
{
	// not need to delete editors because they will be deleted by originalFactory in its destructor
}

void DecoratedDoubleSpinBoxFactory::connectPropertyManager(DecoratedDoublePropertyManager* manager)
{
	originalFactory->addPropertyManager(manager);
	connect(manager, SIGNAL(prefixChanged(QtProperty*, const QString&)), this, SLOT(slotPrefixChanged(QtProperty*, const QString&)));
	connect(manager, SIGNAL(suffixChanged(QtProperty*, const QString&)), this, SLOT(slotSuffixChanged(QtProperty*, const QString&)));
}

QWidget* DecoratedDoubleSpinBoxFactory::createEditor(DecoratedDoublePropertyManager* manager, QtProperty* property,
	QWidget* parent)
{
	QtAbstractEditorFactoryBase* base = originalFactory;
	QWidget* w = base->createEditor(property, parent);
	if (!w)
		return 0;

	QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(w);
	if (!spinBox)
		return 0;

	spinBox->setPrefix(manager->prefix(property));
	spinBox->setSuffix(manager->suffix(property));

	createdEditors[property].append(spinBox);
	editorToProperty[spinBox] = property;

	return spinBox;
}

void DecoratedDoubleSpinBoxFactory::disconnectPropertyManager(DecoratedDoublePropertyManager* manager)
{
	originalFactory->removePropertyManager(manager);
	disconnect(manager, SIGNAL(prefixChanged(QtProperty*, const QString&)), this, SLOT(slotPrefixChanged(QtProperty*, const QString&)));
	disconnect(manager, SIGNAL(suffixChanged(QtProperty*, const QString&)), this, SLOT(slotSuffixChanged(QtProperty*, const QString&)));
}

void DecoratedDoubleSpinBoxFactory::slotPrefixChanged(QtProperty* property, const QString& prefix)
{
	if (!createdEditors.contains(property))
		return;

	DecoratedDoublePropertyManager* manager = propertyManager(property);
	if (!manager)
		return;

	QList<QDoubleSpinBox*> editors = createdEditors[property];
	QListIterator<QDoubleSpinBox*> itEditor(editors);
	while (itEditor.hasNext()) {
		QDoubleSpinBox* editor = itEditor.next();
		editor->setPrefix(prefix);
	}
}

void DecoratedDoubleSpinBoxFactory::slotSuffixChanged(QtProperty* property, const QString& prefix)
{
	if (!createdEditors.contains(property))
		return;

	DecoratedDoublePropertyManager* manager = propertyManager(property);
	if (!manager)
		return;

	QList<QDoubleSpinBox*> editors = createdEditors[property];
	QListIterator<QDoubleSpinBox*> itEditor(editors);
	while (itEditor.hasNext()) {
		QDoubleSpinBox* editor = itEditor.next();
		editor->setSuffix(prefix);
	}
}

void DecoratedDoubleSpinBoxFactory::slotEditorDestroyed(QObject* object)
{
	QMap<QDoubleSpinBox*, QtProperty*>::ConstIterator itEditor =
		editorToProperty.constBegin();
	while (itEditor != editorToProperty.constEnd()) {
		if (itEditor.key() == object) {
			QDoubleSpinBox* editor = itEditor.key();
			QtProperty* property = itEditor.value();
			editorToProperty.remove(editor);
			createdEditors[property].removeAll(editor);
			if (createdEditors[property].isEmpty())
				createdEditors.remove(property);
			return;
		}
		itEditor++;
	}
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    MainWindow mw;
    mw.show();

	MyController controller;
	controller.show();

	if (1)
    {
		QtVariantPropertyManager* variantManager = new QtVariantPropertyManager();

		int i = 0;
		QtProperty* topItem = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(),
			QString::number(i++) + QLatin1String(" Group Property"));

		QtVariantProperty* item = variantManager->addProperty(QVariant::Bool, QString::number(i++) + QLatin1String(" Bool Property"));
		item->setValue(true);
		topItem->addSubProperty(item);

		item = variantManager->addProperty(QVariant::Int, QString::number(i++) + QLatin1String(" Int Property"));
		item->setValue(20);
		item->setAttribute(QLatin1String("minimum"), 0);
		item->setAttribute(QLatin1String("maximum"), 100);
		item->setAttribute(QLatin1String("singleStep"), 10);
		topItem->addSubProperty(item);

		item = variantManager->addProperty(QVariant::Double, QString::number(i++) + QLatin1String(" Double Property"));
		item->setValue(1.2345);
		item->setAttribute(QLatin1String("singleStep"), 0.1);
		item->setAttribute(QLatin1String("decimals"), 3);
		topItem->addSubProperty(item);

		item = variantManager->addProperty(QVariant::String, QString::number(i++) + QLatin1String(" String Property"));
		item->setValue("Value");
		topItem->addSubProperty(item);

		item = variantManager->addProperty(QVariant::Date, QString::number(i++) + QLatin1String(" Date Property"));
		item->setValue(QDate::currentDate().addDays(2));
		topItem->addSubProperty(item);

		item = variantManager->addProperty(QVariant::Time, QString::number(i++) + QLatin1String(" Time Property"));
		item->setValue(QTime::currentTime());
		topItem->addSubProperty(item);

		item = variantManager->addProperty(QVariant::DateTime, QString::number(i++) + QLatin1String(" DateTime Property"));
		item->setValue(QDateTime::currentDateTime());
		topItem->addSubProperty(item);

		item = variantManager->addProperty(QVariant::KeySequence, QString::number(i++) + QLatin1String(" KeySequence Property"));
		item->setValue(QKeySequence(Qt::ControlModifier | Qt::Key_Q));
		topItem->addSubProperty(item);

		item = variantManager->addProperty(QVariant::Char, QString::number(i++) + QLatin1String(" Char Property"));
		item->setValue(QChar(386));
		topItem->addSubProperty(item);

		item = variantManager->addProperty(QVariant::Locale, QString::number(i++) + QLatin1String(" Locale Property"));
		item->setValue(QLocale(QLocale::Polish, QLocale::Poland));
		topItem->addSubProperty(item);

		item = variantManager->addProperty(QVariant::Point, QString::number(i++) + QLatin1String(" Point Property"));
		item->setValue(QPoint(10, 10));
		topItem->addSubProperty(item);

		item = variantManager->addProperty(QVariant::PointF, QString::number(i++) + QLatin1String(" PointF Property"));
		item->setValue(QPointF(1.2345, -1.23451));
		item->setAttribute(QLatin1String("decimals"), 3);
		topItem->addSubProperty(item);

		item = variantManager->addProperty(QVariant::Size, QString::number(i++) + QLatin1String(" Size Property"));
		item->setValue(QSize(20, 20));
		item->setAttribute(QLatin1String("minimum"), QSize(10, 10));
		item->setAttribute(QLatin1String("maximum"), QSize(30, 30));
		topItem->addSubProperty(item);

		item = variantManager->addProperty(QVariant::SizeF, QString::number(i++) + QLatin1String(" SizeF Property"));
		item->setValue(QSizeF(1.2345, 1.2345));
		item->setAttribute(QLatin1String("decimals"), 3);
		item->setAttribute(QLatin1String("minimum"), QSizeF(0.12, 0.34));
		item->setAttribute(QLatin1String("maximum"), QSizeF(20.56, 20.78));
		topItem->addSubProperty(item);

		item = variantManager->addProperty(QVariant::Rect, QString::number(i++) + QLatin1String(" Rect Property"));
		item->setValue(QRect(10, 10, 20, 20));
		topItem->addSubProperty(item);
		item->setAttribute(QLatin1String("constraint"), QRect(0, 0, 50, 50));

		item = variantManager->addProperty(QVariant::RectF, QString::number(i++) + QLatin1String(" RectF Property"));
		item->setValue(QRectF(1.2345, 1.2345, 1.2345, 1.2345));
		topItem->addSubProperty(item);
		item->setAttribute(QLatin1String("constraint"), QRectF(0, 0, 50, 50));
		item->setAttribute(QLatin1String("decimals"), 3);

		item = variantManager->addProperty(QtVariantPropertyManager::enumTypeId(),
			QString::number(i++) + QLatin1String(" Enum Property"));
		QStringList enumNames;
		enumNames << "Enum0" << "Enum1" << "Enum2";
		item->setAttribute(QLatin1String("enumNames"), enumNames);
		item->setValue(1);
		topItem->addSubProperty(item);

		item = variantManager->addProperty(QtVariantPropertyManager::flagTypeId(),
			QString::number(i++) + QLatin1String(" Flag Property"));
		QStringList flagNames;
		flagNames << "Flag0" << "Flag1" << "Flag2";
		item->setAttribute(QLatin1String("flagNames"), flagNames);
		item->setValue(5);
		topItem->addSubProperty(item);

		item = variantManager->addProperty(QVariant::SizePolicy, QString::number(i++) + QLatin1String(" SizePolicy Property"));
		topItem->addSubProperty(item);

		item = variantManager->addProperty(QVariant::Font, QString::number(i++) + QLatin1String(" Font Property"));
		topItem->addSubProperty(item);

		item = variantManager->addProperty(QVariant::Cursor, QString::number(i++) + QLatin1String(" Cursor Property"));
		topItem->addSubProperty(item);

		item = variantManager->addProperty(QVariant::Color, QString::number(i++) + QLatin1String(" Color Property"));
		topItem->addSubProperty(item);

		QtVariantEditorFactory* variantFactory = new QtVariantEditorFactory();

		QtTreePropertyBrowser* variantEditor = new QtTreePropertyBrowser();
		variantEditor->setFactoryForManager(variantManager, variantFactory);
		variantEditor->addProperty(topItem);
		variantEditor->setPropertiesWithoutValueMarked(true);
		variantEditor->setRootIsDecorated(false);

		variantEditor->show();
    }

	if (1)
	{
		QWidget* w = new QWidget();

		QtBoolPropertyManager* boolManager = new QtBoolPropertyManager(w);
		QtIntPropertyManager* intManager = new QtIntPropertyManager(w);
		QtStringPropertyManager* stringManager = new QtStringPropertyManager(w);
		QtSizePropertyManager* sizeManager = new QtSizePropertyManager(w);
		QtRectPropertyManager* rectManager = new QtRectPropertyManager(w);
		QtSizePolicyPropertyManager* sizePolicyManager = new QtSizePolicyPropertyManager(w);
		QtEnumPropertyManager* enumManager = new QtEnumPropertyManager(w);
		QtGroupPropertyManager* groupManager = new QtGroupPropertyManager(w);

		QtProperty* item0 = groupManager->addProperty("QObject");

		QtProperty* item1 = stringManager->addProperty("objectName");
		item0->addSubProperty(item1);

		QtProperty* item2 = boolManager->addProperty("enabled");
		item0->addSubProperty(item2);

		QtProperty* item3 = rectManager->addProperty("geometry");
		item0->addSubProperty(item3);

		QtProperty* item4 = sizePolicyManager->addProperty("sizePolicy");
		item0->addSubProperty(item4);

		QtProperty* item5 = sizeManager->addProperty("sizeIncrement");
		item0->addSubProperty(item5);

		QtProperty* item7 = boolManager->addProperty("mouseTracking");
		item0->addSubProperty(item7);

		QtProperty* item8 = enumManager->addProperty("direction");
		QStringList enumNames;
		enumNames << "Up" << "Right" << "Down" << "Left";
		enumManager->setEnumNames(item8, enumNames);
		QMap<int, QIcon> enumIcons;
		enumIcons[0] = QIcon(":/demo/images/up.png");
		enumIcons[1] = QIcon(":/demo/images/right.png");
		enumIcons[2] = QIcon(":/demo/images/down.png");
		enumIcons[3] = QIcon(":/demo/images/left.png");
		enumManager->setEnumIcons(item8, enumIcons);
		item0->addSubProperty(item8);

		QtProperty* item9 = intManager->addProperty("value");
		intManager->setRange(item9, -100, 100);
		item0->addSubProperty(item9);

		QtCheckBoxFactory* checkBoxFactory = new QtCheckBoxFactory(w);
		QtSpinBoxFactory* spinBoxFactory = new QtSpinBoxFactory(w);
		QtSliderFactory* sliderFactory = new QtSliderFactory(w);
		QtScrollBarFactory* scrollBarFactory = new QtScrollBarFactory(w);
		QtLineEditFactory* lineEditFactory = new QtLineEditFactory(w);
		QtEnumEditorFactory* comboBoxFactory = new QtEnumEditorFactory(w);

		QtAbstractPropertyBrowser* editor1 = new QtTreePropertyBrowser();
		editor1->setFactoryForManager(boolManager, checkBoxFactory);
		editor1->setFactoryForManager(intManager, spinBoxFactory);
		editor1->setFactoryForManager(stringManager, lineEditFactory);
		editor1->setFactoryForManager(sizeManager->subIntPropertyManager(), spinBoxFactory);
		editor1->setFactoryForManager(rectManager->subIntPropertyManager(), spinBoxFactory);
		editor1->setFactoryForManager(sizePolicyManager->subIntPropertyManager(), spinBoxFactory);
		editor1->setFactoryForManager(sizePolicyManager->subEnumPropertyManager(), comboBoxFactory);
		editor1->setFactoryForManager(enumManager, comboBoxFactory);

		editor1->addProperty(item0);

		QtAbstractPropertyBrowser* editor2 = new QtTreePropertyBrowser();
		editor2->addProperty(item0);

		QtAbstractPropertyBrowser* editor3 = new QtGroupBoxPropertyBrowser();
		editor3->setFactoryForManager(boolManager, checkBoxFactory);
		editor3->setFactoryForManager(intManager, spinBoxFactory);
		editor3->setFactoryForManager(stringManager, lineEditFactory);
		editor3->setFactoryForManager(sizeManager->subIntPropertyManager(), spinBoxFactory);
		editor3->setFactoryForManager(rectManager->subIntPropertyManager(), spinBoxFactory);
		editor3->setFactoryForManager(sizePolicyManager->subIntPropertyManager(), spinBoxFactory);
		editor3->setFactoryForManager(sizePolicyManager->subEnumPropertyManager(), comboBoxFactory);
		editor3->setFactoryForManager(enumManager, comboBoxFactory);

		editor3->addProperty(item0);

		QScrollArea* scroll3 = new QScrollArea();
		scroll3->setWidgetResizable(true);
		scroll3->setWidget(editor3);

		QtAbstractPropertyBrowser* editor4 = new QtGroupBoxPropertyBrowser();
		editor4->setFactoryForManager(boolManager, checkBoxFactory);
		editor4->setFactoryForManager(intManager, scrollBarFactory);
		editor4->setFactoryForManager(stringManager, lineEditFactory);
		editor4->setFactoryForManager(sizeManager->subIntPropertyManager(), spinBoxFactory);
		editor4->setFactoryForManager(rectManager->subIntPropertyManager(), spinBoxFactory);
		editor4->setFactoryForManager(sizePolicyManager->subIntPropertyManager(), sliderFactory);
		editor4->setFactoryForManager(sizePolicyManager->subEnumPropertyManager(), comboBoxFactory);
		editor4->setFactoryForManager(enumManager, comboBoxFactory);

		editor4->addProperty(item0);

		QScrollArea* scroll4 = new QScrollArea();
		scroll4->setWidgetResizable(true);
		scroll4->setWidget(editor4);

		QtAbstractPropertyBrowser* editor5 = new QtButtonPropertyBrowser();
		editor5->setFactoryForManager(boolManager, checkBoxFactory);
		editor5->setFactoryForManager(intManager, scrollBarFactory);
		editor5->setFactoryForManager(stringManager, lineEditFactory);
		editor5->setFactoryForManager(sizeManager->subIntPropertyManager(), spinBoxFactory);
		editor5->setFactoryForManager(rectManager->subIntPropertyManager(), spinBoxFactory);
		editor5->setFactoryForManager(sizePolicyManager->subIntPropertyManager(), sliderFactory);
		editor5->setFactoryForManager(sizePolicyManager->subEnumPropertyManager(), comboBoxFactory);
		editor5->setFactoryForManager(enumManager, comboBoxFactory);

		editor5->addProperty(item0);

		QScrollArea* scroll5 = new QScrollArea();
		scroll5->setWidgetResizable(true);
		scroll5->setWidget(editor5);

		QGridLayout* layout = new QGridLayout(w);
		QLabel* label1 = new QLabel("Editable Tree Property Browser");
		QLabel* label2 = new QLabel("Read Only Tree Property Browser, editor factories are not set");
		QLabel* label3 = new QLabel("Group Box Property Browser");
		QLabel* label4 = new QLabel("Group Box Property Browser with different editor factories");
		QLabel* label5 = new QLabel("Button Property Browser");
		label1->setWordWrap(true);
		label2->setWordWrap(true);
		label3->setWordWrap(true);
		label4->setWordWrap(true);
		label5->setWordWrap(true);
		label1->setFrameShadow(QFrame::Sunken);
		label2->setFrameShadow(QFrame::Sunken);
		label3->setFrameShadow(QFrame::Sunken);
		label4->setFrameShadow(QFrame::Sunken);
		label5->setFrameShadow(QFrame::Sunken);
		label1->setFrameShape(QFrame::Panel);
		label2->setFrameShape(QFrame::Panel);
		label3->setFrameShape(QFrame::Panel);
		label4->setFrameShape(QFrame::Panel);
		label5->setFrameShape(QFrame::Panel);
		label1->setAlignment(Qt::AlignCenter);
		label2->setAlignment(Qt::AlignCenter);
		label3->setAlignment(Qt::AlignCenter);
		label4->setAlignment(Qt::AlignCenter);
		label5->setAlignment(Qt::AlignCenter);

		layout->addWidget(label1, 0, 0);
		layout->addWidget(label2, 0, 1);
		layout->addWidget(label3, 0, 2);
		layout->addWidget(label4, 0, 3);
		layout->addWidget(label5, 0, 4);
		layout->addWidget(editor1, 1, 0);
		layout->addWidget(editor2, 1, 1);
		layout->addWidget(scroll3, 1, 2);
		layout->addWidget(scroll4, 1, 3);
		layout->addWidget(scroll5, 1, 4);
		w->show();
	}

	if (1)
	{
		QtDoublePropertyManager* undecoratedManager = new QtDoublePropertyManager();
		QtProperty* undecoratedProperty = undecoratedManager->addProperty("Undecorated");
		undecoratedManager->setValue(undecoratedProperty, 123.45);

		DecoratedDoublePropertyManager* decoratedManager = new DecoratedDoublePropertyManager();
		QtProperty* decoratedProperty = decoratedManager->addProperty("Decorated");
		decoratedManager->setPrefix(decoratedProperty, "speed: ");
		decoratedManager->setSuffix(decoratedProperty, " km/h");
		decoratedManager->setValue(decoratedProperty, 123.45);

		QtDoubleSpinBoxFactory* undecoratedFactory = new QtDoubleSpinBoxFactory();
		DecoratedDoubleSpinBoxFactory* decoratedFactory = new DecoratedDoubleSpinBoxFactory();

		QtTreePropertyBrowser* editor = new QtTreePropertyBrowser();
		editor->setFactoryForManager(undecoratedManager, undecoratedFactory);
		editor->setFactoryForManager(decoratedManager, decoratedFactory);
		editor->addProperty(undecoratedProperty);
		editor->addProperty(decoratedProperty);
		editor->show();
	}

    return app.exec();
}

#include "main.moc"