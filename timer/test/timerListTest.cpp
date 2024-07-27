#include <gtest/gtest.h>
#include "../timerList.h"

TEST(TimerListTest, TimerListTest) {
  timerSortedList* timerList = new timerSortedList;
  timerNode* timer1 = new timerNode;
  timerNode* timer2 = new timerNode;
  timer1->expire = 1722081601;
  timer2->expire = 1722081600;
  timerList->addTimer(timerList->head, timer1);
  timerList->addTimer(timerList->head, timer2);

  // 第一个 timer 应该为 timer2
  EXPECT_EQ(timer2, timerList->head->next);
  // 此时的 timer 数量应该为 2
  EXPECT_EQ(timerList->timerCount, 2);

  timerList->tick(1722081600);

  // 第一个 timer 应该为 timer1
  EXPECT_EQ(timer1, timerList->head->next);
  // 此时的 timer 数量应该为 1
  EXPECT_EQ(timerList->timerCount, 1);

  timerList->tick(1722081603);

  // 第一个 timer 应该为 null
  EXPECT_EQ(timerList->tail, timerList->head->next);
  // 此时的 timer 数量应该为 1
  EXPECT_EQ(timerList->timerCount, 0);
}

TEST(TimerListAdjustTest, TimerListTest) {
  timerSortedList* timerList = new timerSortedList;
  timerNode* timer1 = new timerNode;
  timerNode* timer2 = new timerNode;
  timerNode* timer3 = new timerNode;
  timerNode* timer4 = new timerNode;
  timerNode* timer5 = new timerNode;
  timer1->expire = 1722081605;
  timer2->expire = 1722081603;
  timer3->expire = 1722081604;
  timer4->expire = 1722081602;
  timer5->expire = 1722081601;
  timerList->addTimer(timerList->head, timer1);
  timerList->addTimer(timerList->head, timer2);
  timerList->addTimer(timerList->head, timer3);
  timerList->addTimer(timerList->head, timer4);
  timerList->addTimer(timerList->head, timer5);

  // 最后一个 timer 应该是 timer1
  EXPECT_EQ(timerList->tail->prev, timer1);
  // 第一个 timer 应该是 timer5
  EXPECT_EQ(timerList->head->next, timer5);
  EXPECT_EQ(timerList->timerCount, 5);

  timer2->expire = 1722081606;
  timerList->adjustTimer(timer2);
  // 最后一个 timer 应该是 timer2
  EXPECT_EQ(timerList->tail->prev, timer2);

  timerList->tick(1722081604);
  EXPECT_EQ(timerList->timerCount, 3);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}