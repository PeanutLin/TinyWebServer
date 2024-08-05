#pragma once
#include <functional>
#include <map>
#include <string>
#include <vector>

// 将 string 按照 delimiter 划分
std::vector<std::string> split(const std::string& str, char delimiter);

// 字典树节点
struct Node {
  // 当前节点路由信息
  std::string part;
  // 指向儿子节点
  std::vector<Node*> child;
  // 当前节点是否为结尾节点
  bool isEnd;
  // 是否精确匹配
  bool exactMatch;
};

typedef std::function<void()> handlerFunction;

// 字典树
class Trie {
 public:
  // 构造函数
  Trie();
  // 析构函数
  ~Trie();

 public:
  // 字典树头节点（/）
  Node* root;

  // 插入路由
  void insert(std::string path);
  // 查询路由
  bool search(std::string path, std::map<std::string, std::string>& mp);

 public:
  // 设置路由路径以及路由的函数
  void addRoute(std::string& path, handlerFunction fn);
  // 获取路由对应的函数，和路由的数据
  bool getRoute(std::string& path, handlerFunction& fn,
                std::map<std::string, std::string>& mp);
};