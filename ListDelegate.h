#ifndef LIST_DELEGATE_H
#define LIST_DELEGATE_H

#include <QWidget>
#include <QStyledItemDelegate>
#include "ConceptClient.h"
#include "GTKControl.h"

class ListDelegate : public QStyledItemDelegate {
private:
    GTKControl *control;
public:
    ListDelegate(GTKControl *control) : QStyledItemDelegate() {
        this->control = control;
    }

protected:

    void paint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
        QStyleOptionViewItem opt(option);

        if (!((TreeData *)control->ptr3)->orientation)
            opt.decorationPosition = QStyleOptionViewItem::Left;
        else
            opt.decorationPosition = QStyleOptionViewItem::Top;
        if (((TreeData *)control->ptr3)->markup < 0) {
            QStyledItemDelegate::paint(painter, opt, index);
            return;
        }

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());
        else
        if (option.state & QStyle::State_MouseOver)
            painter->fillRect(option.rect, option.palette.midlight());

        if (((TreeData *)control->ptr3)->orientation) {
            // vertical
            QIcon icon = index.model()->data(index, Qt::DecorationRole).value<QIcon>();
            QSize size = icon.actualSize(QSize(opt.rect.width() - 2, opt.rect.height() - 18));
            int   x    = (opt.rect.width() - size.width()) / 2 - 1;
            icon.paint(painter, QRect(opt.rect.x() + x + 1, opt.rect.y() + 1, size.width(), size.height()), Qt::AlignTop | Qt::AlignHCenter);

            QTextDocument doc;
            doc.setHtml(index.model()->data(index).toString().replace("\n", "<br/>"));

            QSize dsize = doc.size().toSize();

            x = (opt.rect.width() - dsize.width()) / 2;
            painter->translate(opt.rect.left() + x, opt.rect.top() + size.height() + 1);
            QRect clip(0, 0, dsize.width(), dsize.height());

            doc.drawContents(painter, clip);
        } else {
            // horizontal
            QIcon icon = index.model()->data(index, Qt::DecorationRole).value<QIcon>();
            QSize size = icon.actualSize(QSize(opt.rect.width() - 2, opt.rect.height() - 2));
            icon.paint(painter, QRect(opt.rect.x() + 1, opt.rect.y() + 1, opt.rect.width() - 2, opt.rect.height() - 2), Qt::AlignLeft | Qt::AlignVCenter);

            QTextDocument doc;
            doc.setHtml(index.model()->data(index).toString().replace("\n", "<br/>"));

            painter->translate(opt.rect.left() + size.width(), opt.rect.top());
            QRect clip(0, 0, opt.rect.width(), opt.rect.height());
            doc.drawContents(painter, clip);
        }
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
        int   width = ((TreeData *)control->ptr3)->extra;
        QSize result;

        if (((TreeData *)control->ptr3)->markup < 0)
            result = QStyledItemDelegate::sizeHint(option, index);
        else {
            QTextDocument doc;
            doc.setHtml(index.model()->data(index).toString().replace("\n", "<br/>"));
            result = doc.size().toSize();

            QIcon icon = index.model()->data(index, Qt::DecorationRole).value<QIcon>();

            QList<QSize> sizes = icon.availableSizes();
            if (sizes.size()) {
                QSize size = sizes[0];
                if (((TreeData *)control->ptr3)->orientation)
                    result.setHeight(result.height() + size.height());
                else
                    result.setWidth(result.width() + size.width());
            }
        }

        if (width > 0)
            result.setWidth(width);

        return result;
        //return QStyledItemDelegate::sizeHint(option,index);
    }
};
#endif
