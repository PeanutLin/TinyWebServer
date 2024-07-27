#include "timerList.h"

#include "../http/httpConn.h"

// 构造函数
timerSortedList::timerSortedList() {
  head = new timerNode;
  tail = new timerNode;
  head->prev = tail->next = nullptr;
  head->next = tail;
  tail->prev = head;
  timerCount = 0;
}

// 析构函数
timerSortedList::~timerSortedList() {
  timerNode* tmp = head;
  while (tmp != nullptr) {
    head = tmp->next;
    delete tmp;
    tmp = head;
  }
}

// 添加 timer 到 sortedList 中 first 的后面
void timerSortedList::addTimer(timerNode* first, timerNode* timer) {
  if (timer == nullptr) {
    return;
  }
  timerCount++;
  auto tmp = first;
  while (tmp->next != tail) {
    auto nextNode = tmp->next;
    if (nextNode->expire > timer->expire) {
      tmp->next = timer;
      timer->prev = tmp;
      timer->next = nextNode;
      nextNode->prev = timer;
      // 插入后跳出循环
      break;
    } else {
      tmp = nextNode;
    }
  }

  // 插入到链表末尾
  if (tmp->next == tail) {
    tmp->next = timer;
    timer->prev = tmp;
    timer->next = tail;
    tail->prev = timer;
  }
}

// 调整 timer 在 sortedList 上的位置，时间调整是单向的
void timerSortedList::adjustTimer(timerNode* timer) {
  if (timer == nullptr) {
    return;
  }

  auto tmp = timer->prev;

  // 先将待调整的 timeer 从 sotedList 上取下来
  auto prevNode = timer->prev;
  auto nextNode = timer->next;
  prevNode->next = nextNode;
  nextNode->prev = prevNode;

  // 从 tmp 后接着插入
  addTimer(tmp, timer);
}

// 在 sortedList 删除 timer
void timerSortedList::deleteTimer(timerNode* timer) {
  if (timer == nullptr) {
    return;
  }
  timerCount--;
  auto prevNode = timer->prev;
  auto nextNode = timer->next;
  prevNode->next = nextNode;
  nextNode->prev = prevNode;
  delete timer;
}

// 超时处理，将 curTime（包含）前的 Timer 清除
void timerSortedList::tick(time_t curTime) {
  if (head->next == tail) {
    return;
  }

  timerNode* tmp = head;
  while (tmp->next != tail) {
    auto nextNode = tmp->next;
    if (curTime < nextNode->expire) {
      break;
    } else {
      // 超时处理
      if (nextNode->callBackFunction != nullptr) {
        nextNode->callBackFunction(nextNode->userData);
      }
      deleteTimer(nextNode);
    }
  }
}
