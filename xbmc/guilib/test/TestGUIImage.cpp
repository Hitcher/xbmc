/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIImage.h"

#include <memory>

#include <gtest/gtest.h>

namespace
{
class CTestTexture : public CGUITexture
{
public:
  CTestTexture(float x, float y, float width, float height, const CTextureInfo& info)
    : CGUITexture(x, y, width, height, info)
  {
  }
  CGUITexture* Clone() const override { return new CTestTexture(*this); }

private:
  void Begin(KODI::UTILS::COLOR::Color color) override {}
  void End() override {}
  void Draw(float*, float*, float*, const CRect&, const CRect&, int) override {}
};

class CTestImage : public CGUIImage
{
public:
  CTestImage() : CGUIImage(0, 884, 0, 0, 100, 100, CTextureInfo())
  {
    SetInfo(KODI::GUILIB::GUIINFO::CGUIInfoLabel("$INFO[ListItem.Art(clearlogo)]"));
  }
  using CGUIImage::ProcessInstantTransition;
  using CGUIImage::ProcessState;

  void SetCurrent(const std::string& name)
  {
    SetFileName(name);
    ProcessState();
    ProcessInstantTransition();
  }
  void ResolveIncoming(const std::string& name) { m_textureNext->SetFileName(name); }
  void SetInfoFallback(const std::string& name) { m_currentFallback = name; }
  bool IncomingReady() const { return m_textureNext->ReadyToRender(); }
};

class CTestGUI : public CGUIComponent
{
public:
  CTestGUI() : CGUIComponent(false)
  {
    m_guiInfoManager = std::make_unique<CGUIInfoManager>();
    CServiceBroker::RegisterGUI(this);
  }
};

class TestGUIImage : public testing::Test
{
protected:
  static void SetUpTestSuite()
  {
    CGUITexture::Register(
        [](float x, float y, float width, float height, const CTextureInfo& info)
        { return new CTestTexture(x, y, width, height, info); },
        {});
  }
  static void TearDownTestSuite() { CGUITexture::Register({}, {}); }

private:
  CTestGUI m_gui;
};
} // namespace

TEST_F(TestGUIImage, HiddenImageReportsPendingFilenameWithoutProcessing)
{
  for (const bool dynamic : {false, true})
  {
    CTestImage image;
    image.SetVisibleCondition("false", "false");
    image.DynamicResourceAlloc(dynamic);
    CFileItem item;
    item.SetArt("clearlogo", "logo.png");
    image.UpdateVisibility(&item);
    ASSERT_FALSE(image.IsVisible());
    EXPECT_EQ(image.GetDescription(), "logo.png");
    EXPECT_TRUE(image.GetFileName().empty());
    EXPECT_FALSE(image.IncomingReady());
  }
}

TEST_F(TestGUIImage, IncomingFilenameDoesNotRequireAllocatedTexture)
{
  CTestImage image;
  image.SetCurrent("old.png");
  image.SetCrossFade(1000);
  image.SetFileName("new.png");
  image.ProcessState();
  ASSERT_FALSE(image.IncomingReady());
  EXPECT_EQ(image.GetDescription(), "new.png");
  EXPECT_EQ(image.GetFileName(), "old.png");
}

TEST_F(TestGUIImage, LatestPendingRequestReplacesIncomingDescription)
{
  CTestImage image;
  image.SetCurrent("old.png");
  image.SetFileName("incoming.png");
  image.ProcessState();
  image.SetFileName("latest.png");
  EXPECT_EQ(image.GetDescription(), "latest.png");
  EXPECT_EQ(image.GetFileName(), "old.png");
}

TEST_F(TestGUIImage, ReturningToCurrentImageCancelsIncomingDescription)
{
  CTestImage image;
  image.SetCurrent("old.png");
  image.SetFileName("incoming.png");
  image.ProcessState();
  image.SetFileName("old.png");
  EXPECT_EQ(image.GetDescription(), "old.png");
  image.ProcessState();
  EXPECT_EQ(image.GetDescription(), "old.png");
}

TEST_F(TestGUIImage, ClearingImageReportsEmptyBeforeProcessing)
{
  CTestImage image;
  image.SetCurrent("old.png");
  image.SetFileName("");
  EXPECT_TRUE(image.GetDescription().empty());
  EXPECT_EQ(image.GetFileName(), "old.png");
}

TEST_F(TestGUIImage, RepeatedFailedRequestKeepsResolvedFallback)
{
  CTestImage image;
  image.SetCurrent("old.png");
  image.SetFileName("noentry");
  image.ProcessState();
  image.ResolveIncoming("");
  EXPECT_TRUE(image.GetDescription().empty());
  image.SetFileName("noentry");
  EXPECT_TRUE(image.GetDescription().empty());
  image.ProcessState();
  image.ProcessInstantTransition();
  image.SetFileName("noentry");
  EXPECT_TRUE(image.GetDescription().empty());
}

TEST_F(TestGUIImage, ResolvedNonemptyFallbackSurvivesRepeatedRequest)
{
  CTestImage image;
  image.SetFileName("missing.png");
  image.ProcessState();
  image.ResolveIncoming("fallback.png");
  image.SetFileName("missing.png");
  EXPECT_EQ(image.GetDescription(), "fallback.png");
  image.ProcessState();
  image.ProcessInstantTransition();
  image.SetFileName("missing.png");
  EXPECT_EQ(image.GetDescription(), "fallback.png");
}

TEST_F(TestGUIImage, EmptyRequestReportsInfoFallback)
{
  CTestImage image;
  image.SetInfoFallback("fallback.png");
  image.SetFileName("");
  EXPECT_EQ(image.GetDescription(), "fallback.png");
  image.ProcessState();
  EXPECT_EQ(image.GetDescription(), "fallback.png");
}

TEST_F(TestGUIImage, LiteralTextureDescriptionIsUnchanged)
{
  CGUIImage image(0, 884, 0, 0, 100, 100, CTextureInfo("literal.png"));
  EXPECT_EQ(image.GetDescription(), "literal.png");
}

TEST_F(TestGUIImage, FailedImageStaysEmptyWhileHidden)
{
  CTestImage image;
  image.SetFileName("noentry");
  image.ProcessState();
  image.ResolveIncoming("");
  image.ProcessInstantTransition();
  image.SetVisibleCondition("false", "false");
  image.DynamicResourceAlloc(true);
  CFileItem item;
  item.SetArt("clearlogo", "noentry");
  image.UpdateVisibility(&item);
  EXPECT_TRUE(image.GetDescription().empty());
  image.UpdateVisibility(&item);
  EXPECT_TRUE(image.GetDescription().empty());
  item.SetArt("clearlogo", "new.png");
  image.UpdateVisibility(&item);
  EXPECT_EQ(image.GetDescription(), "new.png");
}
