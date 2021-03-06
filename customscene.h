#ifndef CUSTOMSCENE_H
#define CUSTOMSCENE_H

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsLineItem>
#include <QAction>
#include <QGraphicsView>
#include <QKeyEvent>
#include "boxitem.h"
#include <QFileInfo>
#include <QFile>
#include <QImageReader>
#include <QUndoStack>
#include "commands.h"
#include "FreeImage.h"
#include "boxitemmimedata.h"
#include <QClipboard>

class CustomScene : public QGraphicsScene
{
    Q_OBJECT
public:
    CustomScene(QObject* parent = 0);
    ~CustomScene()
    {
        clearAll();
    }

    void loadImage(QString path);
    void saveToFile(const QString& path);
    void clearAll();

    void setTypeNameList (const QStringList &list)
    {
        _typeNameList = list;
    }
    void setTypeName (const QString &name)
    {
        _typeName = name;
    }
    QUndoStack *undoStack() const
    {
        return _undoStack;
    }
    void selectBoxItems(QList<BoxItem *> *boxList, bool op);
    void selectBoxItems(BoxItem *box, bool op);
    void selectBoxItems(bool op);
    void registerItem(BoxItem *b);
    void drawBoxItem(bool op);
    void panImage(bool op);

public slots:
    void changeBoxTypeName(QString name);
    void selectedBoxItemInfo(QRect rect, QString typeName)
    {
        _typeName = typeName;
    }
    void copy();
    void cut();
    void paste();
    void clipboardDataChanged();

private slots:
    void moveBox(QRectF newRect, QRectF oldRect);

signals:
    void imageLoaded(QSize imageSize);
    void cursorMoved(QPointF cursorPos);
    void boxSelected(QRect boxRect, QString typeName);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void keyPressEvent(QKeyEvent *keyEvent);
    void keyReleaseEvent(QKeyEvent *keyEvent);
    void deleteBoxItems();
private:
    QImage *_image;
    QGraphicsPixmapItem *_pixmapItem = nullptr;
    BoxItem* _boxItem = nullptr;//, *_selectedBoxItem;
    QString _typeName;
    QStringList _typeNameList;
    double _zoomFactor = 1;
    QPointF _dragStart, _dragEnd;
    bool _isPanning = false;
    bool _isDrawing = false;
    bool _isMoving = false;
    bool _isMouseMoved = false;
    QPointF _leftTopPoint;
    QPointF _rightBottomPoint;
    QString _boxItemFileName;
    QUndoStack *_undoStack;
    BoxItemMimeData *_boxItemMimeData = nullptr;
    QList<QPointF> _pastePos;
    QPointF _clickedPos;
    void loadBoxItemsFromFile();
    void saveBoxItemsToFile();
};
#endif // CUSTOMSCENE_H
