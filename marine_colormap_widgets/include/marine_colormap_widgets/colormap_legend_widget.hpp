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

#ifndef MARINE_COLORMAP_WIDGETS__COLORMAP_LEGEND_WIDGET_HPP_
#define MARINE_COLORMAP_WIDGETS__COLORMAP_LEGEND_WIDGET_HPP_

#include <QWidget>

#include <vector>

#include "marine_colormap/color.hpp"
#include "marine_colormap/transfer.hpp"

namespace marine_colormap_widgets
{

/// Interactive colorbar legend (ADR-0002). Paints the value->color mapping for a
/// palette (or a baked LUT) and lets an operator drag the min/max handles to pin
/// a `Manual` range over an outlier band.
///
/// The widget **owns** a `marine_colormap::RangeModel` (no borrowed reference is
/// exposed): consumers read `lo()`/`hi()`/`mode()` and react to `rangeChanged`.
/// The value axis spans a **domain** `[min, max]` (the values that exist, e.g.
/// data extents — `setDomain`/`updateAuto`); the model's `[lo, hi]` is the active
/// window inside that domain. Dragging clamps the handles so they cannot cross
/// (lo <= hi - eps, hi >= lo + eps), enforcing ADR-0001's `lo <= hi` at the UI:
/// an inverted drag is prevented, not reversed.
class ColormapLegendWidget : public QWidget
{
  Q_OBJECT

public:
  explicit ColormapLegendWidget(QWidget * parent = nullptr);

  /// Current resolved range and mode (read-through to the owned model).
  float lo() const {return model_.lo();}
  float hi() const {return model_.hi();}
  marine_colormap::RangeMode mode() const {return model_.mode();}

  /// Value-axis extent the handles slide within. Inputs are ordered so
  /// `min <= max`. Does not emit `rangeChanged` (axis extent, not the range).
  void setDomain(float min, float max);
  float domainMin() const {return domain_min_;}
  float domainMax() const {return domain_max_;}

public slots:
  /// Palette (registry index) used to paint the ramp. Ignored if a non-empty LUT
  /// was set (the LUT takes precedence). Repaints; does not emit `rangeChanged`.
  void setPalette(int index);

  /// Explicit baked LUT (e.g. from `marine_colormap::bake_lut`, with gain/contrast
  /// applied). A non-empty LUT overrides the palette for the drawn ramp; pass an
  /// empty vector to fall back to the palette. Repaints; does not emit `rangeChanged`.
  void setLut(const std::vector<marine_colormap::Rgba8> & lut);

  /// Feed data extents to the owned model (a no-op while `Manual`, per
  /// `RangeModel`). The only way to drive `Auto` from outside. Emits
  /// `rangeChanged` and repaints.
  void updateAuto(float min, float max);

  /// Return the model to `Auto` (extent left as-is until the next `updateAuto`).
  /// Emits `rangeChanged(lo(), hi())` and repaints.
  void reset();

signals:
  /// Emitted when the resolved range may have changed: every drag-move that pins
  /// the range, and on `reset()`. Carries the model's current `lo()`/`hi()`.
  void rangeChanged(float lo, float hi);

protected:
  void paintEvent(QPaintEvent * event) override;
  void mousePressEvent(QMouseEvent * event) override;
  void mouseMoveEvent(QMouseEvent * event) override;
  void mouseReleaseEvent(QMouseEvent * event) override;
  void mouseDoubleClickEvent(QMouseEvent * event) override;

  QSize sizeHint() const override;

private:
  /// Which handle a drag is moving.
  enum class Handle { None, Lo, Hi };

  /// Map a domain value to an x pixel (and back), across the full widget width.
  float valueToX(float value) const;
  float xToValue(float x) const;

  /// Hit-test: the handle nearest `x` within the grab tolerance, or `None`.
  Handle handleAt(float x) const;

  /// Minimum gap between the handles, scaled to the domain span.
  float handleEps() const;

  marine_colormap::RangeModel model_;
  std::vector<marine_colormap::Rgba8> lut_;  ///< overrides palette_index_ when non-empty
  int palette_index_{0};
  float domain_min_{0.0f};
  float domain_max_{1.0f};
  Handle dragging_{Handle::None};
};

}  // namespace marine_colormap_widgets

#endif  // MARINE_COLORMAP_WIDGETS__COLORMAP_LEGEND_WIDGET_HPP_
