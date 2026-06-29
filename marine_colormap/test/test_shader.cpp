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

// GL-free sanity checks on the shared GLSL source. Compiling the shader and
// checking numeric parity against the CPU path needs an offscreen GL context;
// that validation is deferred to the first GPU consumer (the rqt QOpenGLWidget
// waterfall). Here we only assert the source is present and exposes the two
// contract functions with the expected version-agnostic shape.

#include <gtest/gtest.h>

#include <string>

#include "marine_colormap/shader.hpp"

namespace
{
std::string glsl() {return std::string(marine_colormap::colormap_glsl());}
}  // namespace

TEST(Shader, SourceIsNonEmpty)
{
  EXPECT_GT(glsl().size(), 0u);
}

TEST(Shader, DefinesContractFunctions)
{
  const std::string s = glsl();
  EXPECT_NE(s.find("float marine_colormap_normalize(float value, float lo, float hi)"),
    std::string::npos);
  EXPECT_NE(s.find("float marine_colormap_response(float t, float gain, float contrast)"),
    std::string::npos);
}

TEST(Shader, MirrorsCpuTransferOps)
{
  // Spot-check that the GLSL carries the load-bearing ops from the CPU transfer:
  // degenerate-range guard, clamp of t*gain before gamma, and the gamma branch.
  const std::string s = glsl();
  EXPECT_NE(s.find("if (!(hi > lo))"), std::string::npos);          // normalize degenerate
  EXPECT_NE(s.find("clamp(t * gain, 0.0, 1.0)"), std::string::npos);  // clip before gamma
  EXPECT_NE(s.find("pow(t, 1.0 / contrast)"), std::string::npos);   // gamma
}

TEST(Shader, IsVersionAgnostic)
{
  // No #version / precision / sampler decls -- consumers supply their own.
  const std::string s = glsl();
  EXPECT_EQ(s.find("#version"), std::string::npos);
  EXPECT_EQ(s.find("precision "), std::string::npos);
  EXPECT_EQ(s.find("sampler"), std::string::npos);
}
