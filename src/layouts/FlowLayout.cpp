#include "FlowLayout.h"

#include <QWidget>

FlowLayout::FlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent),
      m_hSpacing(hSpacing),
      m_vSpacing(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::~FlowLayout()
{
    QLayoutItem *item = nullptr;
    while ((item = takeAt(0)) != nullptr) {
        delete item;
    }
}

void FlowLayout::addItem(QLayoutItem *item)
{
    m_items.append(item);
    invalidate();
}

int FlowLayout::horizontalSpacing() const
{
    if (m_hSpacing >= 0) {
        return m_hSpacing;
    }

    return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
}

int FlowLayout::verticalSpacing() const
{
    if (m_vSpacing >= 0) {
        return m_vSpacing;
    }

    return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
}

Qt::Orientations FlowLayout::expandingDirections() const
{
    return {};
}

bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

int FlowLayout::heightForWidth(int width) const
{
    return doLayout(QRect(0, 0, width, 0), true);
}

int FlowLayout::count() const
{
    return m_items.size();
}

QLayoutItem *FlowLayout::itemAt(int index) const
{
    return index >= 0 && index < m_items.size() ? m_items.at(index) : nullptr;
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    for (QLayoutItem *item : m_items) {
        size = size.expandedTo(item->sizeHint());
    }

    const QMargins margins = contentsMargins();
    size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    return size;
}

void FlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

QSize FlowLayout::sizeHint() const
{
    QSize size = minimumSize();

    if (const QWidget *parentWidget = qobject_cast<const QWidget *>(parent())) {
        const int width = parentWidget->contentsRect().width();
        if (width > 0) {
            size.setWidth(width);
            size.setHeight(doLayout(QRect(0, 0, width, 0), true));
        }
    }

    return size;
}

QLayoutItem *FlowLayout::takeAt(int index)
{
    if (index < 0 || index >= m_items.size()) {
        return nullptr;
    }

    QLayoutItem *item = m_items.takeAt(index);
    invalidate();
    return item;
}

int FlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    const QMargins margins = contentsMargins();
    const QRect effectiveRect = rect.adjusted(+margins.left(),
                                              +margins.top(),
                                              -margins.right(),
                                              -margins.bottom());

    int x = effectiveRect.x();
    int y = effectiveRect.y();
    int rowHeight = 0;
    const int spaceX = horizontalSpacing();
    const int spaceY = verticalSpacing();

    for (QLayoutItem *item : m_items) {
        const QSize itemSize = item->sizeHint();
        const int nextX = x + itemSize.width() + spaceX;

        if (nextX - spaceX > effectiveRect.right() && rowHeight > 0) {
            x = effectiveRect.x();
            y += rowHeight + spaceY;
            rowHeight = 0;
        }

        if (!testOnly) {
            item->setGeometry(QRect(QPoint(x, y), itemSize));
        }

        x += itemSize.width() + spaceX;
        rowHeight = qMax(rowHeight, itemSize.height());
    }

    return y + rowHeight - rect.y() + margins.bottom();
}

int FlowLayout::smartSpacing(QStyle::PixelMetric pixelMetric) const
{
    const QObject *parentObject = parent();
    if (parentObject == nullptr) {
        return -1;
    }

    if (const QWidget *parentWidget = qobject_cast<const QWidget *>(parentObject)) {
        return parentWidget->style()->pixelMetric(pixelMetric, nullptr, parentWidget);
    }

    return static_cast<const QLayout *>(parentObject)->spacing();
}
