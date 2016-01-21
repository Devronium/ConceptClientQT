#ifndef TREE_DELEGATE_H
#define TREE_DELEGATE_H

#include <QWidget>
#include <QStyledItemDelegate>
#include "ConceptClient.h"
#include "GTKControl.h"

class TreeDelegate : public QStyledItemDelegate {
private:
    int            type;
    CConceptClient *CC;
public:
    TreeDelegate(int type, CConceptClient *CC) : QStyledItemDelegate() {
        this->type = type;
        this->CC   = CC;
    }

protected:

    void paint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
        QStyledItemDelegate::paint(painter, option, index);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        QTreeWidget *parent = (QTreeWidget *)index.model()->parent();
        int         column  = index.column();

        QStyleOptionViewItemV4 opt = option;
        initStyleOption(&opt, index);

        QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();

        switch (type) {
            case 0x83:
            case 0x03:
                {
                    QStyleOptionButton opts;
                    opts.rect = option.rect;
                    int x = (opts.rect.width() - opts.iconSize.width()) / 2 - 6;
                    if (x < 0)
                        x = 0;
                    opts.rect.setX(opts.rect.x() + x);
                    opts.rect.setWidth(opts.iconSize.width());
                    //iconSize
                    int val = index.model()->data(index, Qt::UserRole).toInt();
                    if (val)
                        opts.state |= QStyle::State_On | QStyle::State_Enabled;
                    style->drawControl(QStyle::CE_CheckBox, &opts, painter, opt.widget);
                }
                break;

            case 0x84:
            case 0x04:
                {
                    QStyleOptionButton opts;
                    opts.rect = option.rect;
#ifdef __APPLE__
                    int x = (opts.rect.width() - opts.iconSize.width()) / 2 - 2;
#else
                    int x = (opts.rect.width() - opts.iconSize.width()) / 2 - 6;
#endif
                    //if (x<0)
                    //    x=0;
                    opts.rect.setX(opts.rect.x() + x);
                    opts.rect.setWidth(opts.iconSize.width());
                    //iconSize
                    int val = index.model()->data(index, Qt::UserRole).toInt();
                    if (val)
                        opts.state |= QStyle::State_On | QStyle::State_Enabled;
                    style->drawControl(QStyle::CE_RadioButton, &opts, painter, opt.widget);
                }
                break;

            case 0x85:
            case 0x05:
                {
                    int        val    = index.model()->data(index, Qt::DisplayRole).toInt();
                    GTKControl *item2 = CC->Controls[val];
                    if ((item2) && (item2->type == CLASS_IMAGE)) {
                        const QPixmap *pmap = ((QLabel *)item2->ptr)->pixmap();
                        if (pmap) {
                            painter->drawPixmap(QRect(0, 0, 13, 15), *pmap, option.rect);
                            //style->drawControl(QStyle::CE_RadioButton, &opts, painter, opt.widget);
                        }
                    }
                }
                break;

            case 0x87:
            case 0x07:
                {
                    QStyleOptionViewItemV4 opts(option);

                    QString str = index.model()->data(index, Qt::UserRole).toString().replace(QString("\n"), QString("<br/>"));
                    initStyleOption(&opts, index);
                    opts.rect = option.rect;

                    QTextDocument doc;
                    doc.setHtml(str);
                    opts.text   = "";
                    opts.state &= ~QStyle::State_Selected;
                    opts.state &= ~QStyle::State_HasFocus;
                    opts.state &= ~QStyle::State_MouseOver;

                    painter->eraseRect(opts.rect);

                    //style->drawControl(QStyle::CE_ItemViewItem, &opts, painter);
                    painter->translate(opts.rect.left(), opts.rect.top());

                    QRect clip(0, 0, opts.rect.width(), opts.rect.height());
                    doc.drawContents(painter, clip);
                }
                break;

            case 0x82:
            case 0x02:
                {
                    QStyleOptionProgressBarV2 opts;
                    opts.rect    = option.rect;
                    opts.minimum = 0;
                    opts.maximum = 1000000;
                    int val = index.model()->data(index, Qt::UserRole).toFloat() * opts.maximum / 100;
                    opts.progress = val;
                    style->drawControl(QStyle::CE_ProgressBar, &opts, painter, opt.widget);
                }
                break;
        }
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
        if ((type == 0x07) || (type == 0x87)) {
            QStyleOptionViewItemV4 options = option;
            initStyleOption(&options, index);

            QString str = index.model()->data(index, Qt::UserRole).toString().replace(QString("\n"), QString("<br/>"));

            QTextDocument doc;
            doc.setHtml(str);
            doc.setTextWidth(options.rect.width());
            return QSize(doc.idealWidth(), doc.size().height());
        } else
        if ((type == 0x03) || (type == 0x83) || (type == 0x04) || (type == 0x84))
#ifdef __APPLE__
            return QSize(22, 16);
#else
            return QSize(16, 16);
#endif
        return QStyledItemDelegate::sizeHint(option, index);
    }

    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem& option, const QModelIndex& index) const {
        if ((type == 0x81) || (type == 0x87)) {
            QLineEdit *edit = new QLineEdit(parent);
            edit->setMinimumSize(option.rect.size());
            return edit;
        }
        return 0;
    }

    virtual bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem& option, const QModelIndex& index) {
        // to do
        if ((type == 0x83) || (type == 0x84)) {
            if (event->type() == QEvent::MouseButtonRelease) {
                Qt::ItemFlags flags = model->flags(index);
            } else if (event->type() == QEvent::KeyPress) {
                if ((static_cast<QKeyEvent *>(event)->key() != Qt::Key_Space) && (static_cast<QKeyEvent *>(event)->key() != Qt::Key_Select))
                    return false;
            } else
                return false;

            int val = index.model()->data(index, Qt::UserRole).toInt();
            if (val)
                model->setData(index, QString("0"), Qt::UserRole);
            else
                model->setData(index, QString("1"), Qt::UserRole);

            return true;
        }
        return false;
    }
};
#endif
