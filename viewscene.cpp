#include "viewscene.h"
#include <QtDebug>
#include <QScrollBar>

ViewScene::ViewScene(QObject* parent):
    QGraphicsScene(parent),
    image(nullptr),
    pixmapItem(nullptr),
    boxItem(nullptr),
    isDrawing(false),
    _isLabelFileLoaded(false)
{
}

void ViewScene::clear()
{
    saveBoxItemsToFile();
    _isLabelFileLoaded = false;

    if (image != nullptr) {
        delete image;
        image = nullptr;
    }

    foreach (QGraphicsItem *item, this->items()) {
        if ((item->type() == QGraphicsItem::UserType+1) ||
            (item->type() == QGraphicsPixmapItem::Type) ) {
            this->removeItem(item);
            delete item;
        }
    }
}
void ViewScene::loadImage(QString path)
{
    image = new QImage(path);
    pixmapItem = new QGraphicsPixmapItem(QPixmap::fromImage(*image));
    pixmapItem->setTransformationMode(Qt::SmoothTransformation);
    this->addItem(pixmapItem);

    QFileInfo info(path);
    _labelFilePath = info.path() + "/" + info.completeBaseName() + ".txt";
}

void ViewScene::loadBoxItemsFromFile()
{
    QFile file(_labelFilePath);
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    QPoint zero(0, 0);
    QRect fatherRect(zero, image->size());
    qreal x,y,w,h;
    int cls;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QStringList info = in.readLine().split(" ");
        if(info.size() >= 5) {
            cls = info.at(0).toInt();
            w = info.at(3).toFloat() * image->width();
            h = info.at(4).toFloat() * image->height();
            x = info.at(1).toFloat() * image->width() - w/2;
            y = info.at(2).toFloat() * image->height() - h/2;

            BoxItem *b = new BoxItem(fatherRect, image->size(), classNameList, classNameList->at(cls));
            b->setLabelRect(x,y,w,h);
            b->setRect(x,y,w,h);

            this->addItem(b);
        }
    }
    file.close();
}

void ViewScene::saveBoxItemsToFile()
{
    QFile file(_labelFilePath);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    qreal label[4];
    QTextStream out(&file);

    foreach (QGraphicsItem *item, this->items()) {
        if (item->type() == QGraphicsItem::UserType+1) {
            BoxItem *b = qgraphicsitem_cast<BoxItem *>(item);
            b->labelRect(label);
            QString s;
            s.sprintf("%d %f %f %f %f\n",
                      classNameList->indexOf(b->labelClassName()),
                      label[0],label[1],label[2],label[3]);
            out << s;
        }
    }
    file.close();
}

void ViewScene::drawView()
{
    QPoint zero(0, 0);
    QRect rect(zero, image->size() * zoomFactor);
    setSceneRect(rect);

    if (!_isLabelFileLoaded) {
        loadBoxItemsFromFile();
        _isLabelFileLoaded = true;
    }

    foreach (QGraphicsItem *item, this->items()) {
        if (item->type() == QGraphicsItem::UserType+1) {
            BoxItem *b = qgraphicsitem_cast<BoxItem *>(item);
            b->setScale(this->sceneRect());
        } else if (item->type() == QGraphicsPixmapItem::Type) {
            item->setScale(zoomFactor);
        }
    }
}

void ViewScene::deleteBoxItems()
{
    foreach (QGraphicsItem *item, this->selectedItems()) {
        if (item->type() == QGraphicsItem::UserType+1) {
            this->removeItem(item);
            delete item;
        }
    }
}

void ViewScene::selectAllBoxItems(bool op)
{
    // selecting box items
    foreach (QGraphicsItem *item, this->items()) {
        if (item->type() == QGraphicsItem::UserType+1) {
            item->setSelected(op);
        }
    }
}

void ViewScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    switch (event->buttons()) {
    case Qt::LeftButton:
        if (event->modifiers() == Qt::ShiftModifier) { // drawing box item
            selectAllBoxItems(false); // deselect all the other box items
            leftTopPoint = event->scenePos();
            isDrawing = true;
        } else if (event->modifiers() == Qt::ControlModifier) { // selecting multiple box items
            QGraphicsScene::mousePressEvent(event);
        } else { // selecting single box item
            selectAllBoxItems(false);
            QGraphicsScene::mousePressEvent(event);
        }
        break;
    case Qt::RightButton:
        selectAllBoxItems(false);
        QGraphicsScene::mousePressEvent(event);
    break;
    default:
        break;
    }
}

void ViewScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        if(isDrawing) {
            this->views().at(0)->viewport()->setCursor(Qt::CrossCursor);
            if(!boxItem) {
                boxItem = new BoxItem(this->sceneRect(), image->size(), classNameList, classNameList->at(0));
                this->addItem(boxItem);
                boxItem->setSelected(true);
            }
            rightBottomPoint = event->scenePos();
            QRect roi(qMin(rightBottomPoint.x(), leftTopPoint.x()),
                      qMin(rightBottomPoint.y(), leftTopPoint.y()),
                      qAbs(rightBottomPoint.x() - leftTopPoint.x()),
                      qAbs(rightBottomPoint.y() - leftTopPoint.y()));
            int l = sceneRect().left(), r = sceneRect().right(),
                    t = sceneRect().top(), b = sceneRect().bottom();
            if (roi.right() < l || roi.left() > r ||
                    roi.top() > b || roi.bottom() < t )
                return;

            roi.setTop(qMax(t, roi.top()));
            roi.setLeft(qMax(l, roi.left()));
            roi.setBottom(qMin(b, roi.bottom()));
            roi.setRight(qMin(r, roi.right()));

            boxItem->setRect(roi);
        }

        if (_isPanning)
        {
            this->views().at(0)->viewport()->setCursor(Qt::ClosedHandCursor);
            QScrollBar *h = this->views().at(0)->horizontalScrollBar();
            h->setValue(h->value() - (int)(event->scenePos().x() - _dragStart.x()));
            QScrollBar *v = this->views().at(0)->verticalScrollBar();
            v->setValue(v->value() - (int)(event->scenePos().y() - _dragStart.y()));
            _dragEnd = event->scenePos();
        }
    }

    QGraphicsScene::mouseMoveEvent(event);
}

void ViewScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (isDrawing) {
        isDrawing = false;
        boxItem = 0;
    }
    if (_isPanning) {
        _isPanning = false;
        this->views().at(0)->viewport()->setCursor(Qt::ArrowCursor);
    }
    QGraphicsScene::mouseReleaseEvent(event);
}

void ViewScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if (mouseEvent->buttons() == Qt::LeftButton)
    {
        _isPanning = true;
        _dragStart = mouseEvent->scenePos();
        this->views().at(0)->viewport()->setCursor(Qt::ClosedHandCursor);
    }
    QGraphicsScene::mouseDoubleClickEvent(mouseEvent);
}

void ViewScene::keyPressEvent(QKeyEvent *keyEvent)
{
    if(keyEvent->key() == Qt::Key_Delete) {
        deleteBoxItems();
    } else if(keyEvent->key() == Qt::Key_A && keyEvent->modifiers() == Qt::ControlModifier) {
        selectAllBoxItems(true);
    } else if(keyEvent->key() == Qt::Key_Shift) {
        this->views().at(0)->viewport()->setCursor(Qt::CrossCursor);
    } else {
        QGraphicsScene::keyPressEvent(keyEvent);
    }
}

void ViewScene::keyReleaseEvent(QKeyEvent *keyEvent)
{
    if(keyEvent->key() == Qt::Key_Shift) {
            this->views().at(0)->viewport()->setCursor(Qt::ArrowCursor);
    }
    QGraphicsScene::keyReleaseEvent(keyEvent);
}

/**
 * @brief Returns current preview zoom of image.
 */
double ViewScene::viewZoom() const
{
    return zoomFactor;
}

void ViewScene::setViewZoom(double zoom)
{
    zoomFactor = zoom;
    drawView();
}

void ViewScene::setViewZoom(int w, int h)
{
    QSize size = image->size();
    zoomFactor = std::min(1.0 * w / size.width(), 1.0 * h / size.height());
    drawView();
}
