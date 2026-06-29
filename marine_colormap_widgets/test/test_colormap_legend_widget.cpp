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

// Offscreen-Qt tests for ColormapLegendWidget. The CMake test registration sets
// QT_QPA_PLATFORM=offscreen so a QApplication can be created headless in CI.

#include <gtest/gtest.h>

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QPointF>

#include "marine_colormap/transfer.hpp"
#include "marine_colormap_widgets/colormap_legend_widget.hpp"

using marine_colormap::RangeMode;
using marine_colormap_widgets::ColormapLegendWidget;

namespace
{

// A single QApplication for the whole test binary (Qt requires one before any
// QWidget). Registered as a global environment so it is constructed inside
// RUN_ALL_TESTS, after QT_QPA_PLATFORM is in the environment.
class QtEnvironment : public ::testing::Environment
{
public:
  void SetUp() override
  {
    static int argc = 1;
    static char arg0[] = "test_colormap_legend_widget";
    static char * argv[] = {arg0, nullptr};
    app_ = new QApplication(argc, argv);
  }
  void TearDown() override {delete app_; app_ = nullptr;}

private:
  QApplication * app_{nullptr};
};

[[maybe_unused]] ::testing::Environment * const g_qt_env =
  ::testing::AddGlobalTestEnvironment(new QtEnvironment);

// Dispatch a synthetic mouse event to the widget (exercises the real
// press/move/release handlers without a visible window).
void sendMouse(
  QWidget & w, QEvent::Type type, QPointF pos,
  Qt::MouseButton button, Qt::MouseButtons buttons)
{
  QMouseEvent ev(type, pos, button, buttons, Qt::NoModifier);
  QApplication::sendEvent(&w, &ev);
}

}  // namespace

// Dragging a handle switches the model to Manual, emits rangeChanged, and the
// handles do not cross.
TEST(ColormapLegendWidget, DragSwitchesToManualAndEmits)
{
  ColormapLegendWidget w;
  w.resize(200, 40);
  w.setDomain(0.0f, 100.0f);
  w.updateAuto(0.0f, 100.0f);
  ASSERT_EQ(w.mode(), RangeMode::Auto);

  int emissions = 0;
  float last_lo = -1.0f;
  float last_hi = -1.0f;
  QObject::connect(
    &w, &ColormapLegendWidget::rangeChanged,
    [&](float lo, float hi) {++emissions; last_lo = lo; last_hi = hi;});

  // Grab the lo handle (x≈0) and drag it to the middle (x=100 -> value ≈50).
  sendMouse(w, QEvent::MouseButtonPress, QPointF(1, 20), Qt::LeftButton, Qt::LeftButton);
  sendMouse(w, QEvent::MouseMove, QPointF(100, 20), Qt::LeftButton, Qt::LeftButton);
  sendMouse(w, QEvent::MouseButtonRelease, QPointF(100, 20), Qt::LeftButton, Qt::NoButton);

  EXPECT_EQ(w.mode(), RangeMode::Manual);
  EXPECT_GT(emissions, 0);
  EXPECT_LT(w.lo(), w.hi());                 // handles did not cross
  EXPECT_NEAR(w.lo(), 50.0f, 2.0f);          // lo dragged to ~middle of the domain
  EXPECT_FLOAT_EQ(w.hi(), 100.0f);           // hi handle untouched
  EXPECT_FLOAT_EQ(last_lo, w.lo());          // signal carried the current bounds
  EXPECT_FLOAT_EQ(last_hi, w.hi());
}

// reset() returns the model to Auto and emits rangeChanged.
TEST(ColormapLegendWidget, ResetReturnsToAuto)
{
  ColormapLegendWidget w;
  w.resize(200, 40);
  w.setDomain(0.0f, 100.0f);
  w.updateAuto(0.0f, 100.0f);

  // Drag to pin the range -> Manual.
  sendMouse(w, QEvent::MouseButtonPress, QPointF(1, 20), Qt::LeftButton, Qt::LeftButton);
  sendMouse(w, QEvent::MouseMove, QPointF(100, 20), Qt::LeftButton, Qt::LeftButton);
  sendMouse(w, QEvent::MouseButtonRelease, QPointF(100, 20), Qt::LeftButton, Qt::NoButton);
  ASSERT_EQ(w.mode(), RangeMode::Manual);

  int emissions = 0;
  QObject::connect(
    &w, &ColormapLegendWidget::rangeChanged,
    [&](float, float) {++emissions;});

  w.reset();

  EXPECT_EQ(w.mode(), RangeMode::Auto);
  EXPECT_EQ(emissions, 1);
}

// Dragging the lo handle past the hi handle is clamped: lo stays strictly below
// hi (never crosses / collapses to a degenerate range).
TEST(ColormapLegendWidget, DragPastCrossingIsClamped)
{
  ColormapLegendWidget w;
  w.resize(200, 40);
  w.setDomain(0.0f, 100.0f);
  w.updateAuto(0.0f, 100.0f);

  // Drag the lo handle all the way to the right edge (value ≈100, past hi).
  sendMouse(w, QEvent::MouseButtonPress, QPointF(1, 20), Qt::LeftButton, Qt::LeftButton);
  sendMouse(w, QEvent::MouseMove, QPointF(199, 20), Qt::LeftButton, Qt::LeftButton);
  sendMouse(w, QEvent::MouseButtonRelease, QPointF(199, 20), Qt::LeftButton, Qt::NoButton);

  EXPECT_EQ(w.mode(), RangeMode::Manual);
  EXPECT_LT(w.lo(), w.hi());                 // clamp kept lo strictly below hi
  EXPECT_GT(w.lo(), 90.0f);                  // lo was pushed up near hi (hi - eps)
}
