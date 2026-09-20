#include "toast_feature/toast_window.h"

#include <algorithm>
#include <cmath>

#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPropertyAnimation>
#include <QScreen>
#include <QSizePolicy>
#include <QStyleHints>
#include <QTimer>
#include <QVBoxLayout>

#include "ui/fonts.h"

namespace lancue::ui {

namespace {

// Follows the OS light/dark setting via QStyleHints::colorScheme() —
// there's no in-app theme toggle yet (that'll be a Phase 9/10 thing).
// Colors are Mahdi's own palette (2026-08-28), one set per theme.
struct TogglePalette {
    QColor cardBackground;
    QColor cardBorder;
    QColor titleText;
    QColor subtitleText;
    QColor iconAccent;
    QColor closeGlyph;
    QColor closeHoverBackground;
};

TogglePalette paletteFor(bool dark) {
    TogglePalette p;
    if (dark) {
        p.cardBackground = QColor(QStringLiteral("#0F172A"));
        p.cardBorder = QColor(255, 255, 255, 20);
        p.titleText = QColor(QStringLiteral("#E5E7EB"));
        p.subtitleText = QColor(QStringLiteral("#E5E7EB"));
        p.subtitleText.setAlphaF(0.68f);
        p.iconAccent = QColor(QStringLiteral("#047857"));
        p.closeGlyph = QColor(QStringLiteral("#E5E7EB"));
        p.closeGlyph.setAlphaF(0.55f);
        p.closeHoverBackground = QColor(255, 255, 255, 22);
    } else {
        p.cardBackground = QColor(QStringLiteral("#F5F7FA"));
        p.cardBorder = QColor(QStringLiteral("#E5E7EB"));
        p.titleText = QColor(QStringLiteral("#111827"));
        p.subtitleText = QColor(QStringLiteral("#111827"));
        p.subtitleText.setAlphaF(0.62f);
        p.iconAccent = QColor(QStringLiteral("#2563EB"));
        p.closeGlyph = QColor(QStringLiteral("#111827"));
        p.closeGlyph.setAlphaF(0.45f);
        p.closeHoverBackground = QColor(0, 0, 0, 18);
    }
    return p;
}

bool isDarkModeActive() {
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

// Auto reads the OS; Light/Dark force it regardless.
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

// Sizes are picked per size class (Compact/Regular) and handed to Qt as
// plain DIP values — Qt already scales those correctly per-screen, so we
// don't touch devicePixelRatio ourselves.
struct ToastMetrics {
    int width;
    int horizontalPadding;
    int verticalPadding;
    int spacing;
    int iconBadgeSize;
    int closeButtonSize;
    qreal titlePointSize;
    qreal subtitlePointSize;
    int cornerRadius;
};

ToastMetrics computeToastMetrics(const QScreen* screen) {
    // 1280px is roughly the small-laptop-display cutoff — below that a
    // 360px toast starts to feel cramped on a narrow secondary monitor.
    const int logicalWidth = screen ? screen->availableGeometry().width() : 1920;
    const bool compact = logicalWidth < 1280;

    ToastMetrics m;
    if (compact) {
        m.width = 300;
        m.horizontalPadding = 14;
        m.verticalPadding = 26;
        m.spacing = 12;
        m.iconBadgeSize = 30;
        m.closeButtonSize = 20;
        m.titlePointSize = 10.0;
        m.subtitlePointSize = 9.0;
        m.cornerRadius = 14;
    } else {
        m.width = 360;
        m.horizontalPadding = 18;
        m.verticalPadding = 32;
        m.spacing = 14;
        m.iconBadgeSize = 34;
        m.closeButtonSize = 22;
        m.titlePointSize = 11.0;
        m.subtitlePointSize = 10.0;
        m.cornerRadius = 16;
    }

    if (screen) {
        // Cap at 90% of the screen's own width so it never overflows a
        // small secondary monitor.
        m.width = std::min(m.width, static_cast<int>(screen->availableGeometry().width() * 0.9));
    }

    return m;
}

} // namespace

// ---- ToastCloseButton --------------------------------------------------

ToastCloseButton::ToastCloseButton(QWidget* parent) : QAbstractButton(parent) {
    setCursor(Qt::PointingHandCursor);
    setAccessibleName(QStringLiteral("Close"));
    setToolTip(QStringLiteral("Close"));
    setAttribute(Qt::WA_Hover, true);
}

void ToastCloseButton::setColors(const QColor& glyphColor, const QColor& hoverBackground) {
    m_glyphColor = glyphColor;
    m_hoverBackground = hoverBackground;
    update();
}

void ToastCloseButton::setGlyphSize(int squareSizePx) {
    setFixedSize(squareSizePx, squareSizePx);
}

void ToastCloseButton::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF bounds = rect();

    if (underMouse() && m_hoverBackground.alpha() > 0) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_hoverBackground);
        painter.drawEllipse(bounds);
    }

    // Two-line X, inset from the edges, scaled to the button's own size.
    const qreal inset = bounds.width() * 0.30;
    const qreal penWidth = std::max(1.4, bounds.width() * 0.09);

    QPen pen(m_glyphColor);
    pen.setWidthF(penWidth);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);

    painter.drawLine(QPointF(bounds.left() + inset, bounds.top() + inset),
                      QPointF(bounds.right() - inset, bounds.bottom() - inset));
    painter.drawLine(QPointF(bounds.right() - inset, bounds.top() + inset),
                      QPointF(bounds.left() + inset, bounds.bottom() - inset));
}

// ---- ToastAccentStripe --------------------------------------------------

ToastAccentStripe::ToastAccentStripe(QWidget* parent) : QWidget(parent) {
    // Decorative only — let clicks/hover pass through to m_card underneath.
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    // Not WA_TranslucentBackground: that's for top-level windows asking
    // for their own alpha surface. On a plain child widget it caused
    // real rendering corruption on Windows. WA_NoSystemBackground is the
    // right one here — it just stops Qt from erasing this widget before
    // paint, so unpainted pixels keep showing whatever m_card already
    // drew underneath.
    setAttribute(Qt::WA_NoSystemBackground, true);
}

void ToastAccentStripe::setColor(const QColor& color) {
    m_color = color;
    update();
}

void ToastAccentStripe::paintEvent(QPaintEvent* /*event*/) {
    if (!m_color.isValid() || width() <= 0 || height() <= 0) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_color);

    // Capsule shape — corner radius = half the width gives a full
    // semicircular cap top and bottom.
    painter.drawRoundedRect(rect(), width() / 2.0, width() / 2.0);
}

// ---- ToastWindow --------------------------------------------------------

ToastWindow::ToastWindow(QWidget* parent) : QWidget(parent) {
    // Frameless, always-on-top, no taskbar entry, and never takes focus
    // (both flags below matter — one tells the window manager, the
    // other tells Qt not to request activation on show()).
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    // Needed for the rounded card underneath to actually show the
    // desktop through its corners instead of a black box.
    setAttribute(Qt::WA_TranslucentBackground, true);
    // WA_TranslucentBackground alone doesn't stop Qt from still erasing
    // this window with its default color before each repaint — without
    // this, a faint rectangle showed through wherever the rounded card
    // doesn't fully cover this window's corners.
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, false);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    m_card = new QWidget(this);
    m_card->setObjectName(QStringLiteral("ToastCard"));
    // Plain QWidget ignores stylesheet background/border/radius unless
    // this is set.
    m_card->setAttribute(Qt::WA_StyledBackground, true);
    outer->addWidget(m_card);
    // No QGraphicsDropShadowEffect here on purpose — it renders into a
    // rectangular buffer, which leaked past the card's rounded corners
    // as faint square patches (the same "korners" class of bug some
    // compositors have a name for). The 1px border below is the card's
    // only edge treatment now.

    // Sits beside m_card, not inside it, so it can overlap the card's
    // edge without being clipped to it. Positioned in
    // applyMetricsAndPosition() once we know the card's final size.
    m_accentStripe = new ToastAccentStripe(this);

    auto* content = new QHBoxLayout(m_card);

    m_iconBadge = new QLabel(m_card);
    m_iconBadge->setObjectName(QStringLiteral("ToastIconBadge"));
    m_iconBadge->setText(QStringLiteral("LC"));
    m_iconBadge->setAlignment(Qt::AlignCenter);

    auto* textColumn = new QVBoxLayout();
    textColumn->setSpacing(2);
    m_titleLabel = new QLabel(m_card);
    m_subtitleLabel = new QLabel(m_card);
    // Needs to expand so alignment (left/right for RTL) has room to act —
    // a label otherwise only ever takes its own text's natural width.
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_subtitleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    textColumn->addWidget(m_titleLabel);
    textColumn->addWidget(m_subtitleLabel);

    m_closeButton = new ToastCloseButton(m_card);

    // Fixed order regardless of language — RTL mirroring comes from
    // flipping the layout's direction in applyContent(), not from
    // reordering these.
    content->addWidget(m_iconBadge);
    content->addLayout(textColumn, /*stretch=*/1);
    content->addWidget(m_closeButton);

    connect(m_closeButton, &QAbstractButton::clicked, this, &ToastWindow::dismiss);

    m_fadeAnimation = new QPropertyAnimation(this, "windowOpacity", this);

    m_dismissTimer = new QTimer(this);
    m_dismissTimer->setSingleShot(true);
    connect(m_dismissTimer, &QTimer::timeout, this, &ToastWindow::dismiss);

    setWindowOpacity(0.0);
}

void ToastWindow::applyContent(const QString& titleText, const QString& subtitleText, const QString& layoutId,
                                i18n::AppLanguage language, bool dark, const QColor& accentColorOverride,
                                qreal cardOpacity) {
    TogglePalette palette = paletteFor(dark);
    palette.cardBackground.setAlphaF(static_cast<float>(std::clamp(cardOpacity, 0.0, 1.0)));
    const auto direction = i18n::directionFor(language);
    const bool rtl = direction == i18n::LayoutDirection::RightToLeft;

    // An explicit per-layout color wins; otherwise fall back to the
    // theme's default accent, same as before per-layout colors existed.
    const QColor effectiveAccent = accentColorOverride.isValid() ? accentColorOverride : palette.iconAccent;

    // Only the layout's direction controls mirroring here — don't also
    // call setLayoutDirection() on the widgets, or the two mirror twice
    // and cancel out.
    if (auto* content = qobject_cast<QHBoxLayout*>(m_card->layout())) {
        content->setDirection(rtl ? QBoxLayout::RightToLeft : QBoxLayout::LeftToRight);
    }

    // AlignAbsolute matters: without it, word-wrapped RTL text
    // re-flips AlignRight based on its own auto-detected paragraph
    // direction, cancelling the explicit alignment back out.
    const Qt::Alignment textAlign = (rtl ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignAbsolute;

    m_titleLabel->setText(titleText);
    m_titleLabel->setAlignment(textAlign | Qt::AlignVCenter);
    m_titleLabel->setStyleSheet(QStringLiteral("color: %1;").arg(palette.titleText.name(QColor::HexArgb)));

    m_subtitleLabel->setText(subtitleText);
    m_subtitleLabel->setAlignment(textAlign | Qt::AlignVCenter);
    m_subtitleLabel->setStyleSheet(QStringLiteral("color: %1;").arg(palette.subtitleText.name(QColor::HexArgb)));

    m_iconBadge->setText(i18n::layoutBadgeCode(layoutId));
    // Full stylesheet (including radius) is built in
    // applyMetricsAndPosition() — see that method's own comment.
    m_badgeAccent = effectiveAccent;

    m_cardBackground = palette.cardBackground;
    m_cardBorder = palette.cardBorder;

    // Stripe goes on the same side as the badge, so it flips with RTL
    // too. Only the color's set here — geometry happens once we know
    // the card's final size, in applyMetricsAndPosition().
    m_accentStripe->setColor(effectiveAccent);
    m_accentOnRightEdge = rtl;

    m_closeButton->setColors(palette.closeGlyph, palette.closeHoverBackground);
}

void ToastWindow::applyMetricsAndPosition(QScreen* screen) {
    const ToastMetrics metrics = computeToastMetrics(screen);

    m_card->layout()->setContentsMargins(metrics.horizontalPadding, metrics.verticalPadding,
                                          metrics.horizontalPadding, metrics.verticalPadding);
    m_card->layout()->setSpacing(metrics.spacing);
    // Built as one complete stylesheet, not appended across two calls —
    // appending to setStyleSheet(styleSheet() + more) here used to leave
    // the very first toast of a run with square corners (only later
    // shows picked up the radius), since Qt doesn't reliably apply a
    // stylesheet change on top of another one from the same call stack
    // before the widget's first polish. One atomic setStyleSheet() call
    // per widget avoids that entirely.
    m_card->setStyleSheet(QStringLiteral("#ToastCard { background-color: %1; border: 1px solid %2; "
                                          "border-radius: %3px; }")
                               .arg(m_cardBackground.name(QColor::HexArgb), m_cardBorder.name(QColor::HexArgb))
                               .arg(metrics.cornerRadius));

    m_iconBadge->setFixedSize(metrics.iconBadgeSize, metrics.iconBadgeSize);
    m_iconBadge->setStyleSheet(QStringLiteral("background-color: %1; color: white; font-weight: 600; "
                                               "border-radius: %2px;")
                                    .arg(m_badgeAccent.name())
                                    .arg(metrics.iconBadgeSize / 2 - 2));

    m_closeButton->setGlyphSize(metrics.closeButtonSize);

    // Only the point size changes here — showToast() already set the
    // right font family before calling this.
    QFont resolvedTitleFont = m_titleLabel->font();
    resolvedTitleFont.setPointSizeF(metrics.titlePointSize);
    m_titleLabel->setFont(resolvedTitleFont);

    QFont resolvedSubtitleFont = m_subtitleLabel->font();
    resolvedSubtitleFont.setPointSizeF(metrics.subtitlePointSize);
    m_subtitleLabel->setFont(resolvedSubtitleFont);

    setFixedWidth(metrics.width);
    m_card->setFixedWidth(metrics.width);
    // Height follows the content instead of being fixed — a hardcoded
    // number would clip longer translated strings or leave dead space
    // for shorter ones.
    m_titleLabel->setWordWrap(true);
    m_subtitleLabel->setWordWrap(true);
    adjustSize();

    // Stripe geometry, now that adjustSize() has settled the window's
    // final height. Kept flush inside [0, width()) rather than
    // straddling the edge — this is a real top-level window, so
    // anything outside its bounds just gets clipped, it doesn't "poke
    // out" the way an overlapping child widget would.
    //
    // verticalInset must be at least the card's own corner radius: the
    // capsule keeps its full width right up to its rounded cap (unlike
    // the tapered lens this replaced, which thinned out near the tips),
    // so anything less than cornerRadius here lets the cap sit inside
    // the region the card itself is still curving through, poking past
    // the card's own rounded corner. Using cornerRadius directly instead
    // of a guessed number makes this correct by construction rather than
    // needing another round of eyeballing.
    const int stripeWidth = std::max(6, static_cast<int>(std::lround(metrics.iconBadgeSize * 0.20)));
    const int verticalInset = metrics.cornerRadius;
    const int stripeHeight = std::max(0, height() - verticalInset * 2);
    // stripeX anchors the outer edge (0, or width()-stripeWidth for
    // RTL) — growing stripeWidth only extends the stripe further
    // inward, toward the card content, never past the window edge.
    const int stripeX = m_accentOnRightEdge ? width() - stripeWidth : 0;
    m_accentStripe->setGeometry(stripeX, verticalInset, stripeWidth, stripeHeight);
    m_accentStripe->raise();

    if (!screen) {
        return;
    }

    // setScreen() associates the window with the target display for
    // DPI purposes; it doesn't move it — the move() below still does
    // that part.
    setScreen(screen);

    const QRect avail = screen->availableGeometry();
    // Same margin from whichever edge(s) a position anchors to, on all
    // nine. Not mirrored for RTL — a screen position isn't page content.
    constexpr int kMargin = 24;

    int x = 0;
    switch (m_position) {
        case ui::ToastPosition::TopLeft:
        case ui::ToastPosition::CenterLeft:
        case ui::ToastPosition::BottomLeft:
            x = avail.left() + kMargin;
            break;
        case ui::ToastPosition::TopCenter:
        case ui::ToastPosition::Center:
        case ui::ToastPosition::BottomCenter:
            x = avail.left() + (avail.width() - width()) / 2;
            break;
        case ui::ToastPosition::TopRight:
        case ui::ToastPosition::CenterRight:
        case ui::ToastPosition::BottomRight:
            x = avail.right() - width() - kMargin;
            break;
    }

    int y = 0;
    switch (m_position) {
        case ui::ToastPosition::TopLeft:
        case ui::ToastPosition::TopCenter:
        case ui::ToastPosition::TopRight:
            y = avail.top() + kMargin;
            break;
        case ui::ToastPosition::CenterLeft:
        case ui::ToastPosition::Center:
        case ui::ToastPosition::CenterRight:
            y = avail.top() + (avail.height() - height()) / 2;
            break;
        case ui::ToastPosition::BottomLeft:
        case ui::ToastPosition::BottomCenter:
        case ui::ToastPosition::BottomRight:
            y = avail.bottom() - height() - kMargin;
            break;
    }

    move(x, y);
}

void ToastWindow::startDismissTimer(int durationMs) {
    m_dismissTimer->stop();
    if (durationMs > 0) {
        m_dismissTimer->start(durationMs);
    }
    // durationMs <= 0 means "permanent" — leave the timer stopped.
}

void ToastWindow::beginFadeIn() {
    m_fadeAnimation->stop();
    m_fadeAnimation->setDuration(200);
    m_fadeAnimation->setEasingCurve(QEasingCurve::OutCubic);
    m_fadeAnimation->setStartValue(windowOpacity());
    m_fadeAnimation->setEndValue(1.0);
    m_fadeAnimation->start();
}

void ToastWindow::beginFadeOut() {
    m_fadeAnimation->stop();
    disconnect(m_fadeAnimation, &QPropertyAnimation::finished, this, nullptr);
    m_fadeAnimation->setDuration(200);
    m_fadeAnimation->setEasingCurve(QEasingCurve::InCubic);
    m_fadeAnimation->setStartValue(windowOpacity());
    m_fadeAnimation->setEndValue(0.0);
    connect(m_fadeAnimation, &QPropertyAnimation::finished, this, [this]() {
        hide();
        m_showing = false;
        emit dismissed();
    });
    m_fadeAnimation->start();
}

void ToastWindow::showToast(const QString& titleText, const QString& layoutId, i18n::AppLanguage language,
                             int durationMs, QScreen* screen, ui::ToastPosition position,
                             const QColor& accentColorOverride, qreal cardOpacity, ui::AppThemeMode themeMode) {
    const bool dark = resolveDarkMode(themeMode);

    const QFont::StyleStrategy strategy = QFont::PreferAntialias;
    QFont titleFont(titleFontFamily(language));
    titleFont.setStyleStrategy(strategy);
    QFont subtitleFont(bodyFontFamily(language));
    subtitleFont.setStyleStrategy(strategy);
    m_titleLabel->setFont(titleFont);
    m_subtitleLabel->setFont(subtitleFont);

    applyContent(titleText, i18n::toastLayoutChangedText(language), layoutId, language, dark, accentColorOverride,
                 cardOpacity);
    m_position = position; // read by applyMetricsAndPosition() below
    applyMetricsAndPosition(screen);

    m_currentDurationMs = durationMs;
    m_showing = true;

    disconnect(m_fadeAnimation, &QPropertyAnimation::finished, this, nullptr);
    show();
    raise();
    beginFadeIn();
    startDismissTimer(durationMs);
}

void ToastWindow::dismiss() {
    if (!m_showing) {
        return;
    }
    m_dismissTimer->stop();
    beginFadeOut();
}

bool ToastWindow::isShowingToast() const {
    return m_showing;
}

void ToastWindow::updateDuration(int durationMs) {
    if (!m_showing) {
        return;
    }
    m_currentDurationMs = durationMs;
    startDismissTimer(durationMs);
}

} // namespace lancue::ui
