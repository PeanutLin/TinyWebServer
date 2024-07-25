#include "timerList.h"

#include "../http/httpConn.h"

// 构造函数
timerSortedList::timerSortedList() {
  head = new timerNode;
  tail = new timerNode;
  head->prev = tail->next = nullptr;
  head->next = tail;
  tail->prev = head;
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

// 添加 timer 到 sortedList
void timerSortedList::addTimer(timerNode* timer) {
  if (timer == nullptr) {
    return;
  }

  // 插入头开始插入
  auto tmp = head;
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

  //  从 sotedList 上取下来
  auto prevNode = timer->prev;
  auto nextNode = timer->next;
  prevNode->next = nextNode;
  nextNode->prev = prevNode;

  // 从 tmp 后接着插入
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

  if (tmp->next == tail) {  // 插入到链表末尾
    tmp->next = timer;
    timer->prev = tmp;
    timer->next = tail;
    tail->prev = timer;
  }
}

// 在 sortedList 删除 timer
void timerSortedList::delTimer(timerNode* timer) {
  if (timer == nullptr) {
    return;
  }
  auto prevNode = timer->prev;
  auto nextNode = timer->next;
  prevNode->next = nextNode;
  nextNode->prev = prevNode;
  delete timer;
}

// 滴答，处理超时客户端
void timerSortedList::tick() {
  if (head->next == tail) {
    return;
  }

  time_t curTime = time(nullptr);
  timerNode* tmp = head;
  while (tmp->next != tail) {
    auto nextNode = tmp->next;
    if (curTime < nextNode->expire) {
      break;
    } else {
      // 超时处理
      if (tmp->callBackFunction != nullptr) {
        tmp->callBackFunction(nextNode->userData);
      }
      delTimer(nextNode);
    }
  }
}
