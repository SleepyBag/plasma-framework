/*
 * Copyright 2008 by Montel Laurent <montel@kde.org>
 * Copyright 2008 by Marco Martin <notmart@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "popupapplet.h"
#include "private/popupapplet_p.h"

#include <QApplication>
#include <QGraphicsProxyWidget>
#include <QGraphicsLinearLayout>
#include <QTimer>
#include <QVBoxLayout>

#ifdef Q_WS_X11
#include <QX11Info>
#endif

#include <kicon.h>
#include <kiconloader.h>
#include <kwindowsystem.h>
#include <kglobalsettings.h>
#include <netwm.h>

#include "plasma/private/applet_p.h"
#include "plasma/private/extenderitemmimedata_p.h"
#include "plasma/corona.h"
#include "plasma/containment.h"
#include "plasma/private/containment_p.h"
#include "plasma/dialog.h"
#include "plasma/extenders/extender.h"
#include "plasma/extenders/extenderitem.h"
#include "plasma/tooltipmanager.h"
#include "plasma/widgets/iconwidget.h"

namespace Plasma
{

PopupApplet::PopupApplet(QObject *parent, const QVariantList &args)
    : Plasma::Applet(parent, args),
      d(new PopupAppletPrivate(this))
{
    int iconSize = IconSize(KIconLoader::Desktop);
    resize(iconSize, iconSize);
    disconnect(this, SIGNAL(activate()), (Applet*)this, SLOT(setFocus()));
    connect(this, SIGNAL(activate()), this, SLOT(internalTogglePopup()));
    setAcceptDrops(true);
}

PopupApplet::~PopupApplet()
{
    delete widget();
    delete d;
}

void PopupApplet::setPopupIcon(const QIcon &icon)
{
    if (icon.isNull()) {
        if (d->icon) {
            delete d->icon;
            d->icon = 0;
            setLayout(0);
            setAspectRatioMode(d->savedAspectRatio);
        }

        return;
    }

    if (!d->icon) {
        d->icon = new Plasma::IconWidget(icon, QString(), this);
        connect(d->icon, SIGNAL(clicked()), this, SLOT(internalTogglePopup()));

        QGraphicsLinearLayout *layout = new QGraphicsLinearLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->setOrientation(Qt::Horizontal);

        if (formFactor() == Plasma::Vertical || formFactor() == Plasma::Horizontal ) {
            d->savedAspectRatio = aspectRatioMode();
            setAspectRatioMode(Plasma::ConstrainedSquare);
        }

        setLayout(layout);
    } else {
        d->icon->setIcon(icon);
    }
}

void PopupApplet::setPopupIcon(const QString &iconName)
{
    setPopupIcon(KIcon(iconName));
}

QIcon PopupApplet::popupIcon() const
{
    return d->icon ? d->icon->icon() : QIcon();
}

QWidget *PopupApplet::widget()
{
    return d->widget;
}

void PopupApplet::setWidget(QWidget *widget)
{
    if (d->widget) {
        if (d->dialog) {
            d->dialog.data()->setGraphicsWidget(0);
            QVBoxLayout *lay = 0;

            QLayout *existingLayout = d->dialog.data()->layout();
            if (existingLayout) {
                lay = dynamic_cast<QVBoxLayout *>(existingLayout);
                if (!lay) {
                    delete existingLayout;
                }
            }

            if (!lay) {
                lay = new QVBoxLayout;
                d->dialog.data()->setLayout(lay);
            }

            lay->removeWidget(d->widget);
            lay->addWidget(widget);
        } else if (d->proxy) {
            d->proxy.data()->setWidget(widget);
        }
    }

    d->widget = widget;
}

QGraphicsWidget *PopupApplet::graphicsWidget()
{
    if (d->graphicsWidget != 0) {
        return d->graphicsWidget;
    } else {
        return static_cast<Applet*>(this)->d->extender;
    }
}

void PopupApplet::setGraphicsWidget(QGraphicsWidget *graphicsWidget)
{
    if (d->graphicsWidget) {
        if (d->dialog) {
            d->dialog.data()->setGraphicsWidget(graphicsWidget);
        } else {
            QGraphicsLinearLayout *lay = static_cast<QGraphicsLinearLayout *>(layout());
            lay->removeAt(0);
            lay->addItem(graphicsWidget);
        }
    }

    d->graphicsWidget = graphicsWidget;
}

void PopupAppletPrivate::checkExtenderAppearance(Plasma::FormFactor f)
{
    Extender *extender = qobject_cast<Extender*>(q->graphicsWidget());
    if (extender) {
        if (f != Plasma::Horizontal && f != Plasma::Vertical) {
            extender->setAppearance(Extender::NoBorders);
        } else if (q->location() == TopEdge) {
            extender->setAppearance(Extender::TopDownStacked);
        } else {
            extender->setAppearance(Extender::BottomUpStacked);
        }

        if (dialog) {
            dialog.data()->setGraphicsWidget(extender);
        }
    }
}

void PopupAppletPrivate::popupConstraintsEvent(Plasma::Constraints constraints)
{
    Plasma::FormFactor f = q->formFactor();

    if (constraints & Plasma::LocationConstraint) {
        checkExtenderAppearance(f);
    }

    if (constraints & Plasma::FormFactorConstraint ||
        constraints & Plasma::StartupCompletedConstraint ||
        (constraints & Plasma::SizeConstraint &&
         (f == Plasma::Vertical || f == Plasma::Horizontal))) {
        QGraphicsLinearLayout *lay = dynamic_cast<QGraphicsLinearLayout *>(q->layout());

        if (icon && !icon->icon().isNull() && lay) {
            lay->removeAt(0);
        }

        QSizeF minimum;
        QSizeF parentSize;

        QGraphicsWidget *gWidget = q->graphicsWidget();
        //kDebug() << "graphics widget is" << (QObject*)gWidget;
        QWidget *qWidget = q->widget();

        if (gWidget) {
            minimum = gWidget->minimumSize();
            // our layout may have been replaced on us in the call to graphicsWidget!
            lay = dynamic_cast<QGraphicsLinearLayout *>(q->layout());

            if (!(constraints & LocationConstraint)) {
                checkExtenderAppearance(f);
            }
        } else if (qWidget) {
            minimum = qWidget->minimumSizeHint();
        }

        //99% of the times q->parentWidget() is the containment, but using it  we can also manage the applet-in-applet case (i.e. systray)
        if (q->parentLayoutItem()) {
            parentSize = q->parentLayoutItem()->geometry().size();
        } else if (q->parentWidget()) {
            parentSize = q->parentWidget()->size();
        }

        //Applet on desktop
        if (icon && !icon->icon().isNull() && ((f != Plasma::Vertical && f != Plasma::Horizontal) ||
            ((f == Plasma::Vertical && parentSize.width() >= minimum.width()) ||
             (f == Plasma::Horizontal && parentSize.height() >= minimum.height())))) {
            //kDebug() << "we are expanding the popupapplet";

            // we only switch to expanded if we aren't horiz/vert constrained and
            // this applet has an icon.
            // otherwise, we leave it up to the applet itself to figure it out
            if (icon) {
                icon->hide();
            }

            if (savedAspectRatio != Plasma::InvalidAspectRatioMode) {
                q->setAspectRatioMode(savedAspectRatio);
            }

            if (dialog) {
                if (dialog.data()->layout() && qWidget) {
                    //we don't want to delete Widget inside the dialog layout
                    dialog.data()->layout()->removeWidget(qWidget);
                }

                if (qWidget) {
                    qWidget->setParent(0);
                }

                delete dialog.data();
            }

            if (!lay) {
                lay = new QGraphicsLinearLayout();
                lay->setContentsMargins(0, 0, 0, 0);
                lay->setSpacing(0);
                lay->setOrientation(Qt::Horizontal);
                q->setLayout(lay);
            }

            QSize prefSize;

            if (gWidget) {
                if (proxy) {
                    proxy.data()->setWidget(0);
                    delete proxy.data();
                }

                Corona *corona = qobject_cast<Corona *>(gWidget->scene());

                if (corona) {
                    corona->removeOffscreenWidget(gWidget);
                }

                lay->addItem(gWidget);
                prefSize = gWidget->preferredSize().toSize();
            } else if (qWidget) {
                if (!proxy) {
                    proxy = new QGraphicsProxyWidget(q);
                    proxy.data()->setWidget(qWidget);
                    proxy.data()->show();
                }

                lay->addItem(proxy.data());
                prefSize = qWidget->sizeHint();
            }

            //we could be on a big panel, but in that case we will be able to resize
            //more than the natural minimum size, because we'll transform into an icon
            if (f == Plasma::Horizontal) {
                minimum.setHeight(0);
            } else if (f == Plasma::Vertical) {
                minimum.setWidth(0);
            }

            qreal left, top, right, bottom;
            q->getContentsMargins(&left, &top, &right, &bottom);
            QSizeF oldSize(q->size());
            q->setMinimumSize(minimum + QSizeF(left+right, top+bottom));
            //size not saved/invalid size saved
            if (oldSize.width() < q->minimumSize().width() || oldSize.height() < q->minimumSize().height()) {
                q->resize(prefSize);
                emit q->appletTransformedItself();
            }
        //Applet on popup
        } else {
            //kDebug() << "about to switch to a popup";
            //save the aspect ratio mode in case we drag'n drop in the Desktop later
            savedAspectRatio = q->aspectRatioMode();

            if (icon) {
                icon->show();
                q->setAspectRatioMode(Plasma::ConstrainedSquare);
            }

            if (proxy) {
                proxy.data()->setWidget(0); // prevent it from deleting our widget!
                delete proxy.data();
            }

            if (!dialog) {
                dialog = new Plasma::Dialog();

                //no longer use Qt::Popup since that seems to cause a lot of problem when you drag
                //stuff out of your Dialog (extenders). Monitor WindowDeactivate events so we can
                //emulate the same kind of behavior as Qt::Popup (close when you click somewhere
                //else.

                q->setMinimumSize(QSize(0, 0));
                if (gWidget) {
                    Corona *corona = qobject_cast<Corona *>(gWidget->scene());

                    //could that cast ever fail??
                    if (corona) {
                        corona->addOffscreenWidget(gWidget);
                        dialog.data()->setGraphicsWidget(gWidget);
                        gWidget->resize(gWidget->preferredSize());
                    }

                    dialog.data()->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | (gWidget->windowFlags() & Qt::X11BypassWindowManagerHint));
                } else if (qWidget) {
                    QVBoxLayout *l_layout = new QVBoxLayout(dialog.data());
                    l_layout->setSpacing(0);
                    l_layout->setMargin(0);
                    l_layout->addWidget(qWidget);
                    dialog.data()->adjustSize();
                    dialog.data()->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | (qWidget->windowFlags() & Qt::X11BypassWindowManagerHint));
                } else {
                    dialog.data()->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
                }

                KWindowSystem::setState(dialog.data()->winId(), NET::SkipTaskbar | NET::SkipPager);
                dialog.data()->installEventFilter(q);

                QObject::connect(dialog.data(), SIGNAL(dialogResized()), q, SLOT(dialogSizeChanged()));
                QObject::connect(dialog.data(), SIGNAL(dialogVisible(bool)), q, SLOT(dialogStatusChanged(bool)));
            }

            if (icon && lay) {
                lay->addItem(icon);
            }

            q->setMinimumSize(0,0);
        }
    }

    if (dialog && constraints & Plasma::PopupConstraint) {
        updateDialogPosition();
    }

    emit q->sizeHintChanged(Qt::PreferredSize);
}

void PopupApplet::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (!d->icon && !d->popupLostFocus && event->buttons() == Qt::LeftButton) {
        d->clicked = scenePos().toPoint();
        event->setAccepted(true);
        return;
    } else {
        d->popupLostFocus = false;
        Applet::mousePressEvent(event);
    }
}

void PopupApplet::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (!d->icon &&
        (d->clicked - scenePos().toPoint()).manhattanLength() < KGlobalSettings::dndEventDelay()) {
        d->internalTogglePopup();
    } else {
        Applet::mouseReleaseEvent(event);
    }
}

bool PopupApplet::eventFilter(QObject *watched, QEvent *event)
{
    if (!d->passive && watched == d->dialog.data() && (event->type() == QEvent::WindowDeactivate)) {
        d->popupLostFocus = true;
        hidePopup();
        QTimer::singleShot(100, this, SLOT(clearPopupLostFocus()));
    }

    /**
    if (layout() && watched == graphicsWidget() && (event->type() == QEvent::GraphicsSceneResize)) {
        //sizes are recalculated in the constraintsevent so let's just call that.
        d->popupConstraintsEvent(Plasma::FormFactorConstraint);

        //resize vertically if necesarry.
        if (formFactor() == Plasma::MediaCenter || formFactor() == Plasma::Planar) {
            resize(QSizeF(size().width(), minimumHeight()));
        }
    }
    */

    if (watched == d->dialog.data() && event->type() == QEvent::ContextMenu) {
        //pass it up to the applet
        //well, actually we have to pass it to the *containment*
        //because all the code for showing an applet's contextmenu is actually in Containment.
        Containment *c = containment();
        if (c) {
            return c->d->showAppletContextMenu(this, static_cast<QContextMenuEvent*>(event)->globalPos());
        }
    }

    return Applet::eventFilter(watched, event);
}

//FIXME: some duplication between the drag events... maybe add some simple helper function?
void PopupApplet::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasFormat(ExtenderItemMimeData::mimeType())) {
        const ExtenderItemMimeData *mimeData =
            qobject_cast<const ExtenderItemMimeData*>(event->mimeData());
        if (mimeData && qobject_cast<Extender*>(graphicsWidget())) {
            event->accept();
            showPopup();
        }
    }
}

void PopupApplet::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasFormat(ExtenderItemMimeData::mimeType())) {
        const ExtenderItemMimeData *mimeData =
            qobject_cast<const ExtenderItemMimeData*>(event->mimeData());
        if (mimeData && qobject_cast<Extender*>(graphicsWidget())) {
            //We want to hide the popup if we're not moving onto the popup AND it is not the popup
            //we started.
            if (d->dialog && !d->dialog.data()->geometry().contains(event->screenPos()) &&
                mimeData->extenderItem()->extender() != qobject_cast<Extender*>(graphicsWidget())) {
                //We actually try to hide the popup, with a call to showPopup, with a smal timeout,
                //so if the user moves into the popup fast enough, it remains open (the extender
                //will call showPopup which will cancel the timeout.
                showPopup(250);
            }
        }
    }
}

void PopupApplet::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasFormat(ExtenderItemMimeData::mimeType())) {
        const ExtenderItemMimeData *mimeData =
            qobject_cast<const ExtenderItemMimeData*>(event->mimeData());
        if (mimeData && qobject_cast<Extender*>(graphicsWidget())) {
            mimeData->extenderItem()->setExtender(extender());
            QApplication::restoreOverrideCursor();
        }
    }
}

void PopupApplet::showPopup(uint popupDuration)
{
    if (d->dialog) {
        // move the popup before its fist show, even if the show isn't triggered by
        // a click, this should fix the first random position seen in some widgets
        if (!d->dialog.data()->isVisible()) {
            d->internalTogglePopup();
        }

        if (d->timer) {
            d->timer->stop();
        }

        if (popupDuration > 0) {
            if (!d->timer) {
                d->timer = new QTimer(this);
                connect(d->timer, SIGNAL(timeout()), this, SLOT(hideTimedPopup()));
            }

            d->timer->start(popupDuration);
        }
    }
}

void PopupApplet::hidePopup()
{
    if (d->dialog) {
        if (location() != Floating) {
            d->dialog.data()->animatedHide(locationToInverseDirection(location()));
        } else {
            d->dialog.data()->hide();
        }
    }
}

void PopupApplet::togglePopup()
{
    d->internalTogglePopup();
}

Plasma::PopupPlacement PopupApplet::popupPlacement() const
{
    return d->popupPlacement;
}

void PopupApplet::popupEvent(bool)
{
}

void PopupApplet::setPassivePopup(bool passive)
{
    d->passive = passive;
}

bool PopupApplet::isPassivePopup() const
{
    return d->passive;
}

bool PopupApplet::isPopupShowing() const
{
    return d->dialog && d->dialog.data()->isVisible();
}

PopupAppletPrivate::PopupAppletPrivate(PopupApplet *applet)
        : q(applet),
          icon(0),
          widget(0),
          graphicsWidget(0),
          popupPlacement(Plasma::FloatingPopup),
          savedAspectRatio(Plasma::InvalidAspectRatioMode),
          timer(0),
          popupLostFocus(false),
          passive(false)
{
}

PopupAppletPrivate::~PopupAppletPrivate()
{
    if (proxy) {
        proxy.data()->setWidget(0);
    }

    delete dialog.data();
    delete icon;
}

void PopupAppletPrivate::internalTogglePopup()
{
    if (!dialog) {
        q->setFocus(Qt::ShortcutFocusReason);
        return;
    }

    if (!q->view()) {
        return;
    }

    if (timer) {
        timer->stop();
    }

    if (dialog.data()->isVisible()) {
        if (q->location() != Floating) {
            dialog.data()->animatedHide(locationToInverseDirection(q->location()));
        } else {
            dialog.data()->hide();
        }

        dialog.data()->clearFocus();
    } else {
        if (q->graphicsWidget() &&
            q->graphicsWidget() == static_cast<Applet*>(q)->d->extender &&
            static_cast<Applet*>(q)->d->extender->isEmpty()) {
            // we have nothing to show, so let's not.
            return;
        }

        ToolTipManager::self()->hide(q);
        updateDialogPosition();

        KWindowSystem::setOnAllDesktops(dialog.data()->winId(), true);
        KWindowSystem::setState(dialog.data()->winId(), NET::SkipTaskbar | NET::SkipPager);

        if (q->location() != Floating) {
            dialog.data()->animatedShow(locationToDirection(q->location()));
        } else {
            dialog.data()->show();
        }

        if (!(dialog.data()->windowFlags() & Qt::X11BypassWindowManagerHint)) {
            KWindowSystem::activateWindow(dialog.data()->winId());
        }
    }
}

void PopupAppletPrivate::hideTimedPopup()
{
    timer->stop();
    q->hidePopup();
}

void PopupAppletPrivate::clearPopupLostFocus()
{
    popupLostFocus = false;
}

void PopupAppletPrivate::dialogSizeChanged()
{
    //Reposition the dialog
    if (dialog) {
        KConfigGroup *mainGroup = static_cast<Applet*>(q)->d->mainConfigGroup();
        KConfigGroup sizeGroup(mainGroup, "PopupApplet");
        sizeGroup.writeEntry("DialogHeight", dialog.data()->height());
        sizeGroup.writeEntry("DialogWidth", dialog.data()->width());

        updateDialogPosition();

        emit q->configNeedsSaving();
    }
}

void PopupAppletPrivate::dialogStatusChanged(bool status)
{
    q->popupEvent(status);
}

void PopupAppletPrivate::updateDialogPosition()
{
    if (!dialog) {
        return;
    }

    QGraphicsView *view = q->view();

    if (!view) {
        return;
    }

    KConfigGroup *mainGroup = static_cast<Applet*>(q)->d->mainConfigGroup();
    KConfigGroup sizeGroup(mainGroup, "PopupApplet");

    Corona *corona = qobject_cast<Corona *>(q->scene());
    Q_ASSERT(corona);

    int preferredWidth = 0;
    int preferredHeight = 0;
    if (dialog.data()->graphicsWidget()) {
        preferredWidth = dialog.data()->graphicsWidget()->preferredSize().width();
        preferredHeight = dialog.data()->graphicsWidget()->preferredSize().height();
    }

    const int width = qMin(sizeGroup.readEntry("DialogWidth", preferredWidth),
                           corona->screenGeometry(-1).width() - 50);
    const int height = qMin(sizeGroup.readEntry("DialogHeight", preferredHeight),
                            corona->screenGeometry(-1).height() - 50);

    QSize saved(width, height);

    if (saved.isNull()) {
        saved = dialog.data()->sizeHint();
    } else {
        saved = saved.expandedTo(dialog.data()->minimumSizeHint());
    }

    if (saved.width() != dialog.data()->width() || saved.height() != dialog.data()->height()) {
        dialog.data()->resize(saved);
    }

    QSize s = dialog.data()->size();
    QPoint pos = view->mapFromScene(q->scenePos());

    pos = corona->popupPosition(q, s);

    bool reverse = false;
    if (q->formFactor() == Plasma::Vertical) {
        if (view->mapToGlobal(view->mapFromScene(q->scenePos())).y() + q->size().height()/2 < pos.y() + dialog.data()->size().width()/2) {
            reverse = true;
        }
    } else {
        if (view->mapToGlobal(view->mapFromScene(q->scenePos())).x() + q->size().width()/2 < pos.x() + dialog.data()->size().width()/2) {
            reverse = true;
        }
    }

    switch (q->location()) {
    case BottomEdge:
        if (pos.x() >= q->pos().x()) {
            dialog.data()->setResizeHandleCorners(Dialog::NorthEast);
        } else {
            dialog.data()->setResizeHandleCorners(Dialog::NorthWest);
        }

        if (reverse) {
            popupPlacement = Plasma::TopPosedLeftAlignedPopup;
        } else {
            popupPlacement = Plasma::TopPosedRightAlignedPopup;
        }
        break;
    case TopEdge:
        if (pos.x() >= q->pos().x()) {
            dialog.data()->setResizeHandleCorners(Dialog::SouthEast);
        } else {
            dialog.data()->setResizeHandleCorners(Dialog::SouthWest);
        }

        if (reverse) {
            popupPlacement = Plasma::BottomPosedLeftAlignedPopup;
        } else {
            popupPlacement = Plasma::BottomPosedRightAlignedPopup;
        }
        break;
    case LeftEdge:
        if (pos.y() >= q->pos().y()) {
            dialog.data()->setResizeHandleCorners(Dialog::SouthEast);
        } else {
            dialog.data()->setResizeHandleCorners(Dialog::NorthEast);
        }

        if (reverse) {
            popupPlacement = Plasma::RightPosedTopAlignedPopup;
        } else {
            popupPlacement = Plasma::RightPosedBottomAlignedPopup;
        }
        break;

    case RightEdge:
        if (pos.y() >= q->pos().y()) {
            dialog.data()->setResizeHandleCorners(Dialog::SouthWest);
        } else {
            dialog.data()->setResizeHandleCorners(Dialog::NorthWest);
        }

        if (reverse) {
            popupPlacement = Plasma::LeftPosedTopAlignedPopup;
        } else {
            popupPlacement = Plasma::LeftPosedBottomAlignedPopup;
        }
        break;
    default:
        dialog.data()->setResizeHandleCorners(Dialog::NorthEast);
    }

    dialog.data()->move(pos);
}

} // Plasma namespace

#include "popupapplet.moc"

