#include "follower_feature/follower_window.h"

#include <algorithm>

#include <QEasingCurve>
#include <QFont>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QPropertyAnimation>
#include <QScreen>
#include <QStyleHints>
#include <QTimer>
#include <QVBoxLayout>

#include "ui/fonts.h"

namespace lancue::ui {

// ---- FollowerCard ---------------------------------------------------

FollowerCard::FollowerCard(QWidget* parent) : QWidget(parent) {
    // Same reasoning as ToastAccentStripe's own identical attribute (see
    // that class in toast_window.h): stops Qt from erasing this widget
    // before paint, so the corners outside the rounded rect keep
    // showing the translucent top-level window underneath instead of an
    // opaque box.
    setAttribute(Qt::WA_NoSystemBackground, true);
}

void FollowerCard::setBackgroundColor(const QColor& color) {
    m_backgroundColor = color;
    update();
}

void FollowerCard::setBorderColor(const QColor& color) {
    m_borderColor = color;
    update();
}

void FollowerCard::setBorderWidth(int width) {
    m_borderWidth = width;
    update();
}

void FollowerCard::setCornerRadius(int radius) {
    m_cornerRadius = radius;
    update();
}

void FollowerCard::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Inset by half the border's own width so a thick stroke doesn't
    // get clipped at this widget's edge (QPainter strokes are centered
    // on the path by default).
    const qreal halfPen = m_borderWidth / 2.0;
    const QRectF paintRect = QRectF(rect()).adjusted(halfPen, halfPen, -halfPen, -halfPen);
    if (paintRect.width() <= 0 || paintRect.height() <= 0) {
        return;
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(m_backgroundColor);
    painter.drawRoundedRect(paintRect, m_cornerRadius, m_cornerRadius);

    if (m_borderWidth > 0 && m_borderColor.isValid()) {
        painter.setPen(QPen(m_borderColor, m_borderWidth));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(paintRect, m_cornerRadius, m_cornerRadius);
    }
}

// ---- FollowerWindow ---------------------------------------------------

namespace {

bool isDarkModeActive() {
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

// Mirrors ToastWindow's own resolveDarkMode() exactly (same three-value
// AppThemeMode semantics, same single settings.appThemeMode source via
// FollowerFeature) — duplicated rather than shared, since the two
// windows' full palettes differ enough (this one has no icon-badge or
// close-button colors at all) that a shared struct would carry mostly-
// unused fields either way. A real shared theming module is a
// reasonable Phase 9/10 (Visual Polish) refactor once a third consumer
// needs the same colors, not before (§4.3). Both windows reading the
// same settings.appThemeMode is what actually keeps them in lockstep —
// see FollowerFeature's own comment.
bool resolveDarkMode(ui::AppThemeMode mode) {
    switch (mode) {
        case ui::AppThemeMode::Light:
            return false;
        case ui::AppThemeMode::Dark:
            return true;
        case ui::AppThemeMode::Auto:
        default:
            return isDarkModeActive();
    }
}

struct FollowerPalette {
    QColor cardBackground;
    QColor cardBorder;
    QColor text;
};

// `cardOpacity` (0-1) only ever touches the background's alpha, never
// the border's — same split ToastWindow's own showToast() already makes
// (see that file's own cardOpacity handling) — so the border stays a
// clear, fully-opaque identifier (the layout's own color, once
// `accentColorOverride` is valid) no matter how transparent the user
// has set the background.
FollowerPalette paletteFor(bool dark, qreal cardOpacity, const QColor& accentColorOverride) {
    FollowerPalette p;
    if (dark) {
        p.cardBackground = QColor(QStringLiteral("#0F172A"));
        p.cardBorder = accentColorOverride.isValid() ? accentColorOverride : QColor(255, 255, 255, 60);
        p.text = QColor(QStringLiteral("#E5E7EB"));
    } else {
        p.cardBackground = QColor(QStringLiteral("#F5F7FA"));
        p.cardBorder = accentColorOverride.isValid() ? accentColorOverride : QColor(QStringLiteral("#CBD5E1"));
        p.text = QColor(QStringLiteral("#111827"));
    }
    p.cardBackground.setAlphaF(static_cast<float>(std::clamp(cardOpacity, 0.0, 1.0)));
    return p;
}

// Cursor tip is deliberately not covered by the card — same reasoning
// any OS tooltip already follows. Bottom-right of the pointer, not
// top-right: this is the same placement real-time-cursor UI (Figma/
// Google Docs-style collaborator name tags being the closest modern
// analogue to "a small label that follows a pointer") converges on, and
// it keeps the label out from between the eye and whatever's just been
// pointed at, which top-right — sitting right over the content the
// cursor is about to move past on its way up — doesn't.
//
// Tightened in round 3 (Mahdi's own request to sit closer to the
// cursor): small enough that the card visually reads as attached to the
// pointer at a glance, without the default offset's old gap, but still
// clear of the arrow glyph's own hotspot (its top-left tip) so the card
// never sits under where a click would actually land.
constexpr int kCursorOffsetX = 10;
constexpr int kCursorOffsetY = 14;

constexpr int kCardCornerRadius = 12; // left alone per Mahdi's own request — not part of the size-class metrics below
// 3px, up from round 2's 2px (Mahdi's own "the border is too thin to
// catch my attention at a glance" feedback) — the border is this pill's
// main peripheral-vision signal (see FollowerFeature's own comment on
// why the accent color lives on the border rather than a fill or
// stripe), so it needs to read clearly without having to look straight
// at it.
constexpr int kBorderWidthPx = 3;
constexpr int kFadeDurationMs = 150;

struct FollowerMetrics {
    int horizontalPadding;
    int verticalPadding;
    qreal fontPointSize;
};

// Same compact/regular size-class split and 1280px cutoff as
// ToastWindow's own computeToastMetrics() (see that file's own
// comment for where the number comes from) — reused rather than
// re-derived, so a monitor that reads as "compact" to the toast reads
// as "compact" to the follower too. Width and corner radius are
// deliberately not here (see kCardCornerRadius above, and
// FollowerWindow::applyMetricsForScreen()'s own comment on the fixed
// badge width) — only the metrics Mahdi actually asked to scale with
// monitor size do.
FollowerMetrics computeFollowerMetrics(const QScreen* screen) {
    const int logicalWidth = screen ? screen->availableGeometry().width() : 1920;
    const bool compact = logicalWidth < 1280;

    FollowerMetrics m;
    if (compact) {
        m.horizontalPadding = 8;
        m.verticalPadding = 4;
        m.fontPointSize = 9.0;
    } else {
        m.horizontalPadding = 10;
        m.verticalPadding = 5;
        m.fontPointSize = 10.0;
    }
    return m;
}

} // namespace

FollowerWindow::FollowerWindow(QWidget* parent) : QWidget(parent) {
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    // WA_TranslucentBackground alone doesn't stop Qt from still erasing
    // to an opaque system background first on Windows — same DWM
    // compositing-corruption bug ToastWindow's own comment documents;
    // WA_NoSystemBackground is the actual fix there, reused here.
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, false);
    // Never intercepts clicks: this is a passive indicator, not a
    // control, and it constantly sits right where the user is about to
    // click.
    setAttribute(Qt::WA_TransparentForMouseEvents, true);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    m_card = new FollowerCard(this);
    outer->addWidget(m_card);

    auto* content = new QVBoxLayout(m_card);
    m_label = new QLabel(m_card);
    m_label->setAlignment(Qt::AlignCenter);
    content->addWidget(m_label);

    m_fadeAnimation = new QPropertyAnimation(this, "windowOpacity", this);

    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, &QTimer::timeout, this, &FollowerWindow::hideFollower);

    setWindowOpacity(0.0);
}

void FollowerWindow::showFollowing(const QString& badgeCode, i18n::AppLanguage language, const QPoint& globalPos,
                                    int durationMs, ui::AppThemeMode themeMode, qreal cardOpacity,
                                    const QColor& accentColorOverride) {
    m_dark = resolveDarkMode(themeMode);
    m_cardOpacity = cardOpacity;
    m_accentColorOverride = accentColorOverride;

    QFont font(titleFontFamily(language));
    font.setStyleStrategy(QFont::PreferAntialias);
    m_label->setFont(font);
    m_label->setText(badgeCode);

    QScreen* targetScreen = QGuiApplication::screenAt(globalPos);
    if (!targetScreen) {
        targetScreen = QGuiApplication::primaryScreen();
    }
    // Unconditional on every show — unlike updatePosition()'s own
    // screen-change check below, this doesn't run often enough (only on
    // a real layout change, same rate as a toast) for the extra
    // recomputation to matter, and a fresh show must never keep a stale
    // size class from wherever the indicator last was.
    if (targetScreen) {
        setScreen(targetScreen);
        applyMetricsForScreen(targetScreen);
    }
    applyPalette();

    adjustSize();
    positionAt(globalPos);

    m_showing = true;

    show();
    raise();
    beginFadeIn();
    startHideTimer(durationMs);
}

void FollowerWindow::updatePosition(const QPoint& globalPos) {
    if (!m_showing) {
        return;
    }

    QScreen* targetScreen = QGuiApplication::screenAt(globalPos);
    if (!targetScreen) {
        targetScreen = QGuiApplication::primaryScreen();
    }

    // setScreen() before re-deriving metrics/size, same DPI-crossing
    // order as ToastWindow's own applyMetricsAndPosition() uses and
    // documents. Checked first (unlike showFollowing()'s unconditional
    // refresh above): this runs on every real mouse-move while visible,
    // potentially far more often than a toast ever shows, so skipping
    // the screen/size-class recompute on the overwhelmingly common case
    // (cursor still on the same monitor) is worth the branch.
    if (targetScreen && screen() != targetScreen) {
        setScreen(targetScreen);
        applyMetricsForScreen(targetScreen);
        adjustSize();
    }

    positionAt(globalPos);
}

void FollowerWindow::applyMetricsForScreen(const QScreen* targetScreen) {
    const FollowerMetrics metrics = computeFollowerMetrics(targetScreen);

    QFont font = m_label->font();
    font.setPointSizeF(metrics.fontPointSize);
    m_label->setFont(font);

    // Fixed width sized for the widest pair i18n::layoutBadgeCode() can
    // ever produce (two uppercase Latin letters), measured against this
    // size class's own font rather than hardcoded — so the pill never
    // visibly resizes between different two-letter codes (Mahdi's own
    // request), on any monitor. "WW" specifically: the widest common
    // Latin uppercase pair in most fonts, a safe upper bound for any
    // real two-letter code this app produces.
    const QFontMetrics fm(font);
    m_label->setFixedWidth(fm.horizontalAdvance(QStringLiteral("WW")));

    if (auto* content = m_card->layout()) {
        content->setContentsMargins(metrics.horizontalPadding, metrics.verticalPadding, metrics.horizontalPadding,
                                     metrics.verticalPadding);
    }
}

void FollowerWindow::applyPalette() {
    const FollowerPalette palette = paletteFor(m_dark, m_cardOpacity, m_accentColorOverride);

    m_label->setStyleSheet(QStringLiteral("color: %1;").arg(palette.text.name(QColor::HexArgb)));

    m_card->setBackgroundColor(palette.cardBackground);
    m_card->setBorderColor(palette.cardBorder);
    m_card->setBorderWidth(kBorderWidthPx);
    m_card->setCornerRadius(kCardCornerRadius);
}

void FollowerWindow::positionAt(const QPoint& globalPos) {
    QScreen* targetScreen = screen();
    if (!targetScreen) {
        return;
    }

    const QRect avail = targetScreen->availableGeometry();

    int x = globalPos.x() + kCursorOffsetX;
    int y = globalPos.y() + kCursorOffsetY;

    // Flip to the opposite side of the cursor if the default offset
    // would run the card off this screen's edge — the same "don't just
    // clip, reposition" approach a native OS tooltip already takes.
    if (x + width() > avail.right()) {
        x = globalPos.x() - kCursorOffsetX - width();
    }
    if (y + height() > avail.bottom()) {
        y = globalPos.y() - kCursorOffsetY - height();
    }
    x = std::clamp(x, avail.left(), std::max(avail.left(), avail.right() - width()));
    y = std::clamp(y, avail.top(), std::max(avail.top(), avail.bottom() - height()));

    move(x, y);
}

void FollowerWindow::startHideTimer(int durationMs) {
    m_hideTimer->stop();
    if (durationMs > 0) {
        m_hideTimer->start(durationMs);
    }
    // durationMs <= 0 (settings::kFollowerDurationPermanentMs, -1) means
    // "permanent" — leave the timer stopped, same convention and same
    // reasoning as ToastWindow::startDismissTimer()'s own identical
    // check: no timer at all beats one with an enormous/infinite
    // timeout (§4.7). The follower has no dismiss hotkey or close
    // button of its own (unlike the toast), so in practice "permanent"
    // here means it keeps tracking the cursor until the next real
    // LayoutChanged (which just re-shows it, still permanent) or the
    // daemon exits.
}

void FollowerWindow::beginFadeIn() {
    m_fadeAnimation->stop();
    disconnect(m_fadeAnimation, &QPropertyAnimation::finished, this, nullptr);
    m_fadeAnimation->setDuration(kFadeDurationMs);
    m_fadeAnimation->setEasingCurve(QEasingCurve::OutCubic);
    m_fadeAnimation->setStartValue(windowOpacity());
    m_fadeAnimation->setEndValue(1.0);
    m_fadeAnimation->start();
}

void FollowerWindow::beginFadeOut() {
    m_fadeAnimation->stop();
    disconnect(m_fadeAnimation, &QPropertyAnimation::finished, this, nullptr);
    m_fadeAnimation->setDuration(kFadeDurationMs);
    m_fadeAnimation->setEasingCurve(QEasingCurve::InCubic);
    m_fadeAnimation->setStartValue(windowOpacity());
    m_fadeAnimation->setEndValue(0.0);
    connect(m_fadeAnimation, &QPropertyAnimation::finished, this, [this]() {
        hide();
        m_showing = false;
    });
    m_fadeAnimation->start();
}

void FollowerWindow::hideFollower() {
    if (!m_showing) {
        return;
    }
    m_hideTimer->stop();
    beginFadeOut();
}

bool FollowerWindow::isShowingFollower() const {
    return m_showing;
}

void FollowerWindow::updateDuration(int durationMs) {
    if (!m_showing) {
        return;
    }
    startHideTimer(durationMs);
}

} // namespace lancue::ui
