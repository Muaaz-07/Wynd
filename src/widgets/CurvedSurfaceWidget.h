//
// Created by Muhammad Muaaz on 3/16/2026.
//
#ifndef CURVEDSURFACEWIDGET_H
#define CURVEDSURFACEWIDGET_H

#include <QWidget>

class CurvedSurfaceWidget : public QWidget
{
public:
    // Paints the curved surface that sits behind the lower dashboard content.
    explicit CurvedSurfaceWidget(QWidget *parent = nullptr);

    // Enables the elevated wave treatment used by the themed lower panels.
    void setElevatedCurveEnabled(bool enabled);
    // Switches the painted palette between the light and dark theme variants.
    void setDarkPaletteEnabled(bool enabled);

protected:
    // Draws the curved white section and its soft shadow behind the child widgets.
    void paintEvent(QPaintEvent *event) override;

private:
    bool elevatedCurveEnabled = false;
    bool darkPaletteEnabled = false;
};

#endif
