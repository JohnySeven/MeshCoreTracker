#include <gtest/gtest.h>
#include "../../examples/companion_radio/Tracker.h"
#include <helpers/sensors/NMEASatellites.h>

static bool press(Tracker& tracker, uint32_t now) {
  tracker.checkButton(true, now);
  bool result = tracker.checkButton(true, now + 30);
  tracker.checkButton(false, now + 60);
  tracker.checkButton(false, now + 90);
  return result;
}

TEST(Tracker, ReportsOncePerHourWithoutCatchupBursts) {
  Tracker tracker;
  tracker.begin(500);
  EXPECT_FALSE(tracker.locationDue(3600499));
  EXPECT_TRUE(tracker.locationDue(3600500));
  EXPECT_FALSE(tracker.locationDue(3600501));
  EXPECT_TRUE(tracker.locationDue(10800500));
  EXPECT_FALSE(tracker.locationDue(10800501));
}

TEST(Tracker, HourlyScheduleSurvivesMillisRollover) {
  Tracker tracker;
  tracker.begin(UINT32_MAX - 1000);
  EXPECT_FALSE(tracker.locationDue(3598998));
  EXPECT_TRUE(tracker.locationDue(3598999));
}

TEST(Tracker, ThreePressesInFiveSecondsTriggerOnce) {
  Tracker tracker;
  EXPECT_FALSE(press(tracker, 100));
  EXPECT_FALSE(press(tracker, 2600));
  EXPECT_TRUE(press(tracker, 5100));
  EXPECT_FALSE(press(tracker, 5300));
}

TEST(Tracker, ExpiredPressesSlideOutOfWindow) {
  Tracker tracker;
  EXPECT_FALSE(press(tracker, 100));
  EXPECT_FALSE(press(tracker, 4000));
  EXPECT_FALSE(press(tracker, 5200));
  EXPECT_TRUE(press(tracker, 5400));
}

TEST(Tracker, HelpWorksAcrossMillisRollover) {
  Tracker tracker;
  EXPECT_FALSE(press(tracker, UINT32_MAX - 2000));
  EXPECT_FALSE(press(tracker, UINT32_MAX - 1000));
  EXPECT_TRUE(press(tracker, 100));
}

TEST(Tracker, BounceAndHoldsDoNotCountAsExtraPresses) {
  Tracker tracker;
  EXPECT_FALSE(tracker.checkButton(true, 100));
  EXPECT_FALSE(tracker.checkButton(false, 110));
  EXPECT_FALSE(tracker.checkButton(true, 120));
  EXPECT_FALSE(tracker.checkButton(true, 150));
  EXPECT_FALSE(tracker.checkButton(true, 1000));
  EXPECT_FALSE(tracker.checkButton(true, 5000));
  tracker.checkButton(false, 5010);
  tracker.checkButton(false, 5040);
  EXPECT_FALSE(press(tracker, 5100));
  EXPECT_FALSE(press(tracker, 5300));
  EXPECT_TRUE(press(tracker, 5500));
}

TEST(Tracker, FormatsCoordinatesAndTenthsHDOP) {
  char text[161];
  Tracker::formatLocation(text, sizeof(text), -1, 180000000, 11, 23);
  EXPECT_STREQ(text, "Location: lat=-0.000001 lon=180.000000 HDOP=1.1 satellites_in_view=23");
  Tracker::formatLocation(text, sizeof(text), 0, 0, -1, -1);
  EXPECT_STREQ(text, "Location: lat=0.000000 lon=0.000000 HDOP=unknown satellites_in_view=unknown");
  char small[8];
  Tracker::formatLocation(small, sizeof(small), 0, 0, 0, 0);
  EXPECT_EQ(small[7], '\0');
}

TEST(NMEASatellites, CountsViewRatherThanFixSatellites) {
  NMEASatellites satellites;
  EXPECT_EQ(satellites.count(0), -1);
  satellites.update("$GPGGA,1,2,3,4", 0);
  EXPECT_EQ(satellites.count(0), -1);
  satellites.update("$GPGSV,3,1,12,", 100);
  satellites.update("$GPGSV,3,2,12,", 200);
  satellites.update("$GLGSV,2,1,08,", 200);
  EXPECT_EQ(satellites.count(200), 20);
  satellites.update("$GPGSV,3,1,12,", 300);
  EXPECT_EQ(satellites.count(300), 20);
  EXPECT_EQ(satellites.count(10301), -1);
}

TEST(NMEASatellites, DoesNotDoubleCountCombinedGNSSOrBeiDouAliases) {
  NMEASatellites satellites;
  satellites.update("$GBGSV,2,1,08,", 0);
  satellites.update("$GDGSV,2,1,08,", 0);
  EXPECT_EQ(satellites.count(0), 8);
  satellites.update("$GNGSV,4,1,16,", 0);
  EXPECT_EQ(satellites.count(0), 16);
}

TEST(NMEASatellites, RefreshesMatchingTalkerBeforeReusingExpiredSlot) {
  NMEASatellites satellites;
  satellites.update("$GPGSV,3,1,12,", 0);
  satellites.update("$GLGSV,2,1,08,", 5000);
  satellites.update("$GLGSV,2,1,08,", 11000);
  EXPECT_EQ(satellites.count(11000), 8);
}

TEST(NMEASatellites, RejectsMalformedCountsAndHandlesRollover) {
  NMEASatellites satellites;
  satellites.update("", 0);
  satellites.update("$GPGSV,0,1,12,", 0);
  satellites.update("$GPGSV,1,1,999,", 0);
  satellites.update("$GPGSV,1,1,-1,", 0);
  EXPECT_EQ(satellites.count(0), -1);
  satellites.update("$GPGSV,1,1,00,", UINT32_MAX - 500);
  EXPECT_EQ(satellites.count(100), 0);
  EXPECT_EQ(satellites.count(10000), -1);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
