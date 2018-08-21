#ifndef STATEBOX_H
#define STATEBOX_H

#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QColor>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QCursor>
#include <QStyle>
#include <QStyleOptionGraphicsItem>
#include "cornergrabber.h"

enum GrabberID{
    TopLeft = 0,
    TopCenter,
    TopRight,
    RightCenter,
    BottomRight,
    BottomCenter,
    BottomLeft,
    LeftCenter,
    BoxRegion
};

enum TaskStatus {
    Moving = 0,
    Stretching,
    Waiting
};

class BoxItem : public QGraphicsItem
{
public:
    BoxItem(QRectF fatherRect);
    QGraphicsTextItem _text;

    void setScale(QRectF fatherRect);
    void setRect(const QRectF &rect);
    void setRect(const qreal x, qreal y, qreal w, qreal h);
    void setLabelClass(int cls);
    int labelClass() const
    {
        return _labelClass;
    }
    void setLabelRect(QRectF rect);
    void labelRect(qreal *info) const
    {
        qreal ws = 1.0 / _fatherRect.width();
        qreal hs = 1.0 / _fatherRect.height();

        info[0] = _box.center().x()*ws;
        info[1] = _box.center().y()*hs;
        info[2] = _box.width()*ws;
        info[3] = _box.width()*hs;
    }
    void setLabelRect(qreal x, qreal y, qreal w, qreal h);

    enum { Type = UserType + 1 };
    int type() const
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return Type;
    }

private:

    virtual QRectF boundingRect() const;
    void BoxItem::paint (QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    virtual void hoverEnterEvent ( QGraphicsSceneHoverEvent *event );
    virtual void hoverLeaveEvent ( QGraphicsSceneHoverEvent *event );
    virtual void hoverMoveEvent (QGraphicsSceneHoverEvent * event);

    virtual void mouseMoveEvent ( QGraphicsSceneMouseEvent * event );
    virtual void mousePressEvent (QGraphicsSceneMouseEvent * event );
    virtual void mouseReleaseEvent (QGraphicsSceneMouseEvent * event );

    virtual void mouseMoveEvent(QGraphicsSceneDragDropEvent *event);
    virtual void mousePressEvent(QGraphicsSceneDragDropEvent *event);

    void setGrabbers();
    void moveBox(QPointF pos);
    void stretchBox(QPointF pos);

    GrabberID getSelectedGrabber(QPointF point);
    void setGrabberCursor(GrabberID stretchRectState);
    GrabberID _selectedGrabber;
    TaskStatus _taskStatus = Waiting;

    QRectF _fatherRect;
    QRectF _box;
    QRectF _drawingRegion;
    QRectF _boundingRect;
    int _labelClass = 0;
    QRectF _labelRect;

    QColor _color;
    QPen _pen;
    QRectF _oldBox;
    QPointF _dragStart;

    int _grabberWidth;
    int _grabberHeight;

    QRectF _grabbers[8];

};

#endif // STATEBOX_H
