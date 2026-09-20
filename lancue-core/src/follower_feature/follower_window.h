#pragma once

#include <QColor>
#include <QWidget>

#include "corelib/i18n/language.h"
#include "corelib/ui/app_theme_mode.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QPropertyAnimation;
class QScreen;
class QTimer;
QT_END_NAMESPACE

namespace lancue::ui {

// The follower's actual pill shape: a filled rounded rect, then a
// stroked border rounded rect on top of it, both custom-painted with
// QPainter — mirrors ToastAccentStripe's own custom-paint technique
// (see that class in toast_window.h) rather than styling this via
// setStyleSheet() the way this file's first round did.
//
// Why the switch (round 2 feedback): QPainter::setBrush(QColor) is
// guaranteed to respect that QColor's own alpha channel directly, no
// string parsing involved — removing any doubt about whether a
// translucent background-color set via a QSS hex string actually
// renders correctly through this project's WA_TranslucentBackground +
// WA_NoSystemBackground top-level-window combination. That's the exact
// same guarantee ToastAccentStripe's own colored capsule already
// relies on, already proven on real hardware — reusing a proven
// mechanism instead of trusting a QSS code path this project had never
// actually exercised with alpha before.
class FollowerCard final : public QWidget {
    Q_OBJECT

public:
    explicit FollowerCard(QWidget* parent = nullptr);

    void setBackgroundColor(const QColor& color);
    void setBorderColor(const QColor& color);
    void setBorderWidth(int width);
    void setCornerRadius(int radius);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor m_backgroundColor;
    QColor m_borderColor;
    int m_borderWidth = 3;
    int m_cornerRadius = 12;
};

// The mouse-follower indicator (Phase 8). FollowerFeature keeps one
// instance alive for the whole daemon lifetime, same lifecycle
// convention as ToastWindow — see that class's own comment. Unlike
// ToastWindow, there is only ever one of these regardless of monitor
// count: the follower is a single indicator that crosses monitors with
// the cursor, not something shown per-screen.
//
// Frameless, semi-transparent, always-on-top, never steals focus — the
// same window-flags/attributes combination as ToastWindow (proven
// correct on real Windows hardware there; see that class's own comment
// for why each one is needed), reused here rather than rediscovered.
//
// Shows `i18n::layoutBadgeCode()` — the same two-letter Latin code
// ToastWindow's own badge already uses — rather than the full localized
// layout name: always exactly two characters regardless of the layout
// or the app's own UI language (Mahdi's round-2 request), so the pill's
// width never has to change with what it's currently showing, and stays
// legible at a glance in a script every layout's own code is already
// written in on the toast (Latin), rather than switching script with the
// app language.
class FollowerWindow final : public QWidget {
    Q_OBJECT

public:
    explicit FollowerWindow(QWidget* parent = nullptr);

    // Shows (or re-populates, if already visible) the indicator at
    // `globalPos`. `badgeCode` is i18n::layoutBadgeCode()'s own output —
    // already exactly two characters, not further localized here.
    // `durationMs` is how long it stays visible before auto-hiding.
    // `cardOpacity` (0-1) sets the card background's alpha, independent
    // of ToastWindow's own — see FollowerFeature's own comment.
    // `accentColorOverride`: an invalid QColor falls back to the theme's
    // neutral border; a valid one (the layout's assigned swatch, same
    // lookup ToastFeature already does) becomes the pill's border color
    // instead — see follower_window.cpp's own comment on why a border,
    // not a fill or a stripe, was chosen for a pill this small.
    void showFollowing(const QString& badgeCode, i18n::AppLanguage language, const QPoint& globalPos, int durationMs,
                        ui::AppThemeMode themeMode, qreal cardOpacity, const QColor& accentColorOverride = QColor());

    // Re-anchors an already-visible indicator to a new cursor position —
    // called on every real MouseMoved event while showing. No-op if not
    // currently showing. Also re-applies size-class metrics (padding,
    // font size, fixed badge width) if the cursor has crossed onto a
    // differently-sized monitor since the last call — checked here
    // rather than unconditionally re-applied on every call, since this
    // runs far more often than a toast ever shows or the follower is
    // (re)shown from scratch (showFollowing() above always refreshes
    // metrics unconditionally; this only does when it actually has to).
    void updatePosition(const QPoint& globalPos);

    // Fades out and hides if visible; no-op otherwise.
    void hideFollower();

    bool isShowingFollower() const;

    // Updates the auto-hide timer's duration on an already-visible
    // indicator — mirrors ToastWindow::updateDuration() exactly (same
    // live-preview requirement, same reasoning). No-op if not currently
    // showing. Opacity is deliberately NOT live-updated the same way —
    // see FollowerFeature's own comment on why that matches
    // ToastFeature's own precedent of only duration getting this
    // treatment.
    void updateDuration(int durationMs);

private:
    void positionAt(const QPoint& globalPos);
    void applyMetricsForScreen(const QScreen* screen);
    void applyPalette();
    void startHideTimer(int durationMs);
    void beginFadeIn();
    void beginFadeOut();

    FollowerCard* m_card = nullptr;
    QLabel* m_label = nullptr;

    QPropertyAnimation* m_fadeAnimation = nullptr;
    QTimer* m_hideTimer = nullptr;

    bool m_showing = false;

    // Everything applyPalette() needs to redraw without a fresh
    // showFollowing() call (re-used when applyMetricsForScreen() changes
    // the font size a screen crossing requires — the colors don't change
    // mid-show, only the metrics do, so they're cached rather than
    // threaded through every internal call that might touch the
    // rendering).
    bool m_dark = false;
    qreal m_cardOpacity = 0.85;
    QColor m_accentColorOverride;
};

} // namespace lancue::ui
