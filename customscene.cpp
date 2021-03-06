#include "customscene.h"
#include <QtDebug>
#include <QScrollBar>

CustomScene::CustomScene(QObject* parent):
    QGraphicsScene(parent),
    _image(nullptr),
    _pixmapItem(nullptr),
    _boxItem(nullptr),
    _isDrawing(false),
    _undoStack(new QUndoStack),
    _clickedPos(QPointF(0,0))
{
}

void CustomScene::clearAll()
{
    saveBoxItemsToFile();

    if (_image != nullptr) {
        delete _image;
        _image = nullptr;
    }
    if (_pixmapItem != nullptr) {
        delete _pixmapItem;
        _pixmapItem = nullptr;
    }
    if (_boxItemMimeData) {
        delete _boxItemMimeData;
    }

    foreach (QGraphicsItem *item, this->items()) {
        if ((item->type() == QGraphicsItem::UserType+1) ||
                (item->type() == QGraphicsPixmapItem::Type) ) {
            this->removeItem(item);
            delete item;
        }
    }

    if (_undoStack != nullptr) {
        _undoStack->clear();
        delete _undoStack;
        _undoStack = nullptr;
    }

    this->items().clear();
    this->clear();
}

void CustomScene::loadImage(QString filename)
{
    // Get image format
    FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(filename.toLocal8Bit(), 0);
    if(fif == FIF_UNKNOWN)
        fif = FreeImage_GetFIFFromFilename(filename.toLocal8Bit());
    if(fif == FIF_UNKNOWN)
        return;

    // Load image if possible
    FIBITMAP *dib = nullptr;
    if(FreeImage_FIFSupportsReading(fif)) {
        dib = FreeImage_Load(fif, filename.toLocal8Bit());
        if(dib == nullptr)
            return;
    } else
        return;

    // Convert to 24bits and save to memory as JPEG
    FIMEMORY *stream = FreeImage_OpenMemory();
    // FreeImage can only save 24-bit highcolor or 8-bit greyscale/palette bitmaps as JPEG
    FIBITMAP *dib1 = FreeImage_ConvertTo24Bits(dib);
    FreeImage_SaveToMemory(FIF_JPEG, dib1, stream);

    // Load JPEG data
    BYTE *mem_buffer = nullptr;
    DWORD size_in_bytes = 0;
    FreeImage_AcquireMemory(stream, &mem_buffer, &size_in_bytes);

    // Load raw data into QImage and return
    QByteArray array = QByteArray::fromRawData((char*)mem_buffer, (int)size_in_bytes);

    _image = new QImage();
    _image->loadFromData(array);
    //_image = new QImage(filename);
    _pixmapItem = new QGraphicsPixmapItem(QPixmap::fromImage(*_image));
    _pixmapItem->setTransformationMode(Qt::SmoothTransformation);
    this->addItem(_pixmapItem);

    emit imageLoaded(_image->size());
    setSceneRect(_image->rect());

    // load box items
    QFileInfo info(filename);
    _boxItemFileName = info.path() + "/" + info.completeBaseName() + ".txt";
    loadBoxItemsFromFile();

    array.clear();
    FreeImage_CloseMemory(stream);
    FreeImage_Unload(dib);
    FreeImage_Unload(dib1);
}

void CustomScene::loadBoxItemsFromFile()
{
    QFile file(_boxItemFileName);
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    QPoint zero(0, 0);
    QRect fatherRect(zero, _image->size());
    qreal x,y,w,h;
    int index;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QStringList info = in.readLine().split(" ");
        if(info.size() >= 5) {
            index = info.at(0).toInt();
            w = info.at(3).toFloat() * _image->width();
            h = info.at(4).toFloat() * _image->height();
            x = info.at(1).toFloat() * _image->width() - w/2;
            y = info.at(2).toFloat() * _image->height() - h/2;

            BoxItem *b = new BoxItem(fatherRect, _image->size(), _typeNameList, _typeNameList.at(index));
            b->setRect(x,y,w,h);
            this->registerItem(b);
            if (!b->rect().isNull())
                this->addItem(b);
        }
    }
    file.close();
}

void CustomScene::registerItem(BoxItem *b)
{
    connect(b, SIGNAL(typeNameChanged(QString)), this, SLOT(changeBoxTypeName(QString)));
    connect(b, SIGNAL(boxSelected(QRect, QString)), this, SIGNAL(boxSelected(QRect, QString)));
    connect(b, SIGNAL(boxSelected(QRect, QString)), this, SLOT(selectedBoxItemInfo(QRect,QString)));
    connect(b, SIGNAL(stretchCompleted(QRectF, QRectF)), this, SLOT(moveBox(QRectF, QRectF)));
    connect(b, SIGNAL(moveCompleted(QRectF, QRectF)), this, SLOT(moveBox(QRectF, QRectF)));
    b->installEventFilter(this);
}

void CustomScene::saveBoxItemsToFile()
{
    QFile file(_boxItemFileName);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    qreal label[4];
    QTextStream out(&file);

    foreach (QGraphicsItem *item, this->items()) {
        if (item->type() == QGraphicsItem::UserType+1) {
            BoxItem *b = qgraphicsitem_cast<BoxItem *>(item);
            b->rect(label);
            QString s;
            s.sprintf("%d %f %f %f %f\n",
                      _typeNameList.indexOf(b->typeName()),
                      label[0],label[1],label[2],label[3]);
            out << s;
        }
    }
    file.close();
}

void CustomScene::deleteBoxItems()
{
    QList<QGraphicsItem *> boxList;
    foreach (QGraphicsItem *item, this->selectedItems()) {
        if (item->type() == QGraphicsItem::UserType+1) {
            boxList.append(item);
        }
    }

    if (boxList.count() > 0) {
        _undoStack->push(new RemoveBoxesCommand(this, boxList));
        QApplication::setOverrideCursor(qgraphicsitem_cast<BoxItem *>(boxList.at(0))->oldCursor());
    }
    boxList.clear();
}

void CustomScene::selectBoxItems(bool op)
{
    // selecting box items
    foreach (QGraphicsItem *item, this->items()) {
        if (item->type() == QGraphicsItem::UserType+1) {
            item->setSelected(op);
        }
    }
}

void CustomScene::selectBoxItems(BoxItem *box, bool op)
{
    foreach (QGraphicsItem *item, this->items()) {
        if (item->type() == QGraphicsItem::UserType+1) {
            item->setSelected(item==qgraphicsitem_cast<QGraphicsItem *>(box));
        }
    }
}

void CustomScene::selectBoxItems(QList<BoxItem *> *boxList, bool op)
{
    foreach (QGraphicsItem *item, this->items()) {
        if (item->type() == QGraphicsItem::UserType+1) {
            item->setSelected(boxList->contains(qgraphicsitem_cast<BoxItem *>(item)));
        }
    }
}

void CustomScene::drawBoxItem(bool op)
{
    _isDrawing = op;
    _isPanning = false;
}

void CustomScene::panImage(bool op)
{
    _isDrawing = false;
    _isPanning = op;
    selectBoxItems(false);
}

void CustomScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    switch (event->buttons()) {
    case Qt::LeftButton:
        _leftTopPoint = event->scenePos();
        if (_isDrawing && event->modifiers() != Qt::ControlModifier) { // drawing box item

            // no box selected
            int count = 0;
            foreach (QGraphicsItem *item, this->selectedItems()) {
                if (item->type() == QGraphicsItem::UserType + 1) {
                    if (item->isSelected()) {
                        count++;
                    }
                }
            }
            if (count <= 0) {
                QGraphicsScene::mousePressEvent(event);
                return;
            }

            // selected box do not contain pressed pos
            count = 0;
            foreach (QGraphicsItem *item, this->items()) {
                if (item->type() == QGraphicsItem::UserType + 1) {
                    if (item->isSelected() && !item->contains(_leftTopPoint)) {
                        item->setSelected(false);
                        count++;
                    }
                }
            }
            if (count >0) {
                QGraphicsScene::mousePressEvent(event);
                return;
            }

            // selected box contain pressed pos
            foreach (QGraphicsItem *item, this->items()) {
                if ((item->type() == QGraphicsItem::UserType + 1 && item->contains(_leftTopPoint))) {
                    _isMoving = true;
                    QGraphicsScene::mousePressEvent(event);
                    return;
                }
            }
        } else if (event->modifiers() == Qt::ControlModifier) { // selecting multiple box items
            foreach (QGraphicsItem *item, this->items()) {
                if ((item->type() == QGraphicsItem::UserType + 1 && item->contains(_leftTopPoint))) {
                    item->setSelected(true);
                    break;
                }
            }
            QGraphicsScene::mousePressEvent(event);
        } else {// selecting single box item
            foreach (QGraphicsItem *item, this->items()) {
                if ((item->type() == QGraphicsItem::UserType + 1 && item->contains(_leftTopPoint))) {
                    selectBoxItems(qgraphicsitem_cast<BoxItem *>(item), true);
                    _isMoving = true;
                    QGraphicsScene::mousePressEvent(event);
                    break;
                }
            }
        }
        break;
    case Qt::RightButton:
        QGraphicsScene::mousePressEvent(event);
        break;
    default:
        break;
    }
    QGraphicsScene::mousePressEvent(event);
}

void CustomScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        _isMouseMoved = true;

        // add new box item
        if(_isDrawing && (this->selectedItems().count() <= 0 || _boxItem) && !_isMoving && !_isPanning) {
            if(!_boxItem) {
                _boxItem = new BoxItem(this->sceneRect(), _image->size(), _typeNameList, _typeName);
                this->registerItem(_boxItem);
                this->addItem(_boxItem);
            }
            _boxItem->setSelected(true);
            _rightBottomPoint = event->scenePos();
            QRect roi(qMin(_rightBottomPoint.x(), _leftTopPoint.x()),
                      qMin(_rightBottomPoint.y(), _leftTopPoint.y()),
                      qAbs(_rightBottomPoint.x() - _leftTopPoint.x()),
                      qAbs(_rightBottomPoint.y() - _leftTopPoint.y()));
            int l = sceneRect().left(), r = sceneRect().right(),
                    t = sceneRect().top(), b = sceneRect().bottom();
            if (roi.right() < l || roi.left() > r ||
                    roi.top() > b || roi.bottom() < t )
                return;

            roi.setTop(qMax(t, roi.top()));
            roi.setLeft(qMax(l, roi.left()));
            roi.setBottom(qMin(b, roi.bottom()));
            roi.setRight(qMin(r, roi.right()));

            _boxItem->setRect(roi);
        }

        if (_isMoving) {
            QGraphicsScene::mouseMoveEvent(event);
        }
    }
    emit cursorMoved(event->scenePos());

    if(!(_isDrawing && this->selectedItems().count()<=0))
        QGraphicsScene::mouseMoveEvent(event);
}

void CustomScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (_isMouseMoved) {
        if (_isDrawing) {
            if (_boxItem) {
                this->removeItem(_boxItem);
                if ( _boxItem->rect().width() > 5 && (_boxItem->rect().height() > 5 ) ) {
                    QCursor c = Qt::CrossCursor;
                    _boxItem->setOldCursor(c);
                    _undoStack->push(new AddBoxCommand(this, _boxItem));
                }
                _boxItem = nullptr;
                return;
            }
        }
        if (_isMoving) {
            _isMoving = false;
            QGraphicsScene::mouseReleaseEvent(event);
        }

        _isMouseMoved = false;
        return;
    } else {
        if (_boxItemMimeData)
            _clickedPos = event->scenePos();
    }

    QGraphicsScene::mouseReleaseEvent(event);
}

void CustomScene::keyPressEvent(QKeyEvent *keyEvent)
{
    if(keyEvent->key() == Qt::Key_Delete) {
        deleteBoxItems();
    } else if(keyEvent->key() == Qt::Key_A && keyEvent->modifiers() == Qt::ControlModifier) {
        selectBoxItems(true);
    }else {
        QGraphicsScene::keyPressEvent(keyEvent);
    }
}

void CustomScene::keyReleaseEvent(QKeyEvent *keyEvent)
{
    QGraphicsScene::keyReleaseEvent(keyEvent);
}

void CustomScene::changeBoxTypeName(QString name)
{
    _typeName = name;
    QList<BoxItem *> *boxList = new QList<BoxItem *>();
    foreach (QGraphicsItem *item, this->selectedItems()) {
        if (item->type() == QGraphicsItem::UserType+1) {
            BoxItem *b = qgraphicsitem_cast<BoxItem *>(item);
            boxList->append(b);
        }
    }

    if (boxList->count() > 0) {
        _undoStack->push(new SetTargetTypeCommand(this, boxList, name));
    }
    delete boxList;
}

void CustomScene::moveBox(QRectF newRect, QRectF oldRect)
{
    BoxItem *item = reinterpret_cast<BoxItem *>(QObject::sender());
    if (item != nullptr) {
        _undoStack->push(new MoveBoxCommand(this, item, newRect, oldRect));
    }
}


void CustomScene::copy()
{
    if (selectedItems().count() <=0 )
        return;

    if (_boxItemMimeData) {
        delete _boxItemMimeData;
    }
    _boxItemMimeData = new BoxItemMimeData(selectedItems());
    QApplication::clipboard()->setMimeData(_boxItemMimeData);

    _pastePos.clear();
    for (int i=0; i<selectedItems().count(); i++)
        _pastePos.append(QPointF(0,0));
    _clickedPos = QPointF(0,0);
}

void CustomScene::paste()
{
    QMimeData *m = const_cast<QMimeData *>(QApplication::clipboard()->mimeData()) ;
    BoxItemMimeData *data = dynamic_cast<BoxItemMimeData*>(m);
    if (data){
        clearSelection();

        QList<QPointF> offset;
        QList<QRectF> itemRects;
        QRectF unitedRect(0,0,0,0);

        for (int i=0; i<data->items().count(); i++) {
            BoxItem *b = qgraphicsitem_cast<BoxItem*>(data->items().at(i));
            itemRects.append(b->rect());
            unitedRect = unitedRect.united(b->rect());
        }
        if (!unitedRect.isNull()) {
            for (int i=0; i<itemRects.count(); i++) {
                offset.append(itemRects[i].topLeft() - unitedRect.topLeft());
            }
        }

        for (int i=0; i<_pastePos.count(); i++) {
            if (_pastePos[i].isNull()) {
                _pastePos[i] = QPointF(itemRects[i].x(), itemRects[i].y());
            }
        }
        if (!_clickedPos.isNull()) {
            unitedRect.moveCenter(_clickedPos);
            for (int i=0; i<_pastePos.count(); i++) {
                _pastePos[i] = unitedRect.topLeft() + offset[i];
            }
            _clickedPos = QPointF(0,0);
        }
        itemRects.clear();
        offset.clear();

        int count = 0;
        QList<BoxItem*> *copyList = new QList<BoxItem *>();
        foreach (QGraphicsItem* item, data->items()) {
            if (item->type() == QGraphicsItem::UserType + 1) {
                BoxItem *b = qgraphicsitem_cast<BoxItem*>(item);
                if (_clickedPos.isNull())
                    _pastePos[count] += QPointF(10, 10);
                QRectF rect(_pastePos[count].x(), _pastePos[count].y(), b->rect().width(), b->rect().height());
                BoxItem *copy = b->copy();
                copy->setRect(rect);
                copy->setSelected(true);
                this->registerItem(copy);
                copyList->append(copy);

                count++;
            }
        }
        if (copyList->count() > 0) {
            _undoStack->push(new AddBoxCommand(this, copyList));
        }
//        if (copyList)
//            delete copyList;
    }
}

void CustomScene::cut()
{
    QList<QGraphicsItem*> copyList;
    foreach (QGraphicsItem *item , selectedItems()) {
        if (item->type() == QGraphicsItem::UserType + 1) {
            BoxItem *b = qgraphicsitem_cast<BoxItem*>(item);
            BoxItem *copy = b->copy();
            copyList.append(copy);
        }
    }
    if (copyList.count() > 0){
        deleteBoxItems();
        if (_boxItemMimeData) {
            delete _boxItemMimeData;
        }
        _boxItemMimeData = new BoxItemMimeData(copyList);
        QApplication::clipboard()->setMimeData(_boxItemMimeData);

        _pastePos.clear();
        for (int i=0; i<copyList.count(); i++)
            _pastePos.append(QPointF(0,0));
        _clickedPos = QPointF(0,0);
    }
    copyList.clear();
}

void CustomScene::clipboardDataChanged()
{
//    QObject::sender()
//   pasteAction->setEnabled(true);
}
