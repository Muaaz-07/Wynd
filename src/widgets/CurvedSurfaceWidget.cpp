//
// Created by Muhammad Muaaz on 3/16/2026.
//


#include "CurvedSurfaceWidget.h"

#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>

// Keeps the widget transparent until a theme asks for the elevated curve.
CurvedSurfaceWidget::CurvedSurfaceWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
}

// Switches the decorative wave on and off when the theme changes.
void CurvedSurfaceWidget::setElevatedCurveEnabled(bool enabled)
{
    if (elevatedCurveEnabled == enabled) {
        return;
    }

    elevatedCurveEnabled = enabled;
    update();
}

// Chooses whether the painted wave uses the dark or light palette.
void CurvedSurfaceWidget::setDarkPaletteEnabled(bool enabled)
{
    if (darkPaletteEnabled == enabled) {
        return;
    }

    darkPaletteEnabled = enabled;
    update();
}

// Paints a curved lower surface whose top edge dips between the stat cards.
void CurvedSurfaceWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    if (!elevatedCurveEnabled) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);

    const QRectF bounds = rect();
    if (bounds.width() <= 0.0 || bounds.height() <= 0.0) {
        return;
    }

    const qreal width = bounds.width();
    const qreal height = bounds.height();
    const qreal leftY = qMin(138.0, height * 0.36);
    const qreal midY = qMin(110.0, height * 0.29);
    const qreal rightY = qMin(76.0, height * 0.20);
    const qreal crestY = qMin(64.0, height * 0.17);
    const qreal cornerRadius = 28.0;

    QPainterPath shellPath;
    shellPath.addRoundedRect(bounds.adjusted(0.0, 0.0, -1.0, -1.0),
                             cornerRadius,
                             cornerRadius);

    QLinearGradient skyGradient(0.0, 0.0, 0.0, leftY + 36.0);
    if (darkPaletteEnabled) {
        skyGradient.setColorAt(0.0, QColor(11, 70, 91));
        skyGradient.setColorAt(0.55, QColor(15, 124, 149));
        skyGradient.setColorAt(1.0, QColor(24, 182, 200));
    } else {
        skyGradient.setColorAt(0.0, QColor(9, 94, 123));
        skyGradient.setColorAt(0.55, QColor(17, 142, 171));
        skyGradient.setColorAt(1.0, QColor(27, 200, 213));
    }
    painter.fillPath(shellPath, QBrush(skyGradient));

    painter.save();
    painter.setClipPath(shellPath);

    QPainterPath surfacePath;
    surfacePath.moveTo(0.0, leftY);
    surfacePath.cubicTo(width * 0.18, leftY + 28.0,
                        width * 0.34, leftY + 18.0,
                        width * 0.50, midY);
    surfacePath.cubicTo(width * 0.66, crestY,
                        width * 0.82, crestY - 4.0,
                        width, rightY);
    surfacePath.lineTo(width, height);
    surfacePath.lineTo(0.0, height);
    surfacePath.closeSubpath();

    const QColor shadowColor = darkPaletteEnabled
        ? QColor(12, 24, 30)
        : QColor(108, 116, 130);

    // Layers a soft shadow under the sweep so it feels elevated in both themes.
    for (int offset = 18; offset >= 6; offset -= 3) {
        const int alpha = 6 + (18 - offset) * 2;
        painter.fillPath(
            surfacePath.translated(0.0, static_cast<qreal>(offset)),
            QColor(shadowColor.red(), shadowColor.green(), shadowColor.blue(), alpha)
        );
    }

    painter.fillPath(
        surfacePath,
        darkPaletteEnabled
            ? QColor(20, 39, 48, 248)
            : QColor(248, 244, 238, 250)
    );
    painter.restore();
}
