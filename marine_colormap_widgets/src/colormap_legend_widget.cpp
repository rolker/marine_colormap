// Copyright 2026 Roland Arsenault
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "marine_colormap_widgets/colormap_legend_widget.hpp"

#include <QColor>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPolygonF>
#include <QRectF>
#include <QString>

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "marine_colormap/palette.hpp"

namespace marine_colormap_widgets
{

namespace
{
constexpr float kGrabPixels = 16.0f;  ///< handle hit-test tolerance
constexpr float kHandleGapFrac = 1e-3f;  ///< min handle gap as a fraction of the domain span
constexpr int kAxisHeight = 16;  ///< bottom strip reserved for tick labels

/// Local cursor position of a mouse event. `position()` is the Qt6 spelling
/// (it replaced the now-deprecated `localPos()`); on Qt5, where `position()`
/// does not yet exist, fall back to `localPos()`. Keeps both toolkits building.
QPointF eventLocalPos(const QMouseEvent * event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return event->position();
#else
  return event->localPos();
#endif
}

/// Convert a marine_colormap float color (sRGB, straight alpha) to a QColor.
QColor toQColor(const marine_colormap::Rgba & c)
{
  auto q = [](float v) -> int {
      return std::clamp(static_cast<int>(std::lround(v * 255.0f)), 0, 255);
    };
  return QColor(q(c.r), q(c.g), q(c.b), q(c.a));
}
}  // namespace

ColormapLegendWidget::ColormapLegendWidget(QWidget * parent)
: QWidget(parent)
{
  setMinimumSize(120, 32);
}

QSize ColormapLegendWidget::sizeHint() const
{
  return QSize(240, 48);
}

void ColormapLegendWidget::setDomain(float min, float max)
{
  const auto ordered = std::minmax(min, max);
  domain_min_ = ordered.first;
  domain_max_ = ordered.second;
  update();
}

void ColormapLegendWidget::setPalette(int index)
{
  palette_index_ = index;
  update();
}

void ColormapLegendWidget::setLut(const std::vector<marine_colormap::Rgba8> & lut)
{
  lut_ = lut;
  update();
}

void ColormapLegendWidget::updateAuto(float min, float max)
{
  // Capture the resolved state so a no-op (e.g. update_auto() while Manual)
  // neither repaints nor emits a spurious rangeChanged to consumers.
  const float prev_lo = model_.lo();
  const float prev_hi = model_.hi();
  const auto prev_mode = model_.mode();
  model_.update_auto(min, max);
  if (model_.lo() != prev_lo || model_.hi() != prev_hi || model_.mode() != prev_mode) {
    emit rangeChanged(model_.lo(), model_.hi());
    update();
  }
}

void ColormapLegendWidget::setManual(float lo, float hi)
{
  // Same guard as updateAuto()/reset(): only emit + repaint when the resolved
  // state actually moves (set_manual orders lo <= hi via std::minmax, so a
  // re-seed of the current Manual window is a no-op).
  const float prev_lo = model_.lo();
  const float prev_hi = model_.hi();
  const auto prev_mode = model_.mode();
  model_.set_manual(lo, hi);
  if (model_.lo() != prev_lo || model_.hi() != prev_hi || model_.mode() != prev_mode) {
    emit rangeChanged(model_.lo(), model_.hi());
    update();
  }
}

void ColormapLegendWidget::reset()
{
  const float prev_lo = model_.lo();
  const float prev_hi = model_.hi();
  const auto prev_mode = model_.mode();
  model_.reset();
  if (model_.lo() != prev_lo || model_.hi() != prev_hi || model_.mode() != prev_mode) {
    emit rangeChanged(model_.lo(), model_.hi());
    update();
  }
}

float ColormapLegendWidget::handleEps() const
{
  const float span = domain_max_ - domain_min_;
  return span > 0.0f ? span * kHandleGapFrac : 0.0f;
}

float ColormapLegendWidget::valueToX(float value) const
{
  const float span = domain_max_ - domain_min_;
  const float w = static_cast<float>(width());
  if (span <= 0.0f || w <= 1.0f) {
    return 0.0f;
  }
  const float frac = std::clamp((value - domain_min_) / span, 0.0f, 1.0f);
  return frac * (w - 1.0f);
}

float ColormapLegendWidget::xToValue(float x) const
{
  const float w = static_cast<float>(width());
  if (w <= 1.0f) {
    return domain_min_;
  }
  const float frac = std::clamp(x / (w - 1.0f), 0.0f, 1.0f);
  return domain_min_ + frac * (domain_max_ - domain_min_);
}

ColormapLegendWidget::Handle ColormapLegendWidget::handleAt(float x) const
{
  const float dist_lo = std::abs(x - valueToX(model_.lo()));
  const float dist_hi = std::abs(x - valueToX(model_.hi()));
  if (std::min(dist_lo, dist_hi) > kGrabPixels) {
    return Handle::None;
  }
  return (dist_lo <= dist_hi) ? Handle::Lo : Handle::Hi;
}

void ColormapLegendWidget::paintEvent(QPaintEvent * event)
{
  Q_UNUSED(event);
  QPainter painter(this);

  const int w = width();
  const int h = height();
  const int ramp_h = std::max(1, h - kAxisHeight);

  // Color for a normalized position: the baked LUT if one was set, else the
  // selected palette. Clamps to [0, 1] (below/above the active range render as
  // the floor/ceiling color).
  auto color_at = [this](float t) -> QColor {
      t = std::clamp(t, 0.0f, 1.0f);
      if (!lut_.empty()) {
        const int n = static_cast<int>(lut_.size());
        const int idx = std::clamp(static_cast<int>(std::lround(t * (n - 1))), 0, n - 1);
        const auto & c = lut_[static_cast<std::size_t>(idx)];
        return QColor(c.r, c.g, c.b, c.a);
      }
      const std::size_t count = marine_colormap::palette_count();
      if (count == 0) {
        return QColor(0, 0, 0, 0);  // no palette: render transparent rather than UB-clamp
      }
      const std::size_t idx = static_cast<std::size_t>(
        std::clamp<int>(palette_index_, 0, static_cast<int>(count) - 1));
      return toQColor(marine_colormap::palette(idx).sample(t));
    };

  // Ramp: one vertical column per pixel, normalized through the owned model so
  // the gradient is compressed into the active [lo, hi] window.
  for (int x = 0; x < w; ++x) {
    const float t = std::clamp(model_.normalize(xToValue(static_cast<float>(x))), 0.0f, 1.0f);
    painter.setPen(color_at(t));
    painter.drawLine(x, 0, x, ramp_h);
  }

  // Value axis: a few ticks + labels along the bottom.
  painter.setPen(Qt::black);
  constexpr int kTicks = 5;
  for (int i = 0; i < kTicks; ++i) {
    const float frac = static_cast<float>(i) / (kTicks - 1);
    const float value = domain_min_ + frac * (domain_max_ - domain_min_);
    const int x = static_cast<int>(std::lround(frac * (w - 1)));
    painter.drawLine(x, ramp_h, x, ramp_h + 3);
    const QRectF label_rect(x - 24, ramp_h + 3, 48, kAxisHeight - 3);
    painter.drawText(label_rect, Qt::AlignHCenter | Qt::AlignTop, QString::number(value, 'g', 3));
  }

  // Handle markers at lo() and hi(): a vertical line + a triangle tab on top.
  // Dark tab = lo, light tab = hi, so the two are distinguishable.
  auto draw_handle = [&](float value, const QColor & fill) {
      const float x = valueToX(value);
      painter.setPen(QPen(Qt::black, 1));
      painter.drawLine(QPointF(x, 0), QPointF(x, ramp_h));
      QPolygonF tab;
      tab << QPointF(x - 4.0, 0.0) << QPointF(x + 4.0, 0.0) << QPointF(x, 9.0);
      painter.setBrush(fill);
      painter.drawPolygon(tab);
    };
  draw_handle(model_.lo(), QColor(30, 30, 30));
  draw_handle(model_.hi(), QColor(235, 235, 235));
}

void ColormapLegendWidget::mousePressEvent(QMouseEvent * event)
{
  if (event->button() == Qt::LeftButton) {
    const Handle hit = handleAt(static_cast<float>(eventLocalPos(event).x()));
    if (hit != Handle::None) {
      dragging_ = hit;
      event->accept();
      return;
    }
  }
  QWidget::mousePressEvent(event);
}

void ColormapLegendWidget::mouseMoveEvent(QMouseEvent * event)
{
  if (dragging_ == Handle::None) {
    QWidget::mouseMoveEvent(event);
    return;
  }

  const float candidate = xToValue(static_cast<float>(eventLocalPos(event).x()));
  const float eps = handleEps();
  float lo = model_.lo();
  float hi = model_.hi();

  // Clamp so the handles cannot cross (ADR-0002): lo capped at hi - eps, hi
  // floored at lo + eps. Two-step min/max (not std::clamp) avoids a flipped
  // [lo, hi] bound assertion when the range is pinned against a domain edge.
  if (dragging_ == Handle::Lo) {
    lo = std::min(candidate, hi - eps);
    lo = std::max(lo, domain_min_);
  } else {
    hi = std::max(candidate, lo + eps);
    hi = std::min(hi, domain_max_);
  }

  model_.set_manual(lo, hi);
  emit rangeChanged(model_.lo(), model_.hi());
  update();
  event->accept();
}

void ColormapLegendWidget::mouseReleaseEvent(QMouseEvent * event)
{
  if (dragging_ != Handle::None) {
    dragging_ = Handle::None;
    event->accept();
    return;
  }
  QWidget::mouseReleaseEvent(event);
}

void ColormapLegendWidget::mouseDoubleClickEvent(QMouseEvent * event)
{
  // UX affordance for the reset() slot (the tested contract is the slot itself).
  reset();
  event->accept();
}

}  // namespace marine_colormap_widgets
