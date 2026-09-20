#pragma once

#include <QAbstractButton>
#include <QColor>
#include <QWidget>

#include "corelib/i18n/language.h"
#include "corelib/ui/app_theme_mode.h"
#include "corelib/ui/toast_position.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QPropertyAnimation;
class QScreen;
class QTimer;
QT_END_NAMESPACE

namespace lancue::ui {

// X close button, hand-drawn instead of an icon asset — stays crisp at
// any scale and there's no file to ship.
class ToastCloseButton final : public QAbstractButton {
    Q_OBJECT

public:
    explicit ToastCloseButton(QWidget* parent = nullptr);

    // Colors/size come from ToastWindow's per-show theme/metrics calc.
    void setColors(const QColor& glyphColor, const QColor& hoverBackground);
    void setGlyphSize(int squareSizePx);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor m_glyphColor;
    QColor m_hoverBackground;
};

// Colored capsule on the card's leading edge, one color per keyboard
// layout. Sits beside m_card (not inside it) so it can overlap the
// card's edge without getting clipped — see applyMetricsAndPosition()
// for how it's placed.
class ToastAccentStripe final : public QWidget {
    Q_OBJECT

public:
    explicit ToastAccentStripe(QWidget* parent = nullptr);

    void setColor(const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor m_color;
};

// The toast notification window. ToastFeature keeps one instance alive
// for the whole daemon lifetime and just calls showToast()/dismiss() on
// it repeatedly rather than creating a new one each time.
//
// Frameless, always-on-top, and never steals focus.
class ToastWindow final : public QWidget {
    Q_OBJECT

public:
    explicit ToastWindow(QWidget* parent = nullptr);

    // Shows (or re-populates, if already visible) the toast. `titleText`
    // is already localized by the caller; `layoutId` is the raw
    // "en-us"/"fa-ir"-style id, used here to derive the badge's 2-letter
    // code. `screen` and `position` say where to show it. `durationMs`
    // is a real duration or kToastDurationPermanentMs.
    // `accentColorOverride`: a layout's assigned color, or an invalid
    // QColor to fall back to the theme's default accent. `cardOpacity`
    // is 0.0-1.0, already resolved by the caller from
    // Settings::toastOpacityPercent. `themeMode`: Auto follows the OS
    // setting (the only behavior before this existed), Light/Dark force
    // it regardless of the OS.
    void showToast(const QString& titleText, const QString& layoutId, i18n::AppLanguage language, int durationMs,
                    QScreen* screen, ui::ToastPosition position = ui::ToastPosition::BottomRight,
                    const QColor& accentColorOverride = QColor(), qreal cardOpacity = 0.85,
                    ui::AppThemeMode themeMode = ui::AppThemeMode::Auto);

    // Fades out and hides if visible; no-op otherwise.
    void dismiss();

    // True from showToast() until it's fully hidden, including mid
    // fade-out — used to avoid showing a duplicate toast.
    bool isShowingToast() const;

    // Updates the auto-dismiss timer on an already-visible toast without
    // restarting it. No-op if nothing's showing.
    void updateDuration(int durationMs);

signals:
    void dismissed();

private:
    void applyContent(const QString& titleText, const QString& subtitleText, const QString& layoutId,
                       i18n::AppLanguage language, bool dark, const QColor& accentColorOverride, qreal cardOpacity);
    void applyMetricsAndPosition(QScreen* screen);
    void startDismissTimer(int durationMs);
    void beginFadeIn();
    void beginFadeOut();

    QWidget* m_card = nullptr;
    QLabel* m_iconBadge = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_subtitleLabel = nullptr;
    ToastCloseButton* m_closeButton = nullptr;
    ToastAccentStripe* m_accentStripe = nullptr;

    QPropertyAnimation* m_fadeAnimation = nullptr;
    QTimer* m_dismissTimer = nullptr;

    bool m_showing = false;
    int m_currentDurationMs = 0;
    ui::ToastPosition m_position = ui::ToastPosition::BottomRight;
    // Set in applyContent(), read back in applyMetricsAndPosition() to
    // decide which side the accent stripe goes on and to build the
    // card/badge stylesheet in one shot (see that method's own comment
    // on why it's not built across two separate setStyleSheet calls).
    bool m_accentOnRightEdge = false;
    QColor m_cardBackground;
    QColor m_cardBorder;
    QColor m_badgeAccent;
};

} // namespace lancue::ui
